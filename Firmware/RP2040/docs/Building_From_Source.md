# Building from source

Step-by-step guide to **clone** this repository, initialize **all git submodules**, install the **required tools**, and **compile** OGX-Mini firmware on your machine.

For day-to-day board options and button combos, see the [main README](../../../README.md). Documentation index: [README.md](README.md) in this folder.

---

## What you will build

The primary firmware lives under **`Firmware/RP2040`**. It targets Raspberry Pi Pico / Pico 2 / Pico W / Pico 2 W and related RP2040/RP2350 boards. Output is a **`.uf2`** (and `.elf`) you copy to the board in BOOTSEL mode.

Optional / advanced:

| Target | When |
|--------|------|
| **ESP32 companion** (`Firmware/ESP32_*`, BlueRetro hybrid) | Only if you use an ESP32 + RP2040 hybrid board — needs **ESP-IDF** (see [ESP32](#esp32-companion-firmware-optional) below) |
| **WebApp** submodule | Hosted separately; not required to compile firmware |

---

## 1. Required applications (RP2040 / RP2350 firmware)

Install **all** of the following and ensure they are on your **PATH** (open a new terminal after installing).

| Tool | Why it is required | Check |
|------|--------------------|--------|
| **Git** | Clone the repo and initialize submodules | `git --version` |
| **Python 3** (3.8+) | CMake runs Python for BTstack GATT header generation and related scripts | `python3 --version` (Windows: `python` or `py`) |
| **CMake** (3.13+) | Configures the Pico SDK / TinyUSB / Bluepad32 build | `cmake --version` |
| **Ninja** | Build generator used by the project scripts (`-G Ninja`) | `ninja --version` |
| **GNU Arm Embedded Toolchain** (`arm-none-eabi-gcc`) | Cross-compiles for Cortex-M0+/M33 | `arm-none-eabi-gcc --version` |

**Also required at build time (not always a separate “app”):**

| Dependency | Notes |
|------------|--------|
| **Network access** (first configure) | CMake may fetch **picotool** / SDK helpers; submodules and pico-sdk are cloned from GitHub |
| **Raspberry Pi Pico SDK 2.1.0** | Must exist as `Firmware/external/pico-sdk` **or** via `PICO_SDK_PATH` — see [§3](#3-pico-sdk-required-not-a-git-submodule) |
| Disk space | Roughly **1–3+ GB** after submodules + SDK + build tree |

Optional but useful:

| Tool | Purpose |
|------|---------|
| USB serial terminal (`minicom`, `screen`, PuTTY, …) | Reading **Debug** UART logs |
| VS Code + CMake Tools | IDE builds (same CMake options) |

### Install commands by OS

#### Debian / Ubuntu

```bash
sudo apt update
sudo apt install -y git python3 cmake ninja-build gcc-arm-none-eabi
```

On some releases you may also want build helpers used by nested tools:

```bash
sudo apt install -y build-essential libnewlib-arm-none-eabi
```

#### Fedora

```bash
sudo dnf install -y git python3 cmake ninja gcc-arm-none-eabi-toolchain
```

#### Arch Linux

```bash
sudo pacman -S git python cmake ninja arm-none-eabi-gcc arm-none-eabi-newlib
```

#### macOS (Homebrew)

```bash
brew install git python cmake ninja arm-none-eabi-gcc
```

#### Windows

1. Install **[Git for Windows](https://git-scm.com/download/win)** (includes Git Bash).
2. Install tools via **[Chocolatey](https://chocolatey.org/)** (Admin PowerShell), **or** install each package manually:

```powershell
choco install git python cmake ninja gcc-arm-embedded -y
```

3. Confirm `arm-none-eabi-gcc` is on **PATH**. If Chocolatey’s ARM package is missing or incomplete, install **[Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)** (AArch32 bare-metal) and add its `bin` folder to PATH.
4. Open a **new** PowerShell window and verify:

```powershell
git --version
python --version
cmake --version
ninja --version
arm-none-eabi-gcc --version
```

The interactive script `scripts/build.ps1` checks the same tools and prints these hints if anything is missing.

---

## 2. Clone the repository and all submodules

### Fresh clone (recommended)

```bash
git clone --recursive https://github.com/MegaCadeDev/OGX-Mini-2026.git
cd OGX-Mini-2026
```

`--recursive` initializes **top-level** submodules and their nested submodules (important for **Bluepad32 → BTstack**).

### If you already cloned without `--recursive`

```bash
cd OGX-Mini-2026
git submodule update --init --recursive
```

### After `git pull` (keep submodules in sync)

```bash
git pull
git submodule update --init --recursive
```

### What the submodules are

Defined in `.gitmodules` (among others):

| Path | Role |
|------|------|
| `Firmware/external/bluepad32` | Bluetooth stack (includes nested **btstack**) |
| `Firmware/external/tinyusb` | USB device/host |
| `Firmware/external/Pico-PIO-USB` | PIO USB host (Pico W host port, etc.) |
| `Firmware/external/libfixmath` | Fixed-point math |
| `Firmware/ESP32_Blueretro` | BlueRetro (ESP32 hybrid builds) |
| `WebApp` | Web configuration UI sources |
| `Tools/dump-dvd-kit` | Xbox DVD kit helper (not needed for normal firmware) |

CMake also runs `git submodule update --init --recursive` for **bluepad32** and **tinyusb** during configure, but you should still initialize **everything** yourself so PIO-USB, libfixmath, and nested BTstack are present before the first build.

**Verify** nested BTstack exists:

```bash
ls Firmware/external/bluepad32/external/btstack/tool/compile_gatt.py
```

If that file is missing, re-run `git submodule update --init --recursive`.

---

## 3. Pico SDK (required — not a git submodule)

`Firmware/external/pico-sdk` is **gitignored**. The automatic CMake cloners (`get_pico_sdk`) is currently **commented out**, so a clean clone **will not** compile until the SDK is present.

This project expects **Pico SDK tag `2.1.0`**.

### Option A — Clone into the repo (matches default `PICO_SDK_PATH`)

```bash
cd Firmware/external
git clone --branch 2.1.0 --depth 1 https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init --recursive
cd ../../..
```

### Option B — Use an existing SDK install

```bash
# Linux / macOS
export PICO_SDK_PATH=/path/to/pico-sdk

# Windows PowerShell
$env:PICO_SDK_PATH = "C:\path\to\pico-sdk"
```

Point at a checkout of **2.1.0** (or a tree this firmware is known to build with). Nested SDK submodules must be initialized.

### Confirm

```bash
test -f Firmware/external/pico-sdk/pico_sdk_init.cmake && echo "SDK OK" || echo "SDK missing — clone or set PICO_SDK_PATH"
# Windows PowerShell:
# Test-Path Firmware\external\pico-sdk\pico_sdk_init.cmake
```

---

## 4. Build (recommended): interactive scripts

From the **repository root**:

| OS | Command |
|----|---------|
| **Linux / macOS** | `./scripts/build.sh` |
| **Windows** | `.\scripts\build.ps1` |

The script will:

1. Check for `git`, `python3`/`python`, `cmake`, `ninja`, `arm-none-eabi-gcc`
2. Ask for **board** (`OGXM_BOARD`)
3. Ask for **Default** (all modes via combos) or **Fixed** output mode
4. Ask for **Release** or **Debug** (UART logging)
5. Configure CMake with **Ninja** and compile into **`scripts/build/`**

On success, flash the **`.uf2`** from `scripts/build/` (see [§6](#6-flash-the-firmware)).

On failure, the script can save `scripts/build_log.txt` for debugging.

---

## 5. Build (manual): CMake from the command line

```bash
cd Firmware/RP2040
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DOGXM_BOARD=PI_PICOW \
  -DMAX_GAMEPADS=1
cmake --build build
```

Replace `PI_PICOW` with your board. Common values:

| `OGXM_BOARD` | Board |
|--------------|--------|
| `PI_PICO` | Raspberry Pi Pico |
| `PI_PICO2` | Raspberry Pi Pico 2 |
| `PI_PICOW` | Raspberry Pi Pico W |
| `PI_PICO2W` | Raspberry Pi Pico 2 W |
| `RP2040_ZERO` | Waveshare RP2040-Zero |
| `RP2350_ZERO` | Waveshare RP2350-Zero |
| `RP2350_USB_A` | Waveshare RP2350-USB-A |
| `RP2040_XIAO` | Seeed XIAO RP2040 |
| `RP2354` | RP2350 + Pi Radio Module 2 |
| `ADAFRUIT_FEATHER` | Adafruit Feather USB Host |
| `EXTERNAL_4CH_I2C` | 4-channel I2C |
| `ESP32_BLUEPAD32_I2C` / `ESP32_BLUERETRO_I2C` | ESP32 hybrid boards |

Useful options:

| CMake option | Meaning |
|--------------|---------|
| `-DCMAKE_BUILD_TYPE=Debug` | UART logging (`CONFIG_OGXM_DEBUG`) |
| `-DOGXM_FIXED_DRIVER=STEAM` | Lock one output mode (also `XINPUT`, `PS3`, `WII`, `GAMECUBE`, `N64`, …) |
| `-DOGXM_FIXED_DRIVER_ALLOW_COMBOS=ON` | Keep combos even when fixed |
| `-DMAX_GAMEPADS=1` | Gamepad count (multi-pad limits apply; see README) |
| `-DOGXM_SWITCH2_HID_RAW_LOG=ON` | Debug-only Switch 2 Pro HID hex dumps |

Artifacts appear under the build directory (e.g. `Firmware/RP2040/build/` or `scripts/build/` when using the script).

**First configure** applies patches under `Firmware/external/patches/` to Bluepad32 / BTstack / Pico SDK (safe to re-run; “already applied” is normal).

---

## 6. Flash the firmware

1. Hold the board **BOOTSEL** button (or equivalent) and plug USB into the PC — a USB mass-storage drive appears.
2. Copy the **`.uf2`** file onto that drive.
3. The board reboots into the new firmware.

No separate `picotool` install is required for UF2 flashing (the SDK may still download picotool during the **build** for UF2 generation).

---

## 7. Debug builds (UART)

Debug builds log to **UART**, not the Pico’s USB data cable (USB CDC stdio is disabled when TinyUSB host is linked).

| Board family | TX pin | RX pin | Baud |
|--------------|--------|--------|------|
| Pico W / Pico 2 W / RP2354 | **GP4** | **GP5** | **115200** 8N1 |
| Most other boards | **GP0** | **GP1** | **115200** 8N1 |

Wire USB–serial adapter **RX ← board TX**, common **GND**. Details: [Adding_Supported_Controllers — UART](Adding_Supported_Controllers.md#uart-pins-and-baud).

---

## 8. ESP32 companion firmware (optional)

Only needed for **ESP32 + RP2040** hybrid setups. See diagrams under `Hardware/`.

**Required:**

- **Git**, **Python 3**
- **[ESP-IDF v5.1](https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32/get-started/index.html)** (VS Code ESP-IDF extension can install the toolchain)
- **esptool** (comes with ESP-IDF)

Build with the ESP-IDF environment sourced, then build the ESP32 project for your board. CMake may copy BTstack files into the ESP-IDF `components` tree (required because BTstack is not an ESP-IDF component as a plain git checkout).

If you only need a normal Pico / Pico W adapter, **skip this section**.

---

## 9. Troubleshooting

| Symptom | What to check |
|---------|----------------|
| `arm-none-eabi-gcc` not found | Install Arm GNU toolchain; restart terminal; verify PATH |
| `pico_sdk_init.cmake` / SDK missing | Clone SDK **2.1.0** into `Firmware/external/pico-sdk` or set `PICO_SDK_PATH` ([§3](#3-pico-sdk-required-not-a-git-submodule)) |
| `compile_gatt.py` / BTstack missing | `git submodule update --init --recursive` (Bluepad32 nested submodule) |
| Patch apply fatal error | Clean submodule and re-init; ensure you did not manually break `bluepad32` / `btstack` / `pico-sdk` trees |
| CMake / Ninja not found | Install per [§1](#1-required-applications-rp2040--rp2350-firmware) |
| Configure needs network | First build may fetch picotool; allow GitHub access |
| Wrong board UF2 | Rebuild with the correct `-DOGXM_BOARD=…` |
| Windows script execution policy | `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` if `.ps1` is blocked |

Still stuck? Open an issue only after reading [Support_Issue_Requirements.md](Support_Issue_Requirements.md), and attach `scripts/build_log.txt` if you have it.

---

## Quick checklist

- [ ] `git`, `python3`, `cmake`, `ninja`, `arm-none-eabi-gcc` installed and on PATH  
- [ ] `git clone --recursive` **or** `git submodule update --init --recursive`  
- [ ] Pico SDK **2.1.0** at `Firmware/external/pico-sdk` **or** `PICO_SDK_PATH` set  
- [ ] `./scripts/build.sh` or `.\scripts\build.ps1` (or manual CMake)  
- [ ] Flash `.uf2` via BOOTSEL  

---

## Related docs

| Document | Topic |
|----------|--------|
| [Firmware_Architecture.md](Firmware_Architecture.md) | Folder structure, runtime flow, host/device driver file checklists |
| [README — Build](../../../README.md#build) | Short build overview / board list |
| [Support_Issue_Requirements.md](Support_Issue_Requirements.md) | What to include if the build fails and you need help |
| [Adding_Supported_Controllers.md](Adding_Supported_Controllers.md) | Debug UART when developing host drivers |
| [Documentation index](README.md) | All firmware docs by category |
