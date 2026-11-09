/***************************************
 * Manually added ROM symbols for esp32c61
 * These symbols are not in the auto-generated ROM linker scripts
 * but are needed for ECO version handling
 ***************************************/

/* ROM version variable */
_rom_eco_version = 0x40000014;
esp_rom_opiflash_exec_cmd_eco0 = 0x40029d5a; /* ECO0/1 */
esp_rom_opiflash_exec_cmd_eco2 = 0x4002a014;
esp_rom_opiflash_exec_cmd_eco3 = 0x4002acea;
esp_rom_opiflash_exec_cmd_eco4 = 0x4002b91e;
esp_rom_opiflash_exec_cmd_eco5 = 0x4002bc6e;

/* SDIO slave (ECO3 and newer) */
sip_download_begin = 0x400014e8;
sip_get_ptr = 0x400014ec;
slc_from_host_chain_fetch = 0x40001524;
slc_send_to_host_chain = 0x4000153c;
slc_to_host_chain_recycle = 0x40001544;
