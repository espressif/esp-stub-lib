/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/nand.h>
#include <esp-stub-lib/rom_wrappers.h>

#include <target/nand.h>

// SPI NAND commands
#define NAND_CMD_SET_REGISTER        0x1F
#define NAND_CMD_READ_REGISTER       0x0F
#define NAND_CMD_WRITE_ENABLE        0x06
#define NAND_CMD_PAGE_READ           0x13
#define NAND_CMD_PROGRAM_EXECUTE     0x10
#define NAND_CMD_PROGRAM_LOAD        0x02
#define NAND_CMD_PROGRAM_LOAD_RANDOM 0x84
#define NAND_CMD_READ_FROM_CACHE     0x03
#define NAND_CMD_ERASE_BLOCK         0xD8
#define NAND_CMD_READ_ID             0x9F
#define NAND_CMD_RESET               0xFF

#define NAND_MFR_MICRON              0x2C

// NAND_REG_CONFIG bits
#define NAND_CFG_BUF                 BIT(3) /* 1 = Buffer Mode (no auto-prefetch), 0 = Continuous Mode */
#define NAND_CFG_ECC_EN              BIT(4)
#define NAND_CFG_OTP_EN              BIT(6) /* Page reads address the OTP area, which holds the parameter page */

// NAND_REG_STATUS bits
#define NAND_STAT_BUSY               BIT(0)
#define NAND_STAT_ERASE_FAILED       BIT(2)
#define NAND_STAT_PROGRAM_FAILED     BIT(3)

/* Maximum time to wait for any NAND operation.
 * Worst case per W25N01GV datasheet: block erase tBERS max = 10 ms. */
#define NAND_OP_TIMEOUT_US           1000000U
#define NAND_POLL_INTERVAL_US        10U

/* Bytes moved by one bus transaction. */
#define NAND_XFER_MAX                64U

/* PAGE_READ, PROGRAM_EXECUTE and ERASE_BLOCK take a 24-bit row address. */
#define NAND_ROW_ADDR_BITS           24U

/* Parameter page: page 1 of the OTP area, laid out like the ONFI parameter page and
 * stored as redundant copies. Field offsets below are within one copy. */
#define ONFI_PAGE                    0x01
#define ONFI_COPY_SIZE               256U
#define ONFI_COPIES                  3U
#define ONFI_PAGE_SIZE_OFFSET        80U
#define ONFI_PAGES_PER_BLOCK_OFFSET  92U
#define ONFI_BLOCKS_PER_LUN_OFFSET   96U
#define ONFI_LUN_COUNT_OFFSET        100U
#define ONFI_CRC_OFFSET              254U
#define ONFI_CRC_INIT                0x4F4EU
#define ONFI_CRC_POLY                0x8005U

/* Vendor differences. Filled in code, not from a const table: the flasher stub links the NAND
 * code as a plugin that has no .rodata section. */
typedef struct {
    uint32_t plane_select;  /* Column address bit that selects the cache of odd blocks */
    uint8_t ecc_mask;       /* NAND_REG_STATUS bits holding the ECC result */
    bool micron_ecc_status; /* ECC field uses the Micron encoding */
    bool has_buf;           /* NAND_CFG_BUF must be set to disable continuous read */
} nand_vendor_t;

static struct {
    stub_lib_nand_geometry_t geometry;
    uint32_t block_size;
    nand_vendor_t vendor;
    uint8_t last_status;
    bool initialized;
} s_nand;

static uint8_t s_onfi_copy[ONFI_COPY_SIZE] __attribute__((aligned(4)));

