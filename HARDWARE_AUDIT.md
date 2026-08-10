# Hardware and dependency audit

Audit date: 2026-08-10

## Result

| Item | Repository reference | Status | Action |
|---|---|---|---|
| Arduino Mega 2560 Rev3 | Board in both wiring images | Current Arduino hardware | Retain as the supported target |
| Microchip ATmega2560 | MCU on the Mega 2560 | In Production | No replacement required |
| Arduino AVR Boards core | Required compiler/core | Current official AVR core; 1.8.8 latest observed release | Compile against 1.8.8 |
| PNP transistor, TO-92 EBC | Two generic Fritzing symbols | Not auditable: no manufacturer or part number | Select only after the electrical interface is validated |
| 5 ohm, 1 kohm and 10 kohm resistors | Generic schematic parts | Commodity parts; no lifecycle issue | Specify resistance, tolerance, power and voltage ratings in a production BOM |
| Six-pin shrouded header | Generic Fritzing part | Not auditable: no manufacturer or part number | Choose a keyed connector rated for the installation |
| Tarcontar/UART | Historical Git submodule plus vendored files | Public but inactive since 2018 | Use the maintained copy in this repository |
| Tarcontar/Logger | Historical Git submodule plus vendored files | Public but inactive since 2018 | Use the maintained copy in this repository |

## Important limits

The two PNP symbols are descriptions, not modules or orderable components. A
safe substitute cannot be chosen from package and polarity alone. At minimum,
the production design must establish:

- maximum collector-emitter voltage;
- continuous and pulsed collector current;
- gain at the required current;
- resistor dissipation and voltage rating;
- bus polarity and idle state;
- fault and short-circuit behaviour;
- required galvanic isolation;
- connector pinout and protective components.

Do not infer these values from the screenshots. The annotation `min 150mA` is
not enough to validate the transistor stage.

## What was removed

- The `.gitmodules` file: it declared UART and Logger submodules at paths that
  are not submodules; their source files are already committed in the root.
- `ExplorerBotESP32/`: unrelated ESP32 robot firmware added to this MDB fork in
  May 2026. It has no connection to MDB and prevented the repository from having
  a single clear purpose.

## Primary status sources

- Arduino Mega 2560 Rev3: https://docs.arduino.cc/hardware/mega-2560/
- Microchip ATmega2560: https://www.microchip.com/en-us/product/atmega2560
- Official Arduino AVR core: https://github.com/arduino/ArduinoCore-avr
