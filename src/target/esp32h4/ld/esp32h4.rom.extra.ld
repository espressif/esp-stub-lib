/***************************************
 * Manually added ROM symbols for esp32h4
 * These symbols are not in the auto-generated ROM linker scripts
 ***************************************/

/* Dispatch table of the ROM SPI user-command primitives. Part of the ROM
 * interface, but missing from the generated esp32h4 linker scripts, so it is
 * pinned here until it is added there. Populated by the ROM, so the functions
 * it points to are always the ones for the running ROM revision. */
rom_spi_usr_cmd_legacy_funcs = 0x4085ff34;
