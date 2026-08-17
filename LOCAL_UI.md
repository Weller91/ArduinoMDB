# Optional local keypad and display

The Arduino Mega can run a local vending interface using a 20x4 HD44780 LCD
with a common PCF8574 I2C backpack and a 4x4 matrix keypad.

The driver is included in this repository and does not require third-party
LiquidCrystal or Keypad libraries.

## Enable it

At the top of `ArduinoMDB.ino`, change:

```cpp
#define ENABLE_LOCAL_UI 0
```

to:

```cpp
#define ENABLE_LOCAL_UI 1
```

Edit the example `PRODUCTS[]` catalogue to set the product codes, names and
prices in cents. The example entries are placeholders, not production prices.

## Default wiring

### 20x4 I2C LCD

| LCD backpack | Arduino Mega |
|---|---|
| VCC | 5 V |
| GND | GND |
| SDA | SDA / pin 20 |
| SCL | SCL / pin 21 |

The default I2C address is `0x27`. If the backpack uses `0x3F`, change the
third argument in:

```cpp
LocalUI localUI(KEYPAD_ROWS, KEYPAD_COLUMNS, 0x27);
```

This implementation assumes the common PCF8574 backpack mapping:
P0=RS, P1=RW, P2=Enable, P3=backlight and P4-P7=data. Backpacks with a
different mapping require changes in `LocalUI.cpp`.

### 4x4 matrix keypad

| Keypad lead | Mega pin |
|---|---|
| R1 | 22 |
| R2 | 23 |
| R3 | 24 |
| R4 | 25 |
| C1 | 26 |
| C2 | 27 |
| C3 | 28 |
| C4 | 29 |

Membrane keypad lead order varies. Verify its row and column order with a
multimeter or datasheet rather than relying on the connector position.

## Controls

| Key | Action |
|---|---|
| 0-9 | Enter product code |
| # or A | Confirm |
| C | Clear code |
| * or B | Cancel |
| D | Show status |

## Vend flow

1. Enter a product code.
2. Confirm with `#` or `A`.
3. LCD prompts the customer to tap the Nayax Onyx.
4. The Arduino requests the configured price after MDB `BEGIN SESSION`.
5. Only after `VEND APPROVED` should the product mechanism be energized.
6. The delivery sensor or mechanism controller must call
   `reportProductDispensed(true)` for a confirmed delivery, or
   `reportProductDispensed(false)` for a failure.
7. The Arduino reports MDB vend success/failure and completes the session.

Do not call `reportProductDispensed(true)` merely because the output was
energized. Use a delivery sensor or other positive mechanism feedback. Reporting
false success can charge a customer without delivering a product.

## Limits

- The example catalogue is compiled into the sketch.
- The UI currently initiates Nayax cashless vends; coin/bill credit accounting
  still needs an application-level vend controller.
- The display code is designed for a 20x4 HD44780-compatible module.
- Product names are clipped to 20 characters.
