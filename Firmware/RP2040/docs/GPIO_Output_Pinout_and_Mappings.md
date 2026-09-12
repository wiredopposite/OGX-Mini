# GPIO Output Modes: Pin-Outs and Default Controller Mappings

This document lists the **pin connections** and **default button/stick mappings** for the GPIO-based output modes (PS1/PS2, Dreamcast, GameCube, N64) on OGX-Mini.

**Why two kinds of pin numbers?**
- **GPIO numbers** (e.g. 19, 20, 21) are **RP2040 GPIO pins** used for data. The firmware uses these; you wire each signal to the matching GPIO on the Pico.
- **“Connector” numbers** (e.g. connector pin 4, pin 5) are the **pin positions on the console or controller plug** (the physical port). VCC and GND are **not** GPIOs — they’re power rails. On the **adapter (Pico) side** you use the board’s **3V3** and **GND** pins (the Pico’s power pins, not GPIO). On the **console/controller side** you use the plug’s VCC and GND positions (connector pin X, Y) so your cable is wired correctly. So: data = GPIO numbers on the Pico; power = Pico 3V3/GND on the adapter side, and connector pin numbers on the plug side.

**Conventions:**
- **Output mode** = adapter emulates a controller and connects to the console’s controller port.
- **Input (host) mode** = adapter reads a real console controller over GPIO and uses it as input (e.g. to send to Switch/Xbox over USB).
- PadIn = internal gamepad state (from USB/BT or from a GPIO host). Mappings below are **PadIn → console**.
- **Power:** Adapter side = Pico **3V3** and **GND** (board power pins). Console/controller side = **connector pin** numbers for VCC and GND on the plug. When **outputting** to the console, only connect adapter GND to the port’s GND. When **reading** a real controller, connect Pico **3V3** to the plug’s VCC pin and Pico **GND** to the plug’s GND pin.

---

## 1. PlayStation 1 / PlayStation 2 (PS1/PS2)

### Pin-out (output to console and input from real controller)

PS1/PS2 use a **9-pin** controller connector. Below: **GPIO** = RP2040 pin (adapter); **connector** = plug pin # (console/controller cable).

| Signal        | Adapter (Pico) | Console/controller plug | Notes                          |
|---------------|----------------|--------------------------|---------------------------------|
| DATA          | **GPIO 19**    | Connector 1              | Bidirectional data line        |
| COMMAND (CMD) | **GPIO 20**    | Connector 2              |                                 |
| GND           | **GND**        | Connector 4              | Not a GPIO — Pico GND pin      |
| VCC           | **3V3**        | Connector 5              | Not a GPIO — Pico 3V3 pin; only when powering a controller |
| ATTENTION/SEL | **GPIO 21**    | Connector 6              | Select / Attention             |
| CLOCK (CLK)   | **GPIO 22**    | Connector 7              |                                 |
| ACKNOWLEDGE   | **GPIO 26**    | Connector 9              | ACK from controller to console |

**Output mode:** Connect each GPIO to the same-numbered connector pin and Pico **GND** to connector **4** (do not use console VCC). **Input mode:** Same GPIO↔connector data mapping; connect Pico **3V3** → plug pin **5**, Pico **GND** → plug pin **4** to power the controller.

**Can the Pico be powered by the PS2 controller port?**  
The PS1/PS2 controller port provides **3.3 V** on connector pin **5** (VCC) and **GND** on pin **4**. In theory you could power the Pico from the port by connecting pin 5 to the Pico’s **VSYS** (e.g. pin 39 on the Pico), since VSYS accepts about 1.8–5.5 V. **This is not recommended:** the port is designed to power a small controller, not an MCU; current may be limited and the adapter’s draw (especially with USB or Bluetooth) can exceed what the port safely supplies. The supported setup is to power the Pico via USB or an external 3.3 V / 5 V supply and only connect **GND** and the data lines to the console.