static void nand_vendor_init(uint8_t manufacturer_id)
{
    if (manufacturer_id == NAND_MFR_MICRON) {
        /* Plane select sits above the 12-bit column address. ECC result in STATUS[6:4]. BUF is reserved. */
        s_nand.vendor.plane_select = BIT(12);
        s_nand.vendor.ecc_mask = BIT(6) | BIT(5) | BIT(4);
        s_nand.vendor.micron_ecc_status = true;
        s_nand.vendor.has_buf = false;
    } else {
        /* Winbond and GigaDevice: plane select at CA[15], ECC result in STATUS[5:4]. */
        s_nand.vendor.plane_select = BIT(15);
        s_nand.vendor.ecc_mask = BIT(5) | BIT(4);
        s_nand.vendor.micron_ecc_status = false;
        s_nand.vendor.has_buf = true;
    }
}

static uint32_t nand_plane_bit(uint32_t page_number)
{
    uint32_t block_number = page_number / s_nand.geometry.pages_per_block;
    return (block_number & 1U) ? s_nand.vendor.plane_select : 0;
}

static uint32_t nand_row(uint32_t page_number)
{
    return page_number & (BIT(NAND_ROW_ADDR_BITS) - 1U);
}

static int nand_check_page(uint32_t page_number)
{
    if (!s_nand.initialized) {
        return NAND_ERR_NOT_INITIALIZED;
    }
    if (page_number / s_nand.geometry.pages_per_block >= s_nand.geometry.block_count) {
        return STUB_LIB_ERR_INVALID_ARG;
    }
    return 0;
}

static int nand_write_register(uint8_t reg, uint8_t val)
{
    uint8_t data[2] = { reg, val };
    return stub_target_nand_bus_transaction(NAND_CMD_SET_REGISTER, 0, 0, data, 16, NULL, 0);
}

static int nand_write_enable(void)
{
    return stub_target_nand_bus_transaction(NAND_CMD_WRITE_ENABLE, 0, 0, NULL, 0, NULL, 0);
}

/*
 * Two helpers for waiting on NAND operations, split by operation class:
 *
 * nand_wait_ready() — busy-only poll. Used after PAGE_READ and RESET.
 *   Stores the status byte in s_nand.last_status so nand_check_ecc() can inspect
 *   the ECC field from the same status read.  Does NOT inspect P_FAIL/E_FAIL
 *   because those bits are sticky on the W25N01GV: they are set by a failed
 *   PROGRAM_EXECUTE or ERASE_BLOCK and cleared only by the next PROGRAM or
 *   ERASE operation (or by RESET).  A PAGE_READ following a failed program
 *   would therefore see a stale P_FAIL=1 and incorrectly return an error.
 *
 * nand_wait_program_erase() — busy poll + P_FAIL/E_FAIL check. Used after
 *   PROGRAM_EXECUTE and ERASE_BLOCK, where the fail bits are fresh.
 */
static int nand_wait_ready(void)
{
    uint32_t polls = NAND_OP_TIMEOUT_US / NAND_POLL_INTERVAL_US;

    /* Give the chip time to assert OIP (BUSY) before the first poll. */
    stub_lib_delay_us(NAND_POLL_INTERVAL_US);

    while (polls-- > 0) {
        uint8_t status;
        int ret = stub_lib_nand_read_register(NAND_REG_STATUS, &status);
        if (ret != 0) {
            return ret;
        }

        s_nand.last_status = status;
        if ((status & NAND_STAT_BUSY) == 0) {
            return 0;
        }

        stub_lib_delay_us(NAND_POLL_INTERVAL_US);
    }

    return NAND_ERR_TIMEOUT;
}

static int nand_wait_program_erase(void)
{
    int ret = nand_wait_ready();
    if (ret != 0) {
        return ret;
    }

    if (s_nand.last_status & NAND_STAT_ERASE_FAILED) {
        return NAND_ERR_ERASE_FAILED;
    }
    if (s_nand.last_status & NAND_STAT_PROGRAM_FAILED) {
        return NAND_ERR_PROGRAM_FAILED;
    }
    return 0;
}

