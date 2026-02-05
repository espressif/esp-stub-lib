/***************************************
 * Manually added ROM symbols for esp32p4
 * These symbols are not in the auto-generated ROM linker scripts
 * but are needed for ECO version handling
 ***************************************/

/* ROM version variables */
_rom_chip_id = 0x4fc00010;
_rom_eco_version = 0x4fc00014;
esp_rom_opiflash_exec_cmd_eco1 = 0x4fc0fe76;
esp_rom_opiflash_exec_cmd_eco2 = 0x4fc10320;
esp_rom_opiflash_exec_cmd_eco5 = 0x4fc109e4;
esp_rom_opiflash_exec_cmd_eco6 = 0x4fc10a60;
