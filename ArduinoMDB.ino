#include "BillValidator.h"
#include "CashlessReader.h"
#include "CoinChanger.h"
#include "MDBSerial.h"

MDBSerial mdb(1);
CoinChanger changer(mdb);
BillValidator validator(mdb);
CashlessReader onyx(mdb); // Nayax Onyx as MDB Cashless Device #1 (0x10)

// UART0 is used for diagnostics; UART1 is reserved for MDB by MDBSerial.
UART uart(0);

void setup()
{
  mdb.begin();

  uart.begin();
  Logger::SetUART(&uart);

  changer.Reset();
  validator.Reset();
  onyx.Reset();
  uart.println("###############");
}

// Call this after the customer taps and the Onyx reports BEGIN SESSION.
// Price is in the scaled MDB units reported by onyx.GetScaleFactor() and
// onyx.GetDecimalPlaces(), not necessarily raw cents.
bool requestOnyxVend(uint16_t price, uint16_t itemNumber)
{
  return onyx.RequestVend(price, itemNumber);
}

// Call only after the product mechanism has definitely succeeded or failed.
bool finishOnyxVend(bool productDispensed, uint16_t itemNumber)
{
  bool reported = productDispensed
    ? onyx.VendSuccess(itemNumber)
    : onyx.VendFailure();

  if (!reported)
    return false;

  return onyx.SessionComplete();
}

void loop()
{
  unsigned long change;
  changer.Update(change);
  validator.Update(change);

  CashlessReader::State cashlessState = onyx.Update();

  // Close sessions cancelled by the card reader or denied at authorization.
  if (cashlessState == CashlessReader::VEND_DENIED ||
      cashlessState == CashlessReader::CANCEL_REQUESTED)
  {
    onyx.SessionComplete();
  }

  // Application code should:
  // 1. wait for SESSION_IDLE;
  // 2. call requestOnyxVend(price, itemNumber);
  // 3. dispense only after VEND_APPROVED;
  // 4. call finishOnyxVend(actualResult, itemNumber).
  delay(50);
}
