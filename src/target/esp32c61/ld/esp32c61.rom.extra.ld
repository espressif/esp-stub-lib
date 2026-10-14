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
