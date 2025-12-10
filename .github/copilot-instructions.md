# Copilot Instructions for esp-stub-lib

## Repository Overview

**esp-stub-lib** is an experimental C library for creating flash stubs that can be loaded onto Espressif ESP chips. The repository is ~5MB in size and contains a three-layer architecture designed to maximize code reuse across multiple ESP chip targets while eliminating circular dependencies.

**Language/Tools:** C (C17), CMake, Ninja, Python, pre-commit hooks
**Supported Targets:** ESP8266, ESP32, ESP32-S2, ESP32-S3, ESP32-C2, ESP32-C3, ESP32-C5, ESP32-C6, ESP32-C61, ESP32-H2, ESP32-H21, ESP32-H4, ESP32-P4

## Three-Layer Architecture

The library uses a strict dependency flow to avoid circular dependencies:

```
Public API (include/esp-stub-lib/) ← Library clients use this
    ↓
Implementation (src/)
    ↓
Common (src/target/common/) - Weak generic implementations
    ↓
Target (src/target/esp32*/) - Strong target-specific overrides
    ↓
Base (src/target/base/) - Interface headers only
```

**Key Directories:**
- `include/esp-stub-lib/` - Public API headers (flash.h, log.h, mem_utils.h, uart.h, etc.) - **Only these should be used by library clients**
- `src/` - Top-level implementation layer (flash.c, mem_utils.c, uart.c, etc.)
- `src/target/base/include/` - Internal interface headers split into:
  - `target/` - Internal API between common/target layers
  - `private/` - Internal ROM/hardware details
- `src/target/common/` - Generic weak function implementations using SOC_* macros
- `src/target/esp32*/` - Target-specific directories containing:
  - `include/soc/soc.h` - SOC_* macro definitions
  - `src/` - Strong function overrides (optional)
  - `ld/` - Linker scripts for ROM symbols
  - `CMakeLists.txt` - Target build configuration

## Root Directory Structure

```
.astyle-rules.yml           # Code formatting rules for astyle
.check_copyright_config.yaml # Copyright header validation config
.codespellrc                # Spell check configuration
.github/workflows/          # CI workflows (build_example.yml, dangerjs.yml, jira.yml)
.gitignore                  # Git ignore patterns
.pre-commit-config.yaml     # Pre-commit hook configuration
CHANGELOG.md                # Version history
CMakeLists.txt              # Main library build file
LICENSE-APACHE, LICENSE-MIT # Dual license
README.md                   # Main documentation
example/                    # Complete example implementation
include/                    # Public API headers
pyproject.toml              # Python tool configuration (yamlfix, commitizen, codespell)
src/                        # Implementation sources
```

## Build System & Toolchains

### Prerequisites

**IMPORTANT:** Building requires ESP-IDF toolchains to be installed and available in PATH. The build system expects target-specific cross-compilers:

- **Xtensa targets** (esp32, esp32s2, esp32s3): `xtensa-{target}-elf-gcc`
- **RISC-V targets** (esp32c2, esp32c3, esp32c5, esp32c6, esp32c61, esp32h2, esp32h21, esp32h4, esp32p4): `riscv32-esp-elf-gcc`
- **ESP8266**: `xtensa-lx106-elf-gcc` (auto-downloaded by build.sh)

**Setup:** Follow https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html to install ESP-IDF and export the environment (`. $IDF_PATH/export.sh`).

### Building the Example

The example directory contains a complete stub implementation demonstrating the library usage.

**Using build script (recommended):**
```bash
cd example
./build.sh all              # Build all targets
./build.sh esp32            # Build specific target
./build.sh clean            # Clean all builds
```

**Manual build:**
```bash
cd example
mkdir -p build && cd build
cmake -DESP_TARGET=esp32 -GNinja ..  # Replace esp32 with your target
ninja
```

**Build outputs:** After successful build, find in `example/build/`:
- `stub_{target}.elf` - Compiled binary
- `stub_{target}.map` - Memory map
- `stub_{target}.asm` - Disassembly

**Build time:** Initial builds take 30-60 seconds per target depending on toolchain availability.

**ESP8266 special case:** ESP8266 uses an older toolchain. Use `./build.sh esp8266` which auto-downloads the toolchain to `example/toolchain/` directory. Manual builds require setting PATH to include the toolchain.

### Library Build Integration

To use the library in your project:
1. Set `ESP_TARGET` CMake variable to your target chip
2. Add the library directory: `add_subdirectory(path/to/esp-stub-lib)`
3. Link against `esp-stub-lib`: `target_link_libraries(your_target PRIVATE esp-stub-lib)`
4. Include public headers: `#include <esp-stub-lib/flash.h>`

## Code Quality & Validation

### Pre-commit Hooks (ALWAYS RUN BEFORE COMMITTING)

**Setup (required once):**
```bash
pip install pre-commit
pre-commit install -t pre-commit -t commit-msg
```

**Pre-commit checks (run automatically on commit, or manually):**
```bash
pre-commit run --all-files  # Takes 2-3 minutes on first run (installs hooks)
```

**Hooks enforced:**
1. `codespell` - Spell checking (ignores "dout", skips build/)
2. `check-copyright` - Validates copyright headers (Apache-2.0 OR MIT)
3. `trailing-whitespace` - Removes trailing whitespace
4. `end-of-file-fixer` - Ensures files end with newline
5. `check-executables-have-shebangs` - Validates script shebangs
6. `mixed-line-ending` - Enforces LF line endings
7. `double-quote-string-fixer` - Converts double quotes to single quotes (Python)
8. `yamlfix` - Formats YAML files (line length 120, preserves quotes)
9. `conventional-precommit-linter` - Validates commit messages (conventional commits format)
10. `astyle_py` - C code formatting (astyle 3.4.7, OTBS style, 4-space indent)