/* Check the ECC result of the last PAGE_READ (stored by nand_wait_ready()). */
static int nand_check_ecc(void)
{
    uint8_t ecc = s_nand.last_status & s_nand.vendor.ecc_mask;

    if (s_nand.vendor.micron_ecc_status) {
        /* 000 is clean; 001, 011 and 101 are corrected. 010 is
         * uncorrectable and the other values are reserved. */
        if (ecc == 0 || ecc == BIT(4) || ecc == (BIT(5) | BIT(4)) || ecc == (BIT(6) | BIT(4))) {
            return 0;
        }
        return NAND_ERR_ECC_UNCORRECTABLE;
    }

    /* 00 is clean, 01 corrected, 10 uncorrectable. Treat reserved 11 as uncorrectable too. */
    if (ecc & BIT(5)) {
        return NAND_ERR_ECC_UNCORRECTABLE;
    }
    return 0;
}

/* Load a page from the array into the chip's cache. */
static int nand_load_page(uint32_t page_number)
{
    int ret = stub_target_nand_bus_transaction(NAND_CMD_PAGE_READ,
                                               nand_row(page_number),
                                               NAND_ROW_ADDR_BITS,
                                               NULL,
                                               0,
                                               NULL,
                                               0);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }
    return nand_wait_ready();
}

static int nand_read_cache(uint32_t column, uint8_t *buf, uint32_t len)
{
    for (uint32_t offset = 0; offset < len; offset += NAND_XFER_MAX) {
        uint32_t chunk = MIN(len - offset, NAND_XFER_MAX);
        /* READ_FROM_CACHE format: CMD + CA_H + CA_L + DUMMY + DATA.
         * Shift the column left by 8 so the 24-bit address carries the dummy byte. */
        int ret = stub_target_nand_bus_transaction(NAND_CMD_READ_FROM_CACHE,
                                                   (column + offset) << 8,
                                                   24,
                                                   NULL,
                                                   0,
                                                   buf + offset,
                                                   (uint16_t)(chunk * 8));
        if (ret != 0) {
            return NAND_ERR_SPI_FAIL;
        }
    }
    return 0;
}

/* PROGRAM_LOAD sets the rest of the cache to 0xFF, PROGRAM_LOAD_RANDOM keeps it. */
static int nand_load_cache(uint8_t first_cmd, uint32_t column, const uint8_t *buf, uint32_t len)
{
    for (uint32_t offset = 0; offset < len; offset += NAND_XFER_MAX) {
        uint32_t chunk = MIN(len - offset, NAND_XFER_MAX);
        uint8_t cmd = offset == 0 ? first_cmd : NAND_CMD_PROGRAM_LOAD_RANDOM;
        int ret =
            stub_target_nand_bus_transaction(cmd, column + offset, 16, buf + offset, (uint16_t)(chunk * 8), NULL, 0);
        if (ret != 0) {
            return NAND_ERR_SPI_FAIL;
        }
    }
    return 0;
}

static int nand_program_execute(uint32_t page_number)
{
    int ret = stub_target_nand_bus_transaction(NAND_CMD_PROGRAM_EXECUTE,
                                               nand_row(page_number),
                                               NAND_ROW_ADDR_BITS,
                                               NULL,
                                               0,
                                               NULL,
                                               0);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }
    return nand_wait_program_erase();
}

static uint32_t get_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t onfi_crc16(const uint8_t *buf, uint32_t len)
{
    uint32_t crc = ONFI_CRC_INIT;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= (uint32_t)buf[i] << 8;
        for (int bit = 0; bit < 8; bit++) {
            crc = ((crc << 1) ^ ((crc & BIT(15)) ? ONFI_CRC_POLY : 0U)) & 0xFFFFU;
        }
    }
    return (uint16_t)crc;
}

static bool onfi_copy_valid(const uint8_t *copy)
{
    if (copy[0] != 'O' || copy[1] != 'N' || copy[2] != 'F' || copy[3] != 'I') {
        return false;
    }
    uint16_t crc = (uint16_t)(copy[ONFI_CRC_OFFSET] | (copy[ONFI_CRC_OFFSET + 1] << 8));
    return onfi_crc16(copy, ONFI_CRC_OFFSET) == crc;
}