### Default controller mapping (PadIn → PS1/PS2)

| Console (PS1/PS2) | Adapter input (PadIn) |
|-------------------|------------------------|
| D-pad Up/Down/Left/Right | D-pad Up/Down/Left/Right |
| Select            | Back (Select)          |
| Start             | Start                  |
| L3                | L3 (left stick click)  |
| R3                | R3 (right stick click) |
| L1                | LB (left bumper)       |
| R1                | RB (right bumper)     |
| L2                | Left trigger (digital + analog) |
| R2                | Right trigger (digital + analog) |
| Triangle         | Y                      |
| Circle           | B                      |
| Cross            | A                      |
| Square           | X                      |
| Right stick X/Y  | joystick_rx / joystick_ry |
| Left stick X/Y   | joystick_lx / joystick_ly |

Sticks use 0x80 (128) as center; full range 0–255.

---

## 2. Dreamcast (Maple Bus)

### Pin-out (output to console and input from real controller)

Dreamcast uses a **5-pin** controller connector. **GPIO** = RP2040 (adapter); **connector** = plug pin #.

| Signal | Adapter (Pico) | Console/controller plug | Notes                         |
|--------|----------------|--------------------------|-------------------------------|
| D0     | **GPIO 10**    | Connector 1              | Maple Bus data line A         |
| VCC    | —              | Connector 2               | +5 V from console (not 3V3)   |
| GND    | **GND**        | Connector 3               | Not a GPIO — Pico GND pin     |
| Sense  | —              | Connector 4               | Grounded by controller       |
| D1     | **GPIO 11**    | Connector 5               | Maple Bus data line B         |

**Output mode:** Connect **GPIO 10** and **11** to connector 1 and 5, and Pico **GND** to connector **3**. Do not connect console 5V to the Pico. **Input mode:** Same data + GND. To power a Dreamcast controller you need +5V (Pico is 3.3V — use console port or external 5V).

No direction pin is used in the default configuration (`dir_pin = -1`).

### Default controller mapping (PadIn → Dreamcast)

| Console (Dreamcast) | Adapter input (PadIn) |
|---------------------|------------------------|
| D-pad Up/Down/Left/Right | D-pad Up/Down/Left/Right |
| A                   | A                      |
| B                   | B                      |
| X                   | X                      |
| Y                   | Y                      |
| Start               | Start                  |
| C                   | RB                     |
| Z                   | LB                     |
| D (extra)           | MISC                   |
| L trigger (analog) | trigger_l (0–255)     |
| R trigger (analog) | trigger_r (0–255)     |
| Left stick          | joystick_lx / joystick_ly |
| Right stick        | joystick_rx / joystick_ry |

Dreamcast uses inverted logic for buttons (0 = pressed). Sticks are 0–255, 128 center.

---

## 3. GameCube / Wii (GameCube ports)

### Pin-out (output to console and input from real controller)

GameCube controller port is a **7-pin** connector. **GPIO** = RP2040 (adapter); **connector** = plug pin #.

| Signal | Adapter (Pico) | Console/controller plug | Notes                              |
|--------|----------------|--------------------------|------------------------------------|
| —      | —              | Connector 1              | 5 V rumble (not used for logic)    |
| DATA   | **GPIO 19**    | Connector 2              | Single-wire JoyBus data            |
| GND    | **GND**        | Connector 3, 4           | Not a GPIO — Pico GND pin          |
| 3.3 V  | **3V3**        | Connector 6              | Not a GPIO — Pico 3V3; only when powering a controller |

**Output mode:** Connect **GPIO 19** to connector **2** and Pico **GND** to connector **3** (and/or **4**). Do not connect console 5V or 3.3V to the Pico. **Input mode:** Same DATA and GND; connect Pico **3V3** → plug **6** to power the controller.

### Default controller mapping (PadIn → GameCube)

