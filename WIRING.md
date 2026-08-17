# Wiring guide

This guide covers the Arduino Mega 2560 connections used by the example sketch.
It assumes:

- a Nayax Onyx connected through a proper MDB electrical interface;
- optional 20x4 I2C LCD plus 4x4 keypad, or an ILI9341/XPT2046 touchscreen;
- chained 74HC595 registers feeding protected MOSFET motor drivers;
- chained 74HC165 registers reading one home switch per motor;
- one shared product-drop beam.

> [!WARNING]
> Never connect MDB, a vending motor, a relay coil, or a 12/24 V sensor output
> directly to an Arduino pin. Use the correctly rated interface, driver,
> protection and level conversion for the actual hardware.

## Mega pin allocation

| Mega pin | Function |
|---|---|
| 0/1 | UART0 diagnostic logging |
| 18/19 | UART1 MDB, through the MDB interface |
| 20/21 | I2C SDA/SCL for the optional 20x4 LCD |
| 22-25 | 4x4 keypad rows R1-R4 |
| 26-29 | 4x4 keypad columns C1-C4 |
| 30 | Shared drop-beam input |
| 31 | 74HC595 output enable, active LOW |
| 32 | 74HC595 serial data |
| 33 | 74HC595 shift clock |
| 34 | 74HC595 storage/latch clock |
| 35 | 74HC165 serial home-switch data |
| 36 | 74HC165 clock |
| 37 | 74HC165 parallel load, active LOW |
| 50/51/52 | SPI MISO/MOSI/SCK for optional touchscreen |
| 53 | Mega hardware SS; keep as OUTPUT when using SPI |
| 10/9/8 | TFT CS/DC/reset |
| 6/2 | Touch CS/IRQ |

Only the pins for the selected UI are required. The motor array and beam wiring
are common to the keypad/LCD and touchscreen configurations.

## Power and grounding

Use separate appropriately rated supplies for logic, motors and the Nayax where
required by their specifications.

- Mega and 74HC logic: regulated 5 V.
- ILI9341/XPT2046: follow the exact module rating; many bare boards require
  3.3 V power and 3.3 V logic.
- Motors: separate motor supply sized for stall current.
- Nayax Onyx: use its approved MDB harness and rated supply.
- Join logic and MOSFET-driver grounds only where the driver design requires a
  common reference.
- Do not join isolated MDB conductors around the MDB interface.

Place a 100 nF ceramic bypass capacitor directly across VCC/GND at every 74HC
IC. Add suitable bulk capacitance near the motor drivers without allowing motor
noise to return through the Mega ground path.

## MDB and Nayax Onyx

The Arduino is the VMC/master and uses UART1. The Onyx is MDB Cashless Device
#1 at address `0x10`.

```text
Mega UART1 TX/RX
        |
        v
rated MDB electrical interface
        |
        v
MDB power/data harness
        |
        v
Nayax Onyx
```

MDB is not 5 V TTL. Do not connect the MDB data conductor to pins 18 or 19
directly. Use the correctly rated transistor/opto/interface circuit and verify
polarity against the MDB/ICP specification and the actual harness.

## Option A: 20x4 I2C LCD and 4x4 keypad

Select this interface in `ArduinoMDB.ino`:

```cpp
#define UI_KEYPAD_LCD
// #define UI_SPI_TOUCHSCREEN
```

### LCD

| LCD backpack | Mega |
|---|---|
| VCC | 5 V, if the module is rated for it |
| GND | GND |
| SDA | 20 / SDA |
| SCL | 21 / SCL |

The default address is `0x27`. The included driver assumes the common PCF8574
backpack mapping P0=RS, P1=RW, P2=Enable, P3=backlight and P4-P7=LCD data.

### Keypad

| Keypad contact | Mega |
|---|---|
| R1 | 22 |
| R2 | 23 |
| R3 | 24 |
| R4 | 25 |
| C1 | 26 |
| C2 | 27 |
| C3 | 28 |
| C4 | 29 |

Keypad connector order varies. Identify rows and columns with the datasheet or a
multimeter before connecting it.

## Option B: ILI9341/XPT2046 SPI touchscreen

Select this interface:

```cpp
// #define UI_KEYPAD_LCD
#define UI_SPI_TOUCHSCREEN
```

| Display/touch signal | Mega |
|---|---|
| TFT SCK | 52 |
| TFT MOSI | 51 |
| TFT MISO, if present | 50 |
| TFT CS | 10 |
| TFT DC | 9 |
| TFT RESET | 8 |
| Touch SCK | 52, shared |
| Touch MOSI | 51, shared |
| Touch MISO | 50, shared |
| Touch CS | 6 |
| Touch IRQ | 2 |
| GND | Logic GND |
| VCC/backlight | Follow the module specification |

Do not assume the TFT accepts 5 V logic. A bare 3.3 V module needs proper level
translation on SCK, MOSI, CS, DC and reset. Use a module with documented
Mega-compatible regulation/level shifting or add it externally.

## Expandable motor output chain

