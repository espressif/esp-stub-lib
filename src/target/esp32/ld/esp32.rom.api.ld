/** ROM APIs
 */

PROVIDE ( esp_rom_crc32_le = crc32_le );
PROVIDE ( esp_rom_crc16_le = crc16_le );
PROVIDE ( esp_rom_crc8_le  = crc8_le );
PROVIDE ( esp_rom_crc32_be = crc32_be );
PROVIDE ( esp_rom_crc16_be = crc16_be );
PROVIDE ( esp_rom_crc8_be  = crc8_be );

PROVIDE ( esp_rom_uart_flush_tx       = uart_tx_flush );
PROVIDE ( esp_rom_uart_tx_one_char    = uart_tx_one_char );
PROVIDE ( esp_rom_uart_tx_wait_idle   = uart_tx_wait_idle );
PROVIDE ( esp_rom_uart_rx_one_char    = uart_rx_one_char );
PROVIDE ( esp_rom_uart_rx_string      = UartRxString );
PROVIDE ( esp_rom_uart_set_as_console = uart_tx_switch );
PROVIDE ( esp_rom_uart_putc           = ets_write_char_uart );
PROVIDE ( esp_rom_uart_attach         = uartAttach );
PROVIDE ( esp_rom_uart_init           = Uart_Init );
PROVIDE ( esp_rom_uart_div_modify     = uart_div_modify );
PROVIDE ( esp_rom_uart_rx_intr_handler = uart_rx_intr_handler );
PROVIDE ( esp_rom_get_detected_xtal_freq = ets_get_detected_xtal_freq );
PROVIDE ( esp_rom_get_uart_device     = GetUartDevice );

PROVIDE ( esp_rom_output_flush_tx       = uart_tx_flush );
PROVIDE ( esp_rom_output_tx_one_char    = uart_tx_one_char );
PROVIDE ( esp_rom_output_tx_wait_idle   = uart_tx_wait_idle );
PROVIDE ( esp_rom_output_rx_one_char    = uart_rx_one_char );
PROVIDE ( esp_rom_output_rx_string      = UartRxString );
PROVIDE ( esp_rom_output_set_as_console = uart_tx_switch );

PROVIDE ( esp_rom_md5_init   = MD5Init );
PROVIDE ( esp_rom_md5_update = MD5Update );
PROVIDE ( esp_rom_md5_final  = MD5Final );

PROVIDE ( esp_rom_printf   = ets_printf );
PROVIDE ( esp_rom_install_uart_printf = ets_install_uart_printf );
PROVIDE ( esp_rom_delay_us = ets_delay_us );
PROVIDE ( esp_rom_get_reset_reason = rtc_get_reset_reason );
PROVIDE ( esp_rom_route_intr_matrix = intr_matrix_set );
PROVIDE ( esp_rom_isr_attach = ets_isr_attach );
PROVIDE ( esp_rom_isr_unmask = ets_isr_unmask );
PROVIDE ( esp_rom_get_cpu_ticks_per_us = ets_get_cpu_frequency );
PROVIDE ( esp_rom_set_cpu_ticks_per_us = ets_update_cpu_frequency_rom );

PROVIDE ( esp_rom_spiflash_write_enable = SPI_write_enable);
PROVIDE ( esp_rom_spiflash_erase_chip = SPIEraseChip);

PROVIDE ( esp_rom_spiflash_write = SPIWrite );
PROVIDE ( esp_rom_spiflash_write_encrypted = SPI_Encrypt_Write );
PROVIDE ( esp_rom_spiflash_read = SPIRead );
PROVIDE ( esp_rom_spiflash_erase_block = SPIEraseBlock );
PROVIDE ( esp_rom_spiflash_erase_sector = SPIEraseSector );
PROVIDE ( esp_rom_spiflash_erase_area = SPIEraseArea );
