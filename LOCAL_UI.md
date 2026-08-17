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

## Product drop verification

Default wiring:

| Function | Mega pin | Default state |
|---|---|---|
| Motor/relay output | 31 | HIGH while vending |
| Drop beam receiver | 30 | INPUT_PULLUP; LOW when beam is broken |

The `VendMechanism` constructor controls output polarity, beam polarity, vend
timeout and beam filter:

```cpp
VendMechanism dispenser(31, 30, true, true, 5000, 40);
```

This means:

- motor output active HIGH;
- beam break active LOW;
- maximum motor run time 5000 ms;
- beam must remain broken for 40 ms before it counts as a product drop.

Change these values to match the relay and beam receiver. Use an appropriate
driver, flyback protection and separate motor supply; never drive a vending
motor or relay coil directly from a Mega GPIO.

The code refuses to start a vend if the beam is already blocked. After Nayax
approves the transaction, the Arduino energises the output and arms the beam.
A filtered beam break reports MDB `VEND SUCCESS`. If no break occurs before
the timeout, it stops the output and reports `VEND FAILURE`.

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