**Code formatting style (astyle):**
- Style: One True Brace Style (OTBS)
- Indent: 4 spaces, tabs converted to spaces
- Max line length: 120 characters
- Pointer/reference alignment: to name (`int* ptr`, not `int *ptr`)
- Padding: operators, commas, headers

**Copyright headers:** All new .c and .h files must include:
```c
/*
 * SPDX-FileCopyrightText: {years} Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
```

**Ignored for copyright:** Linker scripts (*.ld files in src/target/*/ld/ and example/ld/)

### Commit Message Format

Follow Conventional Commits format (enforced by pre-commit hook):
```
type(scope): subject

body (optional)
```

**Common types:** feat, fix, docs, style, refactor, test, chore, change (for version bumps)

## GitHub Actions CI

**Workflows:**
1. **build_example.yml** - Builds example for all targets on PR/push to non-master branches
   - Builds all ESP-IDF targets (esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c5, esp32c6, esp32c61, esp32h2, esp32h21, esp32h4, esp32p4) using `espressif/esp-idf-ci-action@v1` with latest ESP-IDF
   - Builds ESP8266 separately using `./build.sh esp8266` (requires custom toolchain)
   - Uploads build artifacts (.elf, .map, .asm files)
   - **Failure means:** Build errors, missing toolchain, or incorrect CMake configuration

2. **dangerjs.yml** - PR style linting (runs on PR open/edit/sync)

3. **jira.yml** - Syncs issues/PRs to Jira (internal Espressif workflow)

**External CI Services:**
- **pre-commit.ci** - Runs pre-commit hooks on every push (https://results.pre-commit.ci/badge/github/espressif/esp-stub-lib/master)

**To replicate CI locally:**
```bash
# Pre-commit checks
pre-commit run --all-files

# Build all targets like CI
cd example
./build.sh all
```

## Common Patterns & Conventions

### Function Naming

- **Public API:** `stub_lib_{module}_{action}()` - e.g., `stub_lib_flash_init()`, `stub_lib_uart_tx()`
- **Internal API:** `stub_target_{module}_{action}()` - e.g., `stub_target_flash_init()`, `stub_target_uart_tx()`
- **ROM init functions:** Functions containing `rominit` (e.g., `stub_target_uart_rominit_intr_attach()`) are simplified implementations that can only be called after the ROM download mode has been entered, as they expect some initialization to have been done by ROM. Do not call these functions in other contexts.

### Adding New Functionality

1. **Add public header** to `include/esp-stub-lib/{module}.h`
2. **Add implementation** to `src/{module}.c`
3. **Add common weak implementation** to `src/target/common/src/{module}.c` using SOC_* macros
4. **Add internal interface** to `src/target/base/include/target/{module}.h`
5. **Add target overrides** (if needed) to `src/target/{target}/src/{module}.c` with strong symbols
6. **Update CMakeLists.txt** files as needed to include new sources

### Weak vs Strong Symbols

The common layer provides **weak** implementations that work generically. Target-specific directories provide **strong** implementations to override when hardware details differ. The linker automatically prefers strong symbols.

## Important Notes

- **Experimental status:** This library is not production-ready (stated in README)
- **No tests:** There is no test infrastructure in this repository
- **Build artifacts:** Use `.gitignore` to exclude `build/`, `toolchain/`, and temporary files
- **Documentation:** Update README.md if adding significant new features
- **License:** All contributions are dual-licensed Apache-2.0 OR MIT
- **Target-specific constants:** Defined in `src/target/{target}/include/soc/soc.h` as SOC_* macros (e.g., SOC_UART0_BASE)

## Troubleshooting

### Build failures
- **"ESP_TARGET not defined"** - Add `-DESP_TARGET=esp32` to cmake command
- **"xtensa-esp32-elf-gcc: not found"** - Install and source ESP-IDF environment
- **"Could not find toolchain"** - Ensure ESP-IDF toolchains are in PATH (run `. $IDF_PATH/export.sh`)
- **ESP8266 build fails** - Use `./build.sh esp8266` which handles toolchain setup

### Pre-commit failures
- **astyle errors** - Run `pre-commit run astyle_py --all-files` to auto-fix formatting
- **Copyright errors** - Add proper SPDX header to new files
- **Commit message errors** - Use conventional commits format: `type(scope): description`
- **Codespell errors** - Fix typos or add to `.codespellrc` ignore list if intentional

## Quick Reference

**First-time setup:**
```bash
# Install ESP-IDF and toolchains (see ESP-IDF docs)
# Then in this repository:
pip install pre-commit
pre-commit install -t pre-commit -t commit-msg
```

**Before committing:**
```bash
pre-commit run --all-files     # Auto-runs on commit, but good to check first
```

**Build and test:**
```bash
cd example
./build.sh all                  # Or specify target: ./build.sh esp32
```

**Release process (maintainers only):**
```bash
pip install commitizen
git fetch
git checkout -b update/release_vX.Y.Z
git reset --hard origin/master
cz bump
git push -u && git push --tags
# Then create PR and edit draft release notes
```

---

**Trust these instructions.** Only search for additional information if these instructions are incomplete or incorrect. The build process, architecture, and validation steps documented here have been verified.
