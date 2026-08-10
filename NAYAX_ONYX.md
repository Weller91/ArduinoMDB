# Nayax Onyx MDB integration

This integration treats the Arduino Mega as the **VMC/master** and the Nayax
Onyx as MDB Cashless Device #1.

## Required Onyx hardware

- Nayax Onyx with an active merchant configuration;
- the Nayax MDB cable, not a pulse-only cable;
- a proper MDB power/data harness and correctly rated electrical interface.

The Onyx manual specifies 12–24 V AC or 12–42 V DC input and 5 W consumption,
with a recommended external supply of 24 V DC / 2 A. Never power it from the
Arduino 5 V pin, and never connect the MDB data wire directly to an Arduino pin.

## Nayax Core settings

Match these settings with the implementation before testing:

| Setting | Initial value |
|---|---|
| Card Reader MDB Level (attribute 520) | 1 — Basic features |
| Cashless MDB Address (attribute 98) | Cashless Device #1 / 0x10 |
| Protocol/cable | MDB |
| Currency, scale and decimal places | Set for the merchant and verify from the reader configuration response |

Nayax documents Level 1 as the default. The Onyx also supports Levels 2 and 3,
but this implementation intentionally starts at Level 1. Do not enable always
idle, remote vend, basket/partial refund or other Level 3 options yet.

The Nayax setting **Ignore VMC Configuration** changes whether the reader uses
the VMC handshake values or its stored configuration. Whichever mode is chosen,
the currency and price scaling reported by `CashlessReader` must match the
prices passed to `RequestVend()`.

## Level 1 transaction sequence

1. Arduino resets and configures the Onyx.
2. Arduino enables the reader.
3. Customer taps a payment method.
4. A poll returns **BEGIN SESSION**; state becomes `SESSION_IDLE`.
5. Application calls `RequestVend(price, itemNumber)`.
6. Poll until `VEND_APPROVED` or `VEND_DENIED`.
7. Dispense only after `VEND_APPROVED`.
8. Report the actual physical result with `VendSuccess()` or `VendFailure()`.
9. Call `SessionComplete()`.
10. Poll until **END SESSION** returns the reader to `ENABLED`.

A vend approval is permission to attempt dispensing; it is not proof that the
product moved. Report failures accurately so the reader can apply its refund
logic. Never report success before the product mechanism confirms delivery.

## Price units

MDB Level 1 uses a 16-bit scaled amount:

`real amount = MDB value × scale factor × 10^(-decimal places)`

Read the values returned by:

- `GetScaleFactor()`
- `GetDecimalPlaces()`
- `GetCountryCode()`

For example, a reader reporting scale factor 1 and two decimal places represents
AUD 2.50 as `250`. Do not assume cents without checking the Onyx response.

## Bench-test checklist

- use an authorized test merchant/device configuration where available;
- log the full initialization response before enabling live payments;
- verify the Onyx reports the expected country/currency and scale;
- test approved, denied and customer-cancelled sessions;
- force a physical vend failure and confirm it is reported as `VEND_FAILURE`;
- confirm each transaction appears once in Nayax Core;
- power-cycle during each transaction stage and check recovery;
- only then enable unattended live-card use.

## Protocol scope

Implemented: MDB/ICP Level 1 cashless commands and responses needed for a
single-vend Onyx transaction.

Not implemented: Level 2 revalue, Level 3 expanded currency, always idle,
negative vend, remote vend, data entry, coupons, basket mode, partial refunds,
file transport or multi-message response decoding.

Primary references:

- Nayax Onyx: https://www.nayax.com/solution/onyx/
- Nayax MDB settings: https://nayax-u.nayax.com/article/overview-understanding-machine-attributes-nayax-core-78296
- MDB/ICP 4.3: https://www.namanow.org/wp-content/uploads/Multi-Drop-Bus-and-Internal-Communication-Protocol.pdf