The initial configuration uses five 74HC595s for 40 motor channels. The code
supports up to eight registers/64 channels.

### Mega to first 74HC595

| 74HC595 DIP pin | Signal | Connection |
|---|---|---|
| 16 | VCC | 5 V |
| 8 | GND | Logic GND |
| 14 | DS/SER | Mega 32 |
| 11 | SHCP/SRCLK | Mega 33 |
| 12 | STCP/RCLK | Mega 34 |
| 13 | OE, active LOW | Mega 31 and 10 kOhm pull-up to 5 V |
| 10 | MR/SRCLR, active LOW | 5 V through 10 kOhm |
| 9 | QH' serial out | DS pin 14 of the next 74HC595 |

Pins 11, 12, 13, 10, VCC and GND are shared by every output register. Connect
QH' pin 9 of each register to DS pin 14 of the next register.

The 10 kOhm pull-up on OE keeps all motor outputs disabled while the Mega boots
or is disconnected.

### 74HC595 output channel pins

| Channel within register | DIP pin |
|---|---|
| Q0 / QA | 15 |
| Q1 / QB | 1 |
| Q2 / QC | 2 |
| Q3 / QD | 3 |
| Q4 / QE | 4 |
| Q5 / QF | 5 |
| Q6 / QG | 6 |
| Q7 / QH | 7 |

The register closest to the Mega carries motor channels 0-7, the next carries
8-15, then 16-23, 24-31 and 32-39.

Each Q output connects only to the logic input of its corresponding protected
MOSFET driver channel.

## MOSFET motor channels

A conceptual low-side channel is:

```text
74HC595 Qn -- gate resistor --> logic-level MOSFET gate
                              |
                         gate pulldown
                              |
                             GND

motor supply + ---- fuse ---- motor ---- MOSFET drain
                                      MOSFET source ---- motor supply GND

flyback diode across motor:
cathode to motor supply +
anode to MOSFET drain
```

Select the MOSFET, gate parts, diode/TVS, wiring, connector and fuse for the
motor's measured running and stall current. A module advertised as a MOSFET
board is not automatically suitable for an inductive vending motor.

The example permits only one motor at a time and disables every 74HC595 output
through OE when idle.

## Per-motor home-switch input chain

The initial 40-motor configuration uses five 74HC165s. Add one output register
and one input register for every additional eight motors.

### Mega to first 74HC165

| 74HC165 DIP pin | Signal | Connection |
|---|---|---|
| 16 | VCC | 5 V |
| 8 | GND | Logic GND |
| 9 | Q7 serial output | Mega 35 |
| 2 | CP/clock | Mega 36 |
| 1 | PL/SHLD, active LOW | Mega 37 |
| 15 | Clock inhibit | GND |
| 10 | DS serial input | Q7 pin 9 of the next 74HC165 |
| 7 | Complementary serial output | Not connected |

Clock, parallel load, clock inhibit, VCC and GND are shared by every input
register. The first/closest register's DS pin 10 connects to the next
register's Q7 pin 9, continuing down the chain.

### 74HC165 home-input pins

| Channel within register | DIP pin |
|---|---|
| D0 | 11 |
| D1 | 12 |
| D2 | 13 |
| D3 | 14 |
| D4 | 3 |
| D5 | 4 |
| D6 | 5 |
| D7 | 6 |

Each input needs a defined level. The default software expects an active-LOW
home switch:

```text
5 V ---- 10 kOhm pull-up ----+---- 74HC165 Dn
                             |
                         home switch
                             |
                            GND
```

The closest register carries home channels 0-7, matching motor channels 0-7.
Keep the physical register order identical between the output and input chains.

## Shared product-drop beam

The default input expects a beam receiver that pulls the signal LOW when a
falling product interrupts the beam.

| Beam connection | Mega |
|---|---|
| Logic output | Pin 30 |
| Logic ground | GND, only for a compatible non-isolated sensor |
| Sensor power | Use the sensor's rated supply |

Pin 30 uses `INPUT_PULLUP`. An open-collector/open-drain 5 V-compatible output
is suitable when wired according to its datasheet. A 12/24 V sensor output
requires an optocoupler, transistor or rated level-converter interface. Never
apply more than the Mega input rating.

Mount the beam so every dispensed product must pass through it and cannot rest
permanently blocking it.

## Motor/home/drop sequence

For a valid vend:

1. the selected home input must already be active;
2. the selected 74HC595 output is enabled;
3. the home switch must become inactive as the motor leaves home;
4. the same home switch must become active again;
5. the shared beam must record a filtered interruption;
6. the output chain is disabled;
7. only then is MDB `VEND SUCCESS` sent.

A blocked beam before starting, motor not initially home, missing home return,
motor timeout or missing product drop produces `VEND FAILURE`.

## Product-to-motor mapping

Catalogue entries use a zero-based motor index:

```cpp
{101, "BOOSTER PACK 1", 800, 0},
{102, "BOOSTER PACK 2", 800, 1},
{103, "PREMIUM BOOSTER", 1500, 2}
```

The fields are product code, display name, price in cents and motor channel.
