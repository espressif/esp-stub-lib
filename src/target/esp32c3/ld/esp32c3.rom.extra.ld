/***************************************
 * Manually added ROM symbols for esp32c3
 * These symbols are not in the auto-generated ROM linker scripts
 * but are needed for ECO version handling
 ***************************************/

/* ROM version variables */
_rom_chip_id = 0x40000010;
_rom_eco_version = 0x40000014;

/* GetSecurityInfoProc functions */
GetSecurityInfoProc = 0x4004d51e; /* default: ECO7 version */
GetSecurityInfoProcEco1 = 0x4004b9da;
GetSecurityInfoProcEco7 = 0x4004d51e; /* from esp32c3eco7-20230720; address of function for eco7+ */
