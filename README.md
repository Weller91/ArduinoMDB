# ArduinoMDB

Arduino Mega 2560 code for communicating with MDB/ICP coin changers and bill
validators and MDB cashless readers using the AVR hardware USART in 9-bit mode.

> [!WARNING]
> MDB peripherals do not use 5 V TTL signalling. Do not connect an MDB bus
> directly to an Arduino pin. Use a correctly rated, tested interface and verify
> its polarity, voltage, current and isolation requirements before connecting a
> peripheral. The historical Fritzing diagram in this repository does not name
> the transistor part numbers and is not a production-ready bill of materials.

## Supported target

- Arduino Mega 2560 Rev3 / ATmega2560
- Arduino AVR Boards core 1.8.8
- UART0: diagnostic logging at 9600 baud
- UART1: MDB at 9600 baud, 9-bit mode

The implementation accesses AVR registers and interrupt vectors directly. It is
not portable to Uno R4, GIGA, ESP32, SAMD, RP2040 or other architectures without
a new 9-bit UART backend.

## Current hardware status

No component in the documented circuit is confirmed discontinued as of
2026-08-10. Arduino still documents and sells the Mega 2560 Rev3, and Microchip
lists the ATmega2560 as **In Production**. The two PNP transistor symbols,
resistors and connector are generic Fritzing parts rather than orderable part
numbers, so their lifecycle and suitability cannot be verified.

See [HARDWARE_AUDIT.md](HARDWARE_AUDIT.md) for the full audit and replacement
rules.

## Nayax Onyx cashless support

`CashlessReader` implements the standard MDB Level 1 VMC flow for a Nayax
Onyx configured as Cashless Device #1 at address `0x10`. It supports:

- reset, configuration, max/min prices and peripheral identification;
- reader enable, disable and cancel;
- begin-session and session-cancel handling;
- vend request, approval, denial and cancellation;
- vend success/failure and session completion.

See [NAYAX_ONYX.md](NAYAX_ONYX.md) before wiring or processing a live
transaction. Level 3 features such as always-idle, remote vend, basket mode,
negative vend and partial refunds are deliberately not enabled yet.

## Optional local interfaces and drop verification

The Mega can run headless or use one of two comment-in options:

- a 20x4 I2C LCD with a physical 4x4 matrix keypad;
- a 320x240 ILI9341 SPI display with XPT2046 touch, replacing both.

Both provide product-code entry, catalogue prices, Nayax prompts and vend
results. Each catalogue entry maps to a motor channel. `MotorArray` supports
up to 64 protected MOSFET channels through chained 74HC595 registers and reads
one home switch per motor through chained 74HC165 registers.

Only one motor runs at a time. It must leave and return to its home switch, and
the shared filtered drop beam must also confirm a falling product before
`VendMechanism` reports success. Missing either condition stops the output and
reports vend failure.

With the touchscreen selected, a physical key switch opens a 64-slot,
EEPROM-backed product editor. Code, name, price, motor channel, stock, individual
motor timeout and enabled state are editable on screen. It also provides a
no-charge test vend. Successful paid vends automatically reduce stock.

See [LOCAL_UI.md](LOCAL_UI.md) for selection flags, UI wiring, touchscreen
libraries, shift-register chains, MOSFET protection, home switches and drop
sensor settings.

## Build

Open `ArduinoMDB.ino` in Arduino IDE 2, select **Arduino Mega or Mega 2560**, and
compile with the current Arduino AVR Boards package. The headless and LCD/keypad builds need no external library or Git submodule.
The SPI touchscreen option uses the three Arduino libraries listed in
`LOCAL_UI.md`.

## Wiring

See [WIRING.md](WIRING.md) for the complete Mega pin allocation, both interface
options, 74HC595 motor-output chain, MOSFET stages, 74HC165 home-switch chain,
drop beam, power rules and product-to-motor mapping.

Historical reference files:

- `wiring/interface.fzz` — editable legacy Fritzing project
- `wiring.jpg` — legacy breadboard view
- `MDB_scematic.jpg` — legacy MDB schematic view

The historical diagrams cover only the original MDB interface and are not the
complete vending-machine wiring. Confirm the MDB electrical layer against the
applicable MDB/ICP specification and actual peripheral before building.

## Upstream

Forked from [Tarcontar/ArduinoMDB](https://github.com/Tarcontar/ArduinoMDB).
Upstream's last commit was 2021-12-22. This fork vendors the UART and Logger code
directly, so the stale submodule declaration has been removed.
