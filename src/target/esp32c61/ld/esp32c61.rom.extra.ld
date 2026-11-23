/***************************************
 * Manually added ROM symbols for esp32c61
 * These symbols are not in the auto-generated ROM linker scripts
 * but are needed for ECO version handling
 ***************************************/

/* ROM version variable */
_rom_eco_version = 0x40000014;

/* SDIO slave (ECO3 and newer) */
sip_download_begin = 0x400014e8;
sip_get_ptr = 0x400014ec;
slc_from_host_chain_fetch = 0x40001524;
slc_send_to_host_chain = 0x4000153c;
slc_to_host_chain_recycle = 0x40001544;
