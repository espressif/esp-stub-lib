/***************************************
 * Manually added ROM symbols for esp32s3
 * These symbols are not in the auto-generated ROM linker scripts
 * but are needed for security info and ECO version handling
 ***************************************/

/* GetSecurityInfoProc function */
GetSecurityInfoProc = 0x40048174; /* regular ESP32S3 version */
