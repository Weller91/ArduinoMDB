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

## Build

Open `ArduinoMDB.ino` in Arduino IDE 2, select **Arduino Mega or Mega 2560**, and
compile with the current Arduino AVR Boards package. No external library or Git
submodule checkout is required; the UART and logger sources are included here.

## Wiring files

- `wiring/interface.fzz` — editable legacy Fritzing project
- `wiring.jpg` — breadboard view
- `MDB_scematic.jpg` — schematic view

Treat these as historical reference material. Confirm the MDB electrical layer
against the applicable MDB/ICP specification and the actual peripheral before
building hardware.

## Upstream

Forked from [Tarcontar/ArduinoMDB](https://github.com/Tarcontar/ArduinoMDB).
Upstream's last commit was 2021-12-22. This fork vendors the UART and Logger code
directly, so the stale submodule declaration has been removed.