| Console (GameCube) | Adapter input (PadIn) |
|--------------------|------------------------|
| D-pad Up/Down/Left/Right | D-pad Up/Down/Left/Right |
| A                  | A                      |
| B                  | B                      |
| X                  | X                      |
| Y                  | Y                      |
| Start              | Start                  |
| Z                  | MISC                   |
| L                  | LB                     |
| R                  | RB                     |
| L trigger (analog) | trigger_l (0–255)      |
| R trigger (analog) | trigger_r (0–255)      |
| Left stick         | joystick_lx / joystick_ly |
| C-stick (right)    | joystick_rx / joystick_ry |

Sticks use 0x80 (128) as center; full range 0–255.

---

## 4. Nintendo 64 (N64)

### Pin-out (output to console and input from real controller)

N64 controller port is a **3-pin** connector. **GPIO** = RP2040 (adapter); **connector** = plug pin #.

| Signal | Adapter (Pico) | Console/controller plug | Notes                              |
|--------|----------------|--------------------------|------------------------------------|
| VCC    | **3V3**        | Connector 1              | Not a GPIO — Pico 3V3; only when powering a controller |
| DATA   | **GPIO 19**    | Connector 2              | Single-wire data (3.3 V logic)      |
| GND    | **GND**        | Connector 3               | Not a GPIO — Pico GND pin          |

**Output mode:** Connect **GPIO 19** to connector **2** and Pico **GND** to connector **3**. Do not connect console 3.3V to the Pico. **Input mode:** Same DATA and GND; connect Pico **3V3** → plug **1** to power the controller.

Same physical pin (GP19) as GameCube; only one of N64 or GameCube is active at a time.

### Default controller mapping (PadIn → N64)

| Console (N64) | Adapter input (PadIn) |
|---------------|------------------------|
| D-pad Up/Down/Left/Right | D-pad Up/Down/Left/Right |
| A             | A                      |
| B             | B                      |
| Z             | MISC                   |
| Start         | Start                  |
| L             | LB                     |
| R             | RB                     |
| C-Up          | Right stick Y negative (threshold 4000)  |
| C-Down        | Right stick Y positive (threshold 4000)  |
| C-Left        | Right stick X negative (threshold 4000)  |
| C-Right       | Right stick X positive (threshold 4000)  |
| Joystick X/Y  | joystick_lx / joystick_ly |

N64 stick is signed 8-bit, 0x80 center. C-buttons are derived from the right analog stick with a ±4000 threshold; diagonals combine.

---

## Summary table (output mode: Pico GPIO + power)

| Mode     | Pico: data (GPIO)   | Pico: power  | Plug: GND  | Plug: VCC (input mode only) |
|----------|---------------------|--------------|------------|------------------------------|
| PS1/PS2  | 19, 20, 21, 22, 26  | GND          | Connector 4 | Connector 5 (Pico 3V3)       |
| Dreamcast| 10, 11              | GND          | Connector 3 | Connector 2 (5V from console) |
| GameCube | 19                  | GND          | Connector 3 or 4 | Connector 6 (Pico 3V3)   |
| N64      | 19                  | GND          | Connector 3 | Connector 1 (Pico 3V3)       |

Data lines use **GPIO** numbers on the Pico. Power uses the Pico’s **GND** and **3V3** pins (not GPIO). Connector columns are the **plug pin numbers** on the console/controller cable. When **reading** a controller, also connect Pico 3V3 to the plug’s VCC pin (except Dreamcast, which needs 5V from the console or another source).

**Note:** GameCube and N64 share GP19. Do not use both at the same time on the same pin; switch output mode (and input source if using GPIO input) as needed.

---

## Changing mappings

Default mappings are fixed in the device drivers. User-configurable button remapping is available for **USB device** modes (e.g. Switch, XInput) via the web app; GPIO output modes (PS1/PS2, Dreamcast, GameCube, N64) currently use the defaults above only. See **[Controller_Mappings.md](Controller_Mappings.md)** for the full mapping reference across all modes.
