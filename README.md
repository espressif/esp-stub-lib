[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/espressif/esp-stub-lib/master.svg)](https://results.pre-commit.ci/latest/github/espressif/esp-stub-lib/master)

# esp-stub-lib

This project is experimental and not yet ready for production use.

## Supported Targets

- ESP8266
- ESP32
- ESP32-S2
- ESP32-S3
- ESP32-C2
- ESP32-C3
- ESP32-C5
- ESP32-C6
- ESP32-C61
- ESP32-H2
- ESP32-H4
- ESP32-P4

## How to use

The library provides a simple interface for creating stubs that can be loaded onto ESP chips.

A complete example project is provided in the [example](example/) directory. It demonstrates:
- Basic stub implementation
- Flash operations
- Target-specific configurations
- Build system integration

See the [example README](example/README.md) for build instructions.

## Function Naming Conventions

Functions that contain `rominit` (e.g., `stub_target_uart_rominit_intr_attach`) require ROM preinitialization by entering download mode to work correctly. These functions are simplified compared to full implementation and expect some initialization done by ROM. They should only be called in contexts where the ROM download mode has been entered.

## Project Structure

The library uses a three-layer architecture to eliminate circular dependencies and maximize code reuse:

```
esp-stub-lib/
├── include/esp-stub-lib/         # Public API - used by library clients
│   ├── flash.h                   # (stub_lib_flash_init, stub_lib_flash_read, etc.)
│   ├── log.h
│   ├── mem_utils.h
│   └── ...
│
├── src/                          # Implementation layer
│   ├── flash.c
│   ├── mem_utils.c
│   └── ...
│
└── src/target/                   # Internal abstraction layers
    ├── base/                     # Interface layer (headers only)
    │   └── include/
    │       ├── target/           # Internal API between common/target layers
    │       └── private/          # Internal ROM/hardware details
    │
    ├── common/                   # Generic implementations
    │   ├── src/                  # Weak functions using SOC_* macros
    │   │   ├── mem_utils.c       # Default implementations
    │   │   ├── uart.c
    │   │   └── flash.c
    │   └── CMakeLists.txt
    │
    └── esp32*/                   # Target-specific implementations
        ├── include/
        │   └── soc/
        │       └── soc.h         # SOC_* macro definitions
        ├── src/
        │   ├── mem_utils.c       # Strong overrides (optional)
        │   ├── uart.c
        │   └── ...
        └── CMakeLists.txt
```

**Dependency Flow:**
```
Public API (include/esp-stub-lib/)  ← Library clients use this
    ↓
Implementation (src/)
    ↓
Common (generic implementations with weak functions)
    ↓
Target (overridden weak functions for target-specific implementations)
    ↓
Base (interface headers only - serves both common and target)
```

**Layer Purposes:**
- **include/esp-stub-lib/**: Public API for library clients (e.g., `stub_lib_flash_init()`)
- **src/**: Implementation layer that uses the target abstraction below
- **base/**: Internal interface layer - shared by both common and target implementations
  - `target/`: Internal API between common/target layers (e.g., `stub_target_flash_init()`)
  - `private/`: Internal headers to be used from target implementations. (e.g., `esp_rom_spiflash_read()`)
- **common/**: Provides reusable weak implementations that work across targets
- **target/esp32***: Target-specific constants (`soc.h`) and strong function overrides when needed

> **Note**: Library clients should only include headers from `include/esp-stub-lib/`. The `src/target/base/` folder (both `target/` and `private/`) contains internal implementation details used by the library itself.

## Contributing

Please install the [pre-commit](https://pre-commit.com/) hooks to ensure that your commits are properly formatted:

```bash
pip install pre-commit
pre-commit install -t pre-commit -t commit-msg
```

# How To Release (For Maintainers Only)

```bash
pip install commitizen
git fetch
git checkout -b update/release_v1.1.0
git reset --hard origin/master
cz bump
git push -u
git push --tags
```
Create a pull request and edit the automatically created draft [release notes](https://github.com/espressif/esp-stub-lib/releases).

## License

This document and the attached source code are released as Free Software under either the [Apache License Version 2](LICENSE-APACHE) or [MIT License](LICENSE-MIT) at your option.