static int nand_read_onfi_copies(stub_lib_nand_geometry_t *onfi)
{
    int ret = nand_load_page(ONFI_PAGE);
    if (ret != 0) {
        return ret;
    }

    for (uint32_t copy = 0; copy < ONFI_COPIES; copy++) {
        ret = nand_read_cache(copy * ONFI_COPY_SIZE, s_onfi_copy, ONFI_COPY_SIZE);
        if (ret != 0) {
            return ret;
        }
        if (onfi_copy_valid(s_onfi_copy)) {
            onfi->page_size = get_le32(&s_onfi_copy[ONFI_PAGE_SIZE_OFFSET]);
            onfi->pages_per_block = get_le32(&s_onfi_copy[ONFI_PAGES_PER_BLOCK_OFFSET]);
            onfi->block_count = get_le32(&s_onfi_copy[ONFI_BLOCKS_PER_LUN_OFFSET]) * s_onfi_copy[ONFI_LUN_COUNT_OFFSET];
            return 0;
        }
    }
    return NAND_ERR_GEOMETRY_UNKNOWN;
}

/*
 * Read the geometry from the parameter page. cfg is the configuration to restore afterwards.
 * Micron and Winbond document the access sequence. ECC stays off because the parameter page has
 * no ECC parity. BUF stays as in cfg: with BUF=0, Winbond reads ignore the column address.
 */
static int nand_read_onfi(uint8_t cfg, stub_lib_nand_geometry_t *onfi)
{
    int ret = nand_write_register(NAND_REG_CONFIG, (uint8_t)((cfg & ~NAND_CFG_ECC_EN) | NAND_CFG_OTP_EN));
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }

    ret = nand_read_onfi_copies(onfi);

    if (nand_write_register(NAND_REG_CONFIG, cfg) != 0) {
        return NAND_ERR_SPI_FAIL;
    }
    return ret;
}

static int nand_check_geometry(const stub_lib_nand_geometry_t *geometry)
{
    if (geometry->page_size == 0 || geometry->pages_per_block == 0 || geometry->block_count == 0) {
        STUB_LOGE("NAND geometry unknown; the chip has no parameter page\n");
        return NAND_ERR_GEOMETRY_UNKNOWN;
    }
    /* The column address and the vendor plane select bit fit in 16 bits. */
    if (geometry->page_size != 2048 && geometry->page_size != 4096) {
        STUB_LOGE("NAND page size %u is not 2048 or 4096\n", geometry->page_size);
        return STUB_LIB_ERR_INVALID_ARG;
    }
    uint64_t pages = (uint64_t)geometry->pages_per_block * geometry->block_count;
    if (pages > BIT(NAND_ROW_ADDR_BITS)) {
        STUB_LOGE("NAND has more pages than a 24-bit row address reaches\n");
        return STUB_LIB_ERR_INVALID_ARG;
    }
    return 0;
}

int stub_lib_nand_read_register(uint8_t reg, uint8_t *val)
{
    if (val == NULL) {
        return STUB_LIB_ERR_INVALID_ARG;
    }
    return stub_target_nand_bus_transaction(NAND_CMD_READ_REGISTER, reg, 8, NULL, 0, val, 8);
}

int stub_lib_nand_read_id(uint8_t *manufacturer_id, uint16_t *device_id)
{
    uint8_t id_buf[3];
    int ret = stub_target_nand_bus_transaction(NAND_CMD_READ_ID, 0x00, 8, NULL, 0, id_buf, 24);
    if (ret != 0) {
        return ret;
    }
    if (manufacturer_id) {
        *manufacturer_id = id_buf[0];
    }
    if (device_id) {
        *device_id = (uint16_t)(id_buf[1] << 8 | id_buf[2]);
    }
    return 0;
}

