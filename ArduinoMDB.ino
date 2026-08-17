#include "BillValidator.h"
#include "CoinChanger.h"
#include "MDBSerial.h"

MDBSerial mdb(1);
CoinChanger changer(mdb);
BillValidator validator(mdb);

// UART0 is used for diagnostics; UART1 is reserved for MDB by MDBSerial.
UART uart(0);

void setup()
{
  mdb.begin();

  uart.begin();
  Logger::SetUART(&uart);

  changer.Reset();
  validator.Reset();
  uart.println("###############");
}

void loop()
{
  unsigned long change;
  changer.Update(change);
  validator.Update(change);
  delay(200);
}
