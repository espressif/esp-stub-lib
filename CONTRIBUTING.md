# Contributing to esp-stub-lib

This document describes the coding patterns, conventions, and review expectations
we follow in esp-stub-lib. Please read it before submitting a pull request.

For project structure and include style, see [README.md](README.md).

---

## Philosophy

This document exists to reduce unnecessary back-and-forth during reviews.

Many aspects of esp-stub-lib (ROM usage, weak/strong overrides, SOC headers)
are not obvious from the code alone. Without clear conventions, reviews tend to
focus on structure and style instead of the actual feature.

These guidelines aim to:
- keep the codebase consistent
- make contributions easier
- allow reviews to focus on correctness and design

If something is unclear or missing, feel free to open a PR to improve this document.

---

## Table of Contents

- [Contributing to esp-stub-lib](#contributing-to-esp-stub-lib)
  - [Philosophy](#philosophy)
  - [Table of Contents](#table-of-contents)
  - [Naming](#naming)
  - [Don't Reinvent, Reuse](#dont-reinvent-reuse)
  - [Weak/Strong Overrides](#weakstrong-overrides)
  - [Error Handling](#error-handling)
  - [ROM Functions](#rom-functions)
  - [Memory](#memory)
  - [Logging](#logging)
  - [SOC Headers](#soc-headers)
    - [`soc_caps.h` conventions](#soc_capsh-conventions)
  - [CMake](#cmake)
    - [Per-target CMakeLists.txt](#per-target-cmakeliststxt)
    - [Symbol override](#symbol-override)
  - [Adding a New Chip](#adding-a-new-chip)
    - [SPI register naming](#spi-register-naming)
    - [ROM function signature mismatches](#rom-function-signature-mismatches)
  - [Testing](#testing)
  - [Code Style](#code-style)
    - [License header](#license-header)
    - [Types](#types)
    - [Statics](#statics)
  - [Review Checklist](#review-checklist)
    - [General](#general)
    - [Weak/Strong](#weakstrong)
    - [ROM and Linking](#rom-and-linking)
    - [Errors](#errors)
    - [Build](#build)

---

## Naming

We use `snake_case` everywhere. Functions, variables, types. Macros and constants
are `UPPER_SNAKE_CASE`. Types end with `_t`.

**Public API** functions start with `stub_lib_`:

```c
stub_lib_flash_init()
stub_lib_delay_us()
stub_lib_clock_init()
```

Public types: `stub_lib_*_t` (e.g. `stub_lib_flash_config_t`).

**Target interface** functions start with `stub_target_`:

```c
stub_target_flash_init()
stub_target_clock_init()
```

These live in `src/target/base/include/target/` headers and get implemented as
weak defaults in `src/target/common/src/`, with optional strong overrides in
`src/target/<chip>/src/`.

Client code (`stub_main.c`, etc.) must only call `stub_lib_*` functions.
`stub_target_*` is internal. Don't call it from outside the library.

**Types in signatures.** If a named type or enum exists for a parameter, use it.
Don't fall back to raw `int` or `uint8_t` when a proper type is available.

**File scoped helpers.** Mark them `static`. No prefix needed.

---

## Don't Reinvent, Reuse

Check what already exists before writing new code. A lot of review comments boil
down to "we already have that."

| What you need | What to use | Not this |
|---------------|-------------|----------|
| Register access | `REG_READ()`, `REG_WRITE()` etc. from `<esp-stub-lib/soc_utils.h>` | Local `#define REG_READ(...)` |
| Bit ops | `BIT()`, `IS_ALIGNED()` from `<esp-stub-lib/bit_utils.h>` | `(1 << n)`, `(1U << n)` |
| Delays | `stub_lib_delay_us()` from `<esp-stub-lib/rom_wrappers.h>` | Raw `ets_delay_us()` |
| CRC | `stub_lib_crc16_le()` from `<esp-stub-lib/rom_wrappers.h>` | Raw `crc16_le()` |

If a wrapper or macro exists, use it. Local copies drift and cause conflicts.

**Register definitions go in SOC headers**, not in `.c` files. Put base addresses,
offsets, and bit fields under `src/target/<chip>/include/soc/` in a proper header
(e.g. `spi_reg.h`, `gpio_reg.h`). Even if only one file needs it today, a header
keeps things discoverable. Always check if the definitions you need already exist
before creating new ones.

```c
/* wrong: hardcoded in a .c file */
#define SPI2_BASE   0x60024000

/* right: in soc/spi_reg.h */
#include <soc/spi_reg.h>
```

**Target headers are internal.** External code (esp-flasher-stub, esptool) must only
include from `include/esp-stub-lib/`. Never expose `target/*.h` in a public API.

---

## Weak/Strong Overrides

This is the core pattern. Get familiar with it before contributing.

`src/target/common/src/` has weak defaults:

```c
bool __attribute__((weak)) stub_target_flash_needs_attach(void)
{
    return true;
}
```

`src/target/<chip>/src/` has strong overrides when needed:

```c
bool stub_target_flash_needs_attach(void)
{
    return (READ_PERI_REG(SPI_MEM_CACHE_FCTRL_REG(0)) & SPI_MEM_CACHE_FLASH_USR_CMD) == 0;
}
```

Every `stub_target_*` function must have a weak default in `common/src/`, even if
the body is empty. Add the weak default first, then chip overrides as needed.

The build uses `-Wl,--whole-archive` on the target library so strong symbols win.
Don't remove that flag.

> **Important:** Don't use target specific `#ifdef`s in shared code. If behavior
> differs per chip, write a strong override in the target directory.

---

## Error Handling

Defined in `include/esp-stub-lib/err.h`:

```c
#define STUB_LIB_OK                        0
#define STUB_LIB_FAIL                      STUB_LIB_ERROR_BASE
#define STUB_LIB_ERR_UNKNOWN_FLASH_ID      (STUB_LIB_ERROR_BASE + 0x1)
```

Public API (`stub_lib_*`) functions.

- Return `STUB_LIB_OK` on success, a specific `STUB_LIB_ERR_*` on failure.
  No bare `-1` or `1`.
- Log with `STUB_LOGE` before returning an error.
- Validate inputs at the top (NULL checks, alignment). Use `STUB_LIB_ERR_INVALID_ARG`.
- If a function can't fail, make it `void`.

Internal functions don't need `STUB_LIB_ERR_*`, but still avoid bare magic numbers.
Use local `#define`s or `enum`s within the source file. No need to add them to `err.h`.

---

## ROM Functions

ROM symbols come from linker scripts, not headers. Declare them `extern` in either of these places.

1. A **private header** (`src/target/base/include/private/`) when shared across files.
2. **Locally in the `.c` file** when used in one place only.

```c
extern int  esp_rom_spiflash_erase_chip(void);
extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);
```

Don't put ROM externs in public headers (`include/esp-stub-lib/`).

Check names and signatures against the chip's ROM linker script
(`src/target/<chip>/ld/<chip>.rom.ld`). They differ between chips.

If the same ROM function is declared in multiple files, the signatures must match.
LTO catches mismatches. When a ROM function has different signatures per chip, wrap
it behind a `stub_target_*` function instead of having conflicting externs.

ROM functions can be aliased in `<chip>.rom.api.ld` to give them consistent,
readable names across chips.

```
PROVIDE ( esp_rom_spiflash_set_drvs = SetSpiDrvs );
PROVIDE ( esp_rom_spiflash_common_cmd = SPI_Common_Command );
```

This is optional. If an alias improves readability, go ahead. Keep in mind that
renaming makes it harder to grep for the original symbol when inspecting ROM source.

---

## Memory

No heap. The stub runs in limited target RAM.

- No `malloc`, `free`, `calloc`, `realloc`.
- Use stack variables for temporaries.
- Avoid large `static` variables. The library serves both OpenOCD and esptool, and
  they have different needs. Modules that need persistent state expose a
  `stub_lib_<module>_state_size()` query so the caller sizes a buffer at runtime
  and passes it in via a `void *state` parameter. A size of `0` means "no state
  needed" and the caller may pass `NULL`. Callers own the buffer's lifetime and
  alignment (at least `sizeof(uint32_t)`); the library only reads/writes through
  the pointer.

  ```c
  size_t sz = stub_lib_cache_state_size();
  uint8_t __attribute__((aligned(sizeof(uint32_t)))) buf[sz ? sz : 1];
  void *state = sz ? buf : NULL;
  stub_lib_cache_init(state);
  ```

---

## Logging

Compile-time conditional (`STUB_LOG_ENABLED`). Use the macros:

```c
STUB_LOGE("Something broke: %d\n", err);   // Error
STUB_LOGW("Unexpected value: %d\n", val);   // Warning
STUB_LOGI("Size: %u\n", size);              // Info
STUB_LOGD("Reg = 0x%08x\n", reg);           // Debug
STUB_LOG_TRACE();                           // Function entry (trace level)
```

End format strings with `\n`. Use `STUB_LOGE` before returning errors.
Keep register dumps at debug level, not info. Never use `printf` directly.

---

## SOC Headers

Each target has hardware headers under `src/target/<chip>/include/soc/`:

| Header | What it has |
|--------|-------------|
| `soc.h` | Memory map (`SOC_IROM_LOW`, `SOC_DRAM_HIGH`, ...) |
| `soc_caps.h` | Feature flags (`SOC_RTC_FAST_MEM_SUPPORTED`, ...) |
| `reg_base.h` | Peripheral base addresses (`DR_REG_*_BASE`) |
| `spi_mem_compat.h` | Abstraction for SPI register naming differences |

Extract from ESP-IDF (`components/soc/<target>/`). If a full register file is
broadly useful, it's fine to copy it. You can adapt the header and footer to fit
the project (replace include guards with `#pragma once`, add required includes, fix
license header). But if you only need a few defines for a specific purpose, keep it
minimal rather than pulling in the whole file.

Use `spi_mem_compat.h` to hide naming differences between chips
(`SPI_MEM_` vs `SPI1_MEM_` etc.).

### `soc_caps.h` conventions

This file defines what a chip supports. It is used in `common/src/` to conditionally
compile code that not all chips have.

Naming.
- Boolean flags. `SOC_<SUBSYSTEM>_<FEATURE>_SUPPORTED` with value `1`.
  Examples. `SOC_RTC_FAST_MEM_SUPPORTED`, `SOC_MEM_TCM_SUPPORTED`.
- Numeric values. `SOC_<SUBSYSTEM>_<PROPERTY>` with a count in parentheses.
  Example. `SOC_UART_HP_NUM (3)`.

Only add defines that the stub code actually checks. Don't copy the full
`soc_caps.h` from ESP-IDF.

When you add a new capability flag, also add the corresponding `#if` guard in the
common code that uses it.

```c
#if defined(SOC_RTC_FAST_MEM_SUPPORTED) && SOC_RTC_FAST_MEM_SUPPORTED
    /* RTC-specific code here */
#endif
```

Always use `#if defined(X) && X`, not just `#ifdef X`. This avoids surprises if
someone defines the flag to `0`.

---

## CMake

### Per-target CMakeLists.txt

```cmake
set(srcs
    src/flash.c
    src/clock.c
    src/uart.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

target_include_directories(${ESP_TARGET_LIB}
    PUBLIC include
)

target_link_options(${ESP_TARGET_LIB}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/<chip>.rom.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/<chip>.rom.api.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/<chip>.rom.libc.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/<chip>.rom.libgcc.ld
)
```

### Symbol override

`src/target/CMakeLists.txt` links the target library with `--whole-archive`:

```cmake
target_link_libraries(${COMMON_LIB}
    PUBLIC base
    PRIVATE -Wl,--whole-archive ${ESP_TARGET_LIB} -Wl,--no-whole-archive
)
```

This makes strong symbols in target code override weak ones in common. Don't touch
these flags.

---

## Adding a New Chip

```
src/target/<chip>/
├── CMakeLists.txt
├── include/
│   ├── esp_rom_caps.h
│   └── soc/
│       ├── soc.h
│       ├── soc_caps.h
│       ├── reg_base.h
│       ├── spi_mem_compat.h
│       ├── spi_mem_reg.h       ← may be split: spi_mem_c_reg.h + spi1_mem_c_reg.h on some chips
│       ├── uart_reg.h
│       └── io_mux_reg.h
├── ld/
│   ├── <chip>.rom.ld
│   ├── <chip>.rom.api.ld
│   ├── <chip>.rom.libc.ld
│   └── <chip>.rom.libgcc.ld
└── src/
    ├── flash.c
    ├── clock.c
    └── uart.c
```

1. Read ESP-IDF sources first (`components/soc/<chip>/register/soc/`,
   `components/hal/<chip>/include/hal/`).
2. Create the directory tree.
3. Pull SOC headers from ESP-IDF. Only what you need.
4. Pull `esp_rom_caps.h` from ESP-IDF (`components/esp_rom/<chip>/`). If a capability is
   not yet supported, set it to `-1` as a placeholder to unblock the build, and add a
   `/* TODO */` comment.
5. Pull ROM linker scripts from ESP-IDF
   (`components/esp_rom/<chip>/ld/`). Check that function names match.
6. Write overrides. Start from the closest existing chip:
   - RISC-V single-core (C6, H2, H21, C61): start from `esp32c6`
   - RISC-V multi-core (P4, H4, S31): start from `esp32p4`
7. Write `CMakeLists.txt`.
8. Build with `-DESP_TARGET=<chip>` and fix any remaining errors. It is fine to start
   with only `flash.c` and `uart.c` to get a working build, and add other overrides
   incrementally.

### SPI register naming

On some chips (e.g. esp32s31) the SPI memory register headers are split or use a
different naming scheme (`SPI1_MEM_C_*` instead of `SPI_MEM_*`). Use `spi_mem_compat.h`
to map the generic names the common code uses to the chip-specific register names:

```c
/* spi_mem_compat.h — esp32s31 example */
#define SPI_MEM_CMD_REG(n)   SPI1_MEM_C_CMD_REG
#define SPI_MEM_MST_ST       SPI1_MEM_C_MST_ST
```

If the chip splits the register file (e.g. `spi_mem_c_reg.h` and `spi1_mem_c_reg.h`),
include both in `spi_mem_compat.h` so callers only need one include.

### ROM function signature mismatches

ROM functions sometimes take fewer parameters than the `stub_target_*` interface
expects. Write a strong override that accepts the full interface signature and silences
the unused parameter with `(void)`:

```c
/* ROM on this chip only takes uart_no — clock is baked in */
void stub_target_rom_uart_init(uint8_t uart_no, uint32_t clock)
{
    (void)clock;
    esp_rom_uart_init(uart_no);
}
```

Add a short comment explaining why the parameter is ignored, so reviewers don't flag
it as a bug.

---

## Testing

There is no host-side test framework yet. The `example/stub_main.c` file serves as
a build-time smoke test. It calls every public API function to make sure the library
links correctly for all targets.

When you add a new public API, add a dummy call to it in `example/stub_main.c`.
This ensures the function is reachable and the symbol resolves at link time.

---

## Code Style

Enforced by `.clang-format` (LLVM-based):

- 4 spaces, no tabs
- 120 character line limit
- Right aligned pointers. `int *ptr`
- Opening brace on new line for functions, same line for `if`/`for`/`while`
- Always use braces. No single line `if` bodies
- Include sorting is automatic (see README for full grouping rules)
- Public headers are included as `<esp-stub-lib/flash.h>`, not `<flash.h>` or
  `"flash.h"`. Internal headers use `<target/...>`, `<private/...>`, `<soc/...>`.
  Quoted includes (`"header.h"`) are only for local headers in the same directory
- `#pragma once` for header guards
- Use library macros (`MIN`, `MAX`, `BIT`, `IS_ALIGNED`, `REG_READ`, etc.). Don't
  rewrite them or use bare bit shifts
- Avoid unnecessary casts. Cast when the compiler requires it (e.g. narrowing
  assignments with `-Wconversion`), not "just in case"
- Drop unused includes

### License header

New files:

```c
/*
 * SPDX-FileCopyrightText: <year> Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
```

Files copied from ESP-IDF. Pre-commit will fix the license and copyright header
automatically.

### Types

- Public: `stub_lib_*_t`
- Internal: `<chip>_*_t` (e.g. `esp32c6_flash_state_t`)
- Enums: always typedef'd with `_t`

### Statics

Use the `s_` prefix when you need a `static`:

```c
static uint32_t s_flash_id;
```

But prefer parameters over statics. See [Memory](#memory).

---

## Review Checklist

For reviewers and contributors.

### General

- [ ] No `malloc`/`free`
- [ ] No `printf`. Use `STUB_LOG*` macros
- [ ] No magic constants. Use named defines, `BIT()`, `MIN()`, `MAX()`
- [ ] No redefined library macros. Use `soc_utils.h` and `bit_utils.h`
- [ ] No hardcoded register addresses in `.c` files. Put them in `soc/*.h`
- [ ] Library wrappers used instead of raw ROM calls
- [ ] No ROM externs in public headers
- [ ] Target headers not leaked into public API
- [ ] `#pragma once` on all headers
- [ ] No unused includes
- [ ] No unnecessary casts (cast only when the compiler requires it)
- [ ] License header on all new files; copied files keep original years/license
- [ ] Angle bracket includes for project headers, quoted includes only for same directory
- [ ] Large state passed via parameters, not in statics
- [ ] Named types used in signatures where available
- [ ] Client code calls `stub_lib_*` only, not `stub_target_*`

### Weak/Strong

- [ ] New `stub_target_*` functions have a weak default in `common/src/`
- [ ] Target overrides are plain functions. No `__attribute__((weak))`

### ROM and Linking

- [ ] ROM extern signatures match across all files (LTO-safe)
- [ ] Alignment checks on hardware-facing read/write ops

### Errors

- [ ] Public functions return `STUB_LIB_OK` / `STUB_LIB_ERR_*`
- [ ] `STUB_LOGE` before returning errors
- [ ] Inputs validated at function entry

### Build

- [ ] Sources listed in target's `CMakeLists.txt`
- [ ] ROM linker scripts linked via `target_link_options`
- [ ] Tested with `-DESP_TARGET=<chip>`