int stub_lib_nand_attach(const stub_lib_nand_pins_t *pins, stub_lib_nand_geometry_t *geometry)
{
    if (pins == NULL || geometry == NULL) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    s_nand.initialized = false;
    s_nand.last_status = 0xFF;

    int ret = stub_target_nand_bus_init(pins);
    if (ret != 0) {
        return ret;
    }

    /* Allow the SPI peripheral to stabilize after clock enable and reset release */
    stub_lib_delay_us(5000);

    ret = stub_target_nand_bus_transaction(NAND_CMD_RESET, 0, 0, NULL, 0, NULL, 0);
    if (ret != 0) {
        return NAND_ERR_RESET_FAILED;
    }

    /* W25N01GV tRST is up to 500 us for power-on reset; wait 10 ms */
    stub_lib_delay_us(10000);

    ret = nand_wait_ready();
    if (ret != 0) {
        return ret;
    }

    uint8_t manufacturer_id = 0;
    ret = stub_lib_nand_read_id(&manufacturer_id, NULL);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }
    nand_vendor_init(manufacturer_id);

    ret = nand_write_register(NAND_REG_PROTECT, 0x00);
    if (ret != 0) {
        return ret;
    }

    uint8_t prot = 0xFF;
    ret = stub_lib_nand_read_register(NAND_REG_PROTECT, &prot);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }
    if (prot != 0x00) {
        return NAND_ERR_PROTECTION;
    }

    /*
     * BUF=1 (Buffer Mode) disables the auto-prefetch of BUF=0 (Continuous Mode). In
     * Continuous Mode, READ_FROM_CACHE completion triggers a PAGE_READ of the next page.
     * An explicit PAGE_READ that arrives while it runs (OIP=1) is ignored, and the reads
     * of odd blocks then return all zeros. Micron reserves this bit.
     */
    uint8_t cfg = 0;
    ret = stub_lib_nand_read_register(NAND_REG_CONFIG, &cfg);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }
    cfg = (uint8_t)(cfg & ~(NAND_CFG_OTP_EN | NAND_CFG_ECC_EN));
    if (s_nand.vendor.has_buf) {
        cfg = (uint8_t)(cfg | NAND_CFG_BUF);
    }

    if (geometry->page_size == 0 || geometry->pages_per_block == 0 || geometry->block_count == 0) {
        stub_lib_nand_geometry_t onfi = { 0 };
        ret = nand_read_onfi(cfg, &onfi);
        if (ret != 0 && ret != NAND_ERR_GEOMETRY_UNKNOWN) {
            return ret;
        }
        if (geometry->page_size == 0) {
            geometry->page_size = onfi.page_size;
        }
        if (geometry->pages_per_block == 0) {
            geometry->pages_per_block = onfi.pages_per_block;
        }
        if (geometry->block_count == 0) {
            geometry->block_count = onfi.block_count;
        }
    }

    ret = nand_check_geometry(geometry);
    if (ret != 0) {
        return ret;
    }

    /*
     * On-chip ECC covers the main area. In the spare area, bytes 0–3 hold the user
     * bad-block marker, which read_bbm/write_bbm use. The chip writes its ECC parity to
     * the rest and ignores host writes there. After each PAGE_READ the STATUS register
     * reports the ECC result.
     */
    cfg = (uint8_t)(cfg | NAND_CFG_ECC_EN);
    ret = nand_write_register(NAND_REG_CONFIG, cfg);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }

    uint8_t cfg_after = 0;
    ret = stub_lib_nand_read_register(NAND_REG_CONFIG, &cfg_after);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }
    if ((cfg_after & (NAND_CFG_ECC_EN | NAND_CFG_BUF)) != (cfg & (NAND_CFG_ECC_EN | NAND_CFG_BUF))) {
        return NAND_ERR_SPI_FAIL;
    }

    s_nand.geometry = *geometry;
    s_nand.block_size = geometry->page_size * geometry->pages_per_block;
    s_nand.initialized = true;
    return 0;
}

