# Optional local user interfaces and drop verification

The Arduino Mega can run headless, use a 20x4 LCD plus 4x4 keypad, or use an
ILI9341 SPI touchscreen with an XPT2046 touch controller.

## Select one interface

At the top of `ArduinoMDB.ino`, uncomment one option:

```cpp
// #define UI_KEYPAD_LCD
// #define UI_SPI_TOUCHSCREEN
```

Leave both commented for headless MDB operation. Do not enable both together.

## Option 1: 20x4 LCD and 4x4 keypad

The LCD/keypad driver is included and requires no third-party libraries.

### LCD wiring

| LCD backpack | Arduino Mega |
|---|---|
| VCC | 5 V |
| GND | GND |
| SDA | SDA / pin 20 |
| SCL | SCL / pin 21 |

The default I2C address is `0x27`. Change it in the `LocalUI` constructor if
the backpack uses `0x3F`.

The driver assumes the common PCF8574 mapping: P0=RS, P1=RW, P2=Enable,
P3=backlight and P4-P7=data.

### Keypad wiring

| Keypad lead | Mega pin |
|---|---|
| R1-R4 | 22, 23, 24, 25 |
| C1-C4 | 26, 27, 28, 29 |

Membrane keypad lead order varies. Verify it with a multimeter or datasheet.

| Key | Action |
|---|---|
| 0-9 | Enter product code |
| # or A | Confirm |
| C | Clear |
| * or B | Cancel |
| D | Status |

## Option 2: SPI touchscreen

Initial support targets a 320x240 ILI9341 display with an XPT2046 resistive
touch controller. It uses:

- Adafruit GFX Library
- Adafruit ILI9341
- XPT2046_Touchscreen

Install these through Arduino Library Manager.

| TFT/touch signal | Mega pin |
|---|---|
| MISO | 50 |
| MOSI | 51 |
| SCK | 52 |
| TFT CS | 10 |
| TFT DC | 9 |
| TFT RESET | 8 |
| Touch CS | 6 |
| Touch IRQ | 2 |
| Hardware SS | 53; keep configured as an output if hardware requires it |
| VCC/GND | Follow the exact module specification |

Many bare ILI9341 panels are 3.3 V devices. Use a module with documented
Mega-compatible level shifting or add proper level translation. Do not assume
every breakout accepts 5 V logic.

Touch calibration defaults to raw X/Y limits 250-3800. Adjust the four
calibration values passed to `TouchscreenUI` if button positions do not line
up. Rotation and some clone-board axis wiring may also require swapping or
inverting X/Y in `readTouch()`.

The touchscreen displays an on-screen numeric keypad and replaces both the
character LCD and physical keypad.

## Expandable motor array and drop verification

The first configuration enables 40 motors and supports up to 64. It uses chained
logic registers so the Mega pin count does not grow with every motor:

- 74HC595 serial-output registers feed protected MOSFET driver inputs;
- 74HC165 parallel-input registers read one home switch per motor;
- only one motor can be active at a time;
- a hardware-wide 74HC595 output-enable line disables every channel when idle.

### Register wiring

| Function | Mega pin |
|---|---|
| 74HC595 serial data | 32 |
| 74HC595 shift clock | 33 |
| 74HC595 latch | 34 |
| 74HC595 OE (active LOW) | 31 |
| 74HC165 serial data | 35 |
| 74HC165 clock | 36 |
| 74HC165 parallel load | 37 |
| Shared drop beam | 30 |

Five 74HC595s and five 74HC165s provide 40 channels. Add another of each and
increase `MOTOR_COUNT` for each additional eight motors, up to 64 in the
current implementation.

The 74HC595 output is only a logic signal for a MOSFET driver stage. It must not
drive a motor. Each motor channel needs a correctly rated MOSFET, gate
resistor/pulldown, flyback suppression, fuse/protection and suitable motor power
supply. All outputs are disabled through OE during boot and while idle.

The 74HC165 home inputs require defined logic levels. With the default
`homeActiveLow = true`, use suitable pull-ups and switches/sensors that pull
their input LOW at home.

### Motor cycle

A selected motor must begin with its home switch active. The controller then:

1. enables only the selected MOSFET channel;
2. waits for that motor to leave its home switch;
3. waits for it to return home;
4. stops and disables every output;
5. requires the shared drop beam to have confirmed a falling product.

A motor that does not leave and return home within 8000 ms is stopped and
reported as a failure. A motor whose switch is not home before starting is not
energised.

### Shared drop beam

The beam input is pin 30 with `INPUT_PULLUP`; the default considers LOW to be
a broken beam. The interruption must persist for 40 ms to reject noise.

A product may cross the beam before or just after the spiral returns home. The
controller records either order, then requires both events. After the motor
returns home it allows 1200 ms for the product to reach the beam. Missing either
the home cycle or the confirmed drop reports MDB `VEND FAILURE`.

### Product mapping

Each catalogue row includes its motor channel:

```cpp
{101, "BOOSTER PACK 1", 800, 0},
{102, "BOOSTER PACK 2", 800, 1}
```

The fields are product code, display name, price in cents and zero-based motor
index.

## Vend flow

1. Enter or touch a product code.
2. Confirm the product.
3. Tap the Nayax Onyx.
4. Wait for MDB `VEND APPROVED`.
5. Arduino starts the product output and arms the drop beam.
6. A valid beam interruption confirms delivery.
7. Success or failure is reported to Nayax and the session is completed.

Edit the example `PRODUCTS[]` catalogue in `ArduinoMDB.ino`; its entries are
placeholders rather than production prices.
