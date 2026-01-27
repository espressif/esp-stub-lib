/***************************************
 * Manually added ROM symbols for esp32c5
 * These symbols are not in the auto-generated ROM linker scripts
 * but are needed for ECO version handling
 ***************************************/

/* ROM version variables */
_rom_eco_version = 0x40000014;
esp_rom_opiflash_exec_cmd_eco2 = 0x4002c42a;
esp_rom_opiflash_exec_cmd_eco3 = 0x4002e9f4;
