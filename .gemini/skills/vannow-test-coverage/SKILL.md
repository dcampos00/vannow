---
name: vannow-test-coverage
description: Compiles, runs, and generates code coverage statistics for the VanNOW C++ firmware (Central and Remote nodes) using native PlatformIO unit tests and manual GCC/Gcov compilation.
---

# VanNOW Firmware Tests and Code Coverage

Use this skill when you or the user needs to run unit tests, check test status, troubleshoot compilation errors in native mock targets, or measure/generate code coverage reports for the `central` and `remote` firmware projects.

---

## 1. Directory Structure

The firmware is written in C++ and organized as PlatformIO projects inside the `firmware/` directory:

- [central/](file:///home/daniel/Projects/vannow/firmware/central): Contains the main system controller firmware, handling 11 power output channels.
- [remote/](file:///home/daniel/Projects/vannow/firmware/remote): Contains the button, encoder, and power management logic for the wireless wall panel remotes.
- [lib/](file:///home/daniel/Projects/vannow/firmware/lib): Contains shared libraries:
  - `arduino_mock`: Fake Arduino runtime to run test suites on native desktop/host environments.
  - `protocol`: Defines the ESP-NOW payload structures.
  - `wireless`: Driver wrapper logic (excluded in native target tests).

---

## 2. Running Unit Tests via PlatformIO

Both the `central` and `remote` projects have a `native` environment in their respective `platformio.ini` files. This compiles and executes tests locally on the host machine.

### Central Firmware Tests
To run tests for the central module:
```bash
cd firmware/central
~/.platformio/penv/bin/pio test -e native
```

### Remote Firmware Tests
To run tests for the remote module:
```bash
cd firmware/remote
~/.platformio/penv/bin/pio test -e native
```

### Compilation Troubleshooting
If the native unit test execution in the `remote` folder fails to compile with errors like `fatal error: WirelessManager.h: No such file or directory`, make sure `RemoteSender.cpp` is excluded from the compilation filter in native mode.

Ensure the `src_filter` in [remote/platformio.ini](file:///home/daniel/Projects/vannow/firmware/remote/platformio.ini) matches:
```ini
[env:native]
...
src_filter = +<*> -<main.cpp> -<RemoteSender.cpp>
```
*(Since `RemoteSender.cpp` depends on the hardware-specific Wi-Fi stack in `lib/wireless`, it cannot be built natively and must be bypassed in native tests.)*

---

## 3. Measuring Code Coverage

To calculate statement (line) coverage on host builds:

### Automated Script (Recommended)
You can run the pre-configured script from anywhere in the workspace:
```bash
./.gemini/skills/vannow-test-coverage/scripts/run_coverage.py
```
This script compiles both projects with `--coverage` flags, executes their tests, extracts the line-by-line coverage counts using `gcov`, formats them into a markdown table, and automatically cleans up binary artifacts.

### Manual Coverage Build Workflow
If you need to perform the compilation and calculation steps manually:

#### 1. Compile Central Tests with Coverage
```bash
cd firmware/central
g++ -O0 -g --coverage \
  -I src -I test/test_channels -I ../lib/arduino_mock/src -I ../lib/protocol -I .pio/libdeps/native/Unity/src \
  src/Channel.cpp src/DigitalChannel.cpp src/DimmableChannel.cpp src/SystemController.cpp \
  ../lib/arduino_mock/src/Arduino.cpp .pio/libdeps/native/Unity/src/unity.c \
  test/test_channels/test_channels.cpp -o test_channels_cov
```

#### 2. Run Central Tests & Generate Data
```bash
./test_channels_cov
gcov test_channels_cov-Channel.gcda test_channels_cov-DigitalChannel.gcda test_channels_cov-DimmableChannel.gcda test_channels_cov-SystemController.gcda
```

#### 3. Compile Remote Tests with Coverage
```bash
cd ../remote
g++ -O0 -g --coverage \
  -I src -I test/test_remote_handlers -I ../lib/arduino_mock/src -I ../lib/protocol -I .pio/libdeps/native/Unity/src \
  src/ButtonHandler.cpp src/EncoderHandler.cpp src/PowerManager.cpp \
  ../lib/arduino_mock/src/Arduino.cpp .pio/libdeps/native/Unity/src/unity.c \
  test/test_remote_handlers/test_remote_handlers.cpp -o test_remote_handlers_cov
```

#### 4. Run Remote Tests & Generate Data
```bash
./test_remote_handlers_cov
gcov test_remote_handlers_cov-ButtonHandler.gcda test_remote_handlers_cov-EncoderHandler.gcda test_remote_handlers_cov-PowerManager.gcda
```

---

## 4. Hardware/SDK Exclusions
The following components cannot be run in host native test targets and must be verified in system/integration tests on actual target microcontrollers:
1. **`lib/wireless/src/WirelessManager.cpp`**: Employs ESP32 SDK functions (`esp_now_init`, `esp_now_send`, etc.) to setup Wi-Fi station mode.
2. **`remote/src/RemoteSender.cpp`**: Uses the `WirelessManager` library to transmit actions over ESP-NOW.
3. **`main.cpp` entry points**: Boots the hardware, configures pins, registers interrupts, and handles low-level setup.
