## v0.2.0 (2026-03-24)

### ✨ New Features

- add plugin system support for loading optional features *(Roland Dobai - b299965)*

### 📖 Documentation

- add CONTRIBUTING.md with coding conventions and review guide *(erhankur - db4ca08)*


## v0.1.0 (2026-03-19)

### ✨ New Features

- **esp32c61**: Add large flash support *(Jaroslav Burian - edb0e0b)*
- **security**: add api to check flash is encrypted *(erhankur - 96e788b)*
- **flash**: Set flash size to maximum supported when ID is unknown *(Jaroslav Burian - 70373a5)*
- **flash**: Support custom SPI flash pin configuration *(Jaroslav Burian - 6d9c71a)*
- **flash**: Support eFuse SPI pin configuration *(Jaroslav Burian - 334a201)*
- **flash**: Add 4 byte flashing support for ESP32-P4 and ESP32-S3 *(Jaroslav Burian - 6e7bb85)*
- **flash**: Add support for 4 byte address flashing *(Jaroslav Burian - af03c85)*
- **usb-otg**: Finish the USB-OTG RX/TX support *(Radim Karniš - b50a3ce)*
- **usb_serial_jtag**: Add USB-Serial/JTAG transport layer support *(Radim Karniš - 58272f8)*
- add SHA256 hash calculation APIs *(erhankur - f0827f5)*
- set log level in runtime *(erhankur - 6df67f1)*
- Add asynchronous erase function *(Jaroslav Burian - f7ce526)*
- Add miniz rom wrappers *(Jaroslav Burian - eff7a03)*
- Add MD5 check ROM wrappers *(Jaroslav Burian - 1b234e6)*
- Disable Watchdogs when USB-Serial-JTAG is used *(Jaroslav Burian - 3489324)*
- Add CPU clock increase support *(Jaroslav Burian - 3661a14)*
- add support for the USB-CDC feature *(Jaroslav Safka - ac03ab8)*
- esp32p4 eco5 support *(erhankur - b6f33c1)*
- Add basic flashing support *(Jaroslav Burian - 4101650)*
- Add UART support *(Jaroslav Burian - ec7beca)*
- add support for security info *(Jaroslav Safka - 4797c8e)*
- add target soc_caps.h *(erhankur - 709ef1e)*
- add mem utils, for checking memory range types *(Anton Maklakov - 10b0cfa)*
- add soc header for targets *(Anton Maklakov - 045250d)*
- Add wrappers for UART RX/TX ROM funcs *(Radim Karniš - c9b98d0)*
- add S3 initial read support *(Anton Maklakov - d1503e5)*
- update structure for initial SPI Flash support *(Anton Maklakov - f54ee9e)*
- add ROM function wrappers *(erhankur - cfcc920)*
- Add initial error codes *(Anton Maklakov - facae9e)*
- add xtensa trax memory helpers *(erhankur - 3c8a527)*
- add libc rom linker files *(erhankur - 56c7d97)*

### 🐛 Bug Fixes

- **esp32**: save/restore SPI1 user register across flash operations *(erhankur - 5a5be66)*
- **esp8266**: Use custom flash config due to different ROM implementation *(Jaroslav Burian - 3eb4aa1)*
- **esp32s2**: wait for SPI0 idle using correct peripheral index *(erhankur - f419d01)*
- **flash**: wait for erase complete before returning from erase_area *(erhankur - cef3cb3)*
- **flash**: change erase_area function to support large flash mode *(erhankur - 3718892)*
- **esp32c2**: Improve crystal frequency detection by comparing to internal *(Jaroslav Burian - 899c483)*
- **esp32h21**: use correct ROM addresses for libc functions *(erhankur - 983c4e0)*
- **esp32h4**: Use the latest ld files and support clock init *(Radim Karniš - 2019d1e)*
- **usb_serial_jtag**: Use custom implementation of flush and tx as ROM flushes on newline character *(Jaroslav Burian - aa2f920)*
- **flash**: Wait for SPI0 to be ready when there is no HW arbiter *(Jaroslav Burian - 280cac7)*
- **flash**: Properly calculate timeout for data shorter than 1KB *(Jaroslav Burian - 29e0730)*
- **flash**: Do not save flash ID to ROM read only structure *(Jaroslav Burian - 49cc6b5)*
- **esp8266**: Fix flash write enable by custom implementation instead of ROM *(Jaroslav Burian - c8a6296)*
- **esp8266**: Properly attach SPI flash *(Jaroslav Burian - b56f101)*
- **esp32s3**: Fix LoadStoreException during flashing over USB OTG *(Jaroslav Burian - 2541cc9)*
- **esp32s3**: APB frequency is equal to XTAL frequency when PLL is not used *(Jaroslav Burian - 5e8492b)*
- **md5**: Use proper context structure for esp32c2 *(Jaroslav Burian - a30a4d5)*
- **md5**: Use proper context structure for esp32c2 *(Jaroslav Burian - 46ecbd5)*
- **esp32c3**: Use XTAL frequency for UART clock which is default in ROM *(Jaroslav Burian - ab85095)*
- **esp32c3**: Remove return causing security info unsupported feature error *(Jaroslav Burian - bcc2c2e)*
- **esp32c2**: Detect APB frequency even without clock initialization *(Jaroslav Burian - 46deae2)*
- **esp32s3**: Properly detect APB frequency *(Jaroslav Burian - da588ec)*
- **usb-otg**: Add missing ld aliases *(Radim Karniš - 7615fe8)*
- **clock**: Wait for the clocks to stabilize after init *(Radim Karniš - 545d100)*
- **log**: log output aligned in flash.c *(erhankur - 8b0183f)*
- **target**: allow strong functions override weak ones *(erhankur - 4382edd)*
- **esp8266**: Add missing flash function definitions and implement UART TX flush *(Jaroslav Burian - e64d437)*
- **uart**: Fixed initialization for the early stages for many chips *(Anton Maklakov - 19bd30f)*
- **include**: Make the headers includable from C++ *(Roland Dobai - d7e68bc)*
- add clock.c to esp32h21 build *(erhankur - b943a84)*
- Use correct clock for UART baudrate *(Jaroslav Burian - e84786a)*
- update copyright header *(Anton Maklakov - 6c2970d)*
- clean up error codes *(Anton Maklakov - a58afdd)*

### 📖 Documentation

- **esp-stub-lib**: add code conventions for include style *(erhankur - fdf131d)*
- add MAINTENANCE.md and link from README *(Roland Dobai - 01cef04)*

### 🔧 Code Refactoring

- **aes_xts**: Use common helper function to wait for register state *(Jaroslav Burian - 05d54e6)*
- move common flash functions to the common layer *(erhankur - d8146e0)*
- cleanup unused block/sector erase functions *(erhankur - 4155edc)*
- print flash logs as verbose or error *(erhankur - 4443dcc)*
- improve flash api returns *(erhankur - c7a56aa)*
- Simplify USB-Serial-JTAG code *(Jaroslav Burian - 9471844)*
- Make soc utils public *(Jaroslav Burian - c75846d)*
- move target interface to /include/target/ *(Anton Maklakov - 4d26034)*
- cleanup useless uart headers *(erhankur - bcb85ef)*

---

## v0.0.1 (2025-03-17)
