# OGX-Mini 2026 firmware build script (Windows PowerShell)
# Run from anywhere. Creates scripts\build\ and runs CMake/Ninja there; source is Firmware/RP2040. Optional log in scripts\ on failure.

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = (Resolve-Path (Join-Path $ScriptDir "..")).Path
$FirmwareRp2040 = Join-Path $RepoRoot "Firmware\RP2040"

if (-not (Test-Path (Join-Path $FirmwareRp2040 "CMakeLists.txt"))) {
    Write-Host "Error: Firmware/RP2040/CMakeLists.txt not found. Run this script from the OGX-Mini-2026 repo (scripts\ folder)." -ForegroundColor Red
    exit 1
}

$Boards = @(
    @{ Id = "PI_PICO";      Desc = "Pi Pico" },
    @{ Id = "PI_PICO2";     Desc = "Pi Pico 2" },
    @{ Id = "PI_PICOW";     Desc = "Pi Pico W" },
    @{ Id = "PI_PICO2W";    Desc = "Pi Pico 2 W" },
    @{ Id = "RP2040_ZERO";  Desc = "Waveshare RP2040-Zero" },
    @{ Id = "RP2350_ZERO";  Desc = "Waveshare RP2350-Zero" },
    @{ Id = "RP2350_USB_A"; Desc = "Waveshare RP2350-USB-A" },
    @{ Id = "RP2040_XIAO";  Desc = "Seeed Studio XIAO RP2040" },
    @{ Id = "RP2354";       Desc = "RP2354 (RP2350 + Pi Radio Module 2 — BT + PIO USB host GP0/GP1)" },
    @{ Id = "ADAFRUIT_FEATHER"; Desc = "Adafruit Feather USB Host" },
    @{ Id = "EXTERNAL_4CH_I2C";  Desc = "External 4CH I2C" },
    @{ Id = "ESP32_BLUEPAD32_I2C";  Desc = "ESP32 Bluepad32 I2C" },
    @{ Id = "ESP32_BLUERETRO_I2C";  Desc = "ESP32 BlueRetro I2C" }
)

$FixedDrivers = @(
    @{ Id = "XINPUT";   Desc = "Xbox 360 (XInput)" },
    @{ Id = "XBOXOG";   Desc = "Original Xbox (Gamepad)" },
    @{ Id = "PS3";      Desc = "PlayStation 3" },
    @{ Id = "SWITCH";   Desc = "Nintendo Switch Pro" },
    @{ Id = "WIIU";     Desc = "Wii U (GameCube Adapter)" },
    @{ Id = "WII";      Desc = "Wii (Wiimote)" },
    @{ Id = "PS1PS2";   Desc = "PlayStation 1/2 (GPIO)" },
    @{ Id = "GAMECUBE"; Desc = "GameCube (GPIO)" },
    @{ Id = "DREAMCAST"; Desc = "Dreamcast (GPIO)" },
    @{ Id = "N64";      Desc = "Nintendo 64 (GPIO)" },
    @{ Id = "DINPUT";   Desc = "DInput" },
    @{ Id = "PS4";      Desc = "PlayStation 4 (DualShock 4 USB)" },
    @{ Id = "STEAM";    Desc = "SteamOS / Bazzite (DualSense + touchpad mouse)" },
    @{ Id = "PSCLASSIC"; Desc = "PlayStation Classic" },
    @{ Id = "WEBAPP";   Desc = "Web App" }
)

function Test-Command($name) {
    $null -ne (Get-Command $name -ErrorAction SilentlyContinue)
}

Write-Host "=== OGX-Mini 2026 Build Script ===" -ForegroundColor Cyan
Write-Host ""

# --- Prerequisites ---
$Missing = @()
if (-not (Test-Command "git"))       { $Missing += "git" }
if (-not (Test-Command "python") -and -not (Test-Command "py")) { $Missing += "python" }
if (-not (Test-Command "cmake"))     { $Missing += "cmake" }
if (-not (Test-Command "ninja"))     { $Missing += "ninja" }
if (-not (Test-Command "arm-none-eabi-gcc")) { $Missing += "arm-none-eabi-gcc" }

if ($Missing.Count -gt 0) {
    Write-Host "Missing required tools: $($Missing -join ', ')" -ForegroundColor Red
    Write-Host ""
    Write-Host "Install options:"
    Write-Host "  - Chocolatey:  choco install git python cmake ninja gcc-arm-embedded"
    Write-Host "  - ARM GCC:     https://developer.arm.com/downloads/-/gnu-rm (add to PATH)"
    Write-Host "  - See README.md in the project root for details."
    Write-Host ""
    $ans = Read-Host "Exit now? [Y/n]"
    if ($ans -ne "n" -and $ans -ne "N") { exit 1 }
}

# --- Board selection ---
Write-Host "Select board:"
for ($i = 0; $i -lt $Boards.Count; $i++) {
    Write-Host "  $($i+1)) $($Boards[$i].Desc) ($($Boards[$i].Id))"
}
Write-Host "  $($Boards.Count + 1)) Cancel"
$boardChoice = Read-Host "Choice [1-$($Boards.Count)]"
if ([int]$boardChoice -eq $Boards.Count + 1) { Write-Host "Cancelled."; exit 0 }
if ([int]$boardChoice -lt 1 -or [int]$boardChoice -gt $Boards.Count) { Write-Host "Invalid choice."; exit 1 }
$OGXM_BOARD = $Boards[[int]$boardChoice - 1].Id