int stub_lib_nand_read_bbm(uint32_t page_number, uint8_t *spare_data)
{
    int ret = nand_check_page(page_number);
    if (ret != 0) {
        return ret;
    }
    if (spare_data == NULL || !IS_ALIGNED((uintptr_t)spare_data, 4)) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    ret = nand_load_page(page_number);
    if (ret != 0) {
        return ret;
    }

    /* The spare area starts at column page_size. */
    return nand_read_cache(s_nand.geometry.page_size | nand_plane_bit(page_number), spare_data, 4);
}

int stub_lib_nand_write_bbm(uint32_t page_number, bool is_bad)
{
    int ret = nand_check_page(page_number);
    if (ret != 0) {
        return ret;
    }

    uint8_t marker[4];
    for (uint32_t i = 0; i < sizeof(marker); i++) {
        marker[i] = is_bad ? 0x00 : 0xFF;
    }

    /* Load the page so PROGRAM_LOAD_RANDOM keeps its other bytes. */
    ret = nand_load_page(page_number);
    if (ret != 0) {
        return ret;
    }

    ret = nand_write_enable();
    if (ret != 0) {
        return ret;
    }

    uint32_t column = s_nand.geometry.page_size | nand_plane_bit(page_number);
    ret = nand_load_cache(NAND_CMD_PROGRAM_LOAD_RANDOM, column, marker, sizeof(marker));
    if (ret != 0) {
        return ret;
    }

    return nand_program_execute(page_number);
}

int stub_lib_nand_read_page(uint32_t page_number, uint8_t *buf, uint32_t buf_size)
{
    int ret = nand_check_page(page_number);
    if (ret != 0) {
        return ret;
    }
    if (buf == NULL) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    ret = nand_load_page(page_number);
    if (ret != 0) {
        return ret;
    }

    /* Fail fast on uncorrectable errors: the cache may hold corrupted data. */
    ret = nand_check_ecc();
    if (ret != 0) {
        return ret;
    }

    return nand_read_cache(nand_plane_bit(page_number), buf, MIN(buf_size, s_nand.geometry.page_size));
}

int stub_lib_nand_write_page(uint32_t page_number, const uint8_t *buf, uint32_t buf_size)
{
    int ret = nand_check_page(page_number);
    if (ret != 0) {
        return ret;
    }
    if (buf == NULL) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    uint32_t write_len = MIN(buf_size, s_nand.geometry.page_size);
    if (write_len == 0) {
        return 0;
    }

    ret = nand_write_enable();
    if (ret != 0) {
        return ret;
    }

    ret = nand_load_cache(NAND_CMD_PROGRAM_LOAD, nand_plane_bit(page_number), buf, write_len);
    if (ret != 0) {
        return ret;
    }

    return nand_program_execute(page_number);
}

int stub_lib_nand_erase_block(uint32_t page_number)
{
    int ret = nand_check_page(page_number);
    if (ret != 0) {
        return ret;
    }
    if (page_number % s_nand.geometry.pages_per_block != 0) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    ret = nand_write_enable();
    if (ret != 0) {
        return ret;
    }

    ret = stub_target_nand_bus_transaction(NAND_CMD_ERASE_BLOCK,
                                           nand_row(page_number),
                                           NAND_ROW_ADDR_BITS,
                                           NULL,
                                           0,
                                           NULL,
                                           0);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }

    return nand_wait_program_erase();
}

uint32_t stub_lib_nand_get_page_size(void)
{
    return s_nand.initialized ? s_nand.geometry.page_size : 0;
}

uint32_t stub_lib_nand_get_pages_per_block(void)
{
    return s_nand.initialized ? s_nand.geometry.pages_per_block : 0;
}

uint32_t stub_lib_nand_get_block_size(void)
{
    return s_nand.initialized ? s_nand.block_size : 0;
}

uint32_t stub_lib_nand_get_block_count(void)
{
    return s_nand.initialized ? s_nand.geometry.block_count : 0;
}