# --- Combo vs fixed driver ---
Write-Host ""
Write-Host "Build type:"
Write-Host "  1) Default (combos enabled, all modes selectable)"
Write-Host "  2) Fixed output mode (combos disabled, single mode)"
$buildTypeChoice = Read-Host "Choice [1-2]"
$OGXM_FIXED_DRIVER = ""
if ($buildTypeChoice -eq "2") {
    Write-Host "Select fixed output mode:"
    for ($i = 0; $i -lt $FixedDrivers.Count; $i++) {
        Write-Host "  $($i+1)) $($FixedDrivers[$i].Desc) ($($FixedDrivers[$i].Id))"
    }
    Write-Host "  $($FixedDrivers.Count + 1)) Cancel"
    $driverChoice = Read-Host "Choice [1-$($FixedDrivers.Count)]"
    if ([int]$driverChoice -eq $FixedDrivers.Count + 1) { Write-Host "Cancelled."; exit 0 }
    if ([int]$driverChoice -lt 1 -or [int]$driverChoice -gt $FixedDrivers.Count) { Write-Host "Invalid choice."; exit 1 }
    $OGXM_FIXED_DRIVER = $FixedDrivers[[int]$driverChoice - 1].Id
}

# --- Release vs Debug ---
Write-Host ""
Write-Host "Build configuration:"
Write-Host "  1) Release (smaller, faster)"
Write-Host "  2) Debug (UART logging, no optimization)"
$configChoice = Read-Host "Choice [1-2]"
$CMAKE_BUILD_TYPE = if ($configChoice -eq "2") { "Debug" } else { "Release" }

$OGXM_SWITCH2_HID_RAW_LOG = $false
if ($configChoice -eq "2") {
    Write-Host ""
    Write-Host "Switch 2 Pro USB (PID 0x2069) debugging (Debug builds only):"
    Write-Host "  1) Off (default)"
    Write-Host "  2) UART: raw HID hex when report buffer changes (-DOGXM_SWITCH2_HID_RAW_LOG=ON)"
    $s2RawChoice = Read-Host "Choice [1-2]"
    if ($s2RawChoice -eq "2") { $OGXM_SWITCH2_HID_RAW_LOG = $true }
}

# --- Run build ---
$BuildDir = Join-Path $ScriptDir "build"
if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
New-Item -ItemType Directory -Path $BuildDir | Out-Null

Write-Host ""
$fixedInfo = if ($OGXM_FIXED_DRIVER) { " fixed=$OGXM_FIXED_DRIVER" } else { "" }
$s2Info = if ($OGXM_SWITCH2_HID_RAW_LOG) { " switch2_raw_hid=ON" } else { "" }
Write-Host "Building: board=$OGXM_BOARD type=$CMAKE_BUILD_TYPE$fixedInfo$s2Info"
Write-Host "Output directory: $BuildDir"
Write-Host ""

$cmakeArgs = @(
    "-G", "Ninja",
    "-DOGXM_BOARD=$OGXM_BOARD",
    "-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
)
if ($OGXM_FIXED_DRIVER) {
    $cmakeArgs += "-DOGXM_FIXED_DRIVER=$OGXM_FIXED_DRIVER"
    $cmakeArgs += "-DOGXM_FIXED_DRIVER_ALLOW_COMBOS=OFF"
}
if ($OGXM_SWITCH2_HID_RAW_LOG) {
    $cmakeArgs += "-DOGXM_SWITCH2_HID_RAW_LOG=ON"
}

$buildLog = Join-Path $env:TEMP "ogxm_build_log.txt"
$buildSuccess = $false
try {
    Push-Location $BuildDir
    & cmake $cmakeArgs $FirmwareRp2040 2>&1 | Tee-Object -FilePath $buildLog
    if ($LASTEXITCODE -ne 0) { throw "CMake failed" }
    & ninja 2>&1 | Tee-Object -FilePath $buildLog -Append
    if ($LASTEXITCODE -ne 0) { throw "Ninja failed" }
    $buildSuccess = $true
} finally {
    Pop-Location
}

if ($buildSuccess) {
    Write-Host ""
    Write-Host "Build completed successfully. Output is in: $BuildDir" -ForegroundColor Green
    if ($CMAKE_BUILD_TYPE -eq "Debug") {
        Write-Host ""
        Write-Host "Debug logging (OGXM_LOG, Switch 2 raw HID): UART only at 115200 8N1 — not the Pico USB cable." -ForegroundColor Yellow
        if ($OGXM_BOARD -eq "PI_PICOW" -or $OGXM_BOARD -eq "PI_PICO2W") {
            Write-Host "  Wire USB-serial RX to GP4 (board TX), GND to GND."
        } else {
            Write-Host "  Wire USB-serial RX to GP0 (board TX), GND to GND."
        }
        Write-Host "  Then open that COM port in PuTTY / VS Code Serial Monitor at 115200."
    }
} else {
    Write-Host ""
    Write-Host "Build failed. You can save the output to a log file for troubleshooting." -ForegroundColor Yellow
    $logPath = Join-Path $ScriptDir "build_log.txt"
    $saveLog = Read-Host "Save log to $logPath? [y/N]"
    if ($saveLog -match "^[yY]") {
        Copy-Item $buildLog $logPath -Force
        Write-Host "Log saved to $logPath"
    }
    exit 1
}
