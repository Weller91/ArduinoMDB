// Choose one local interface by uncommenting it. Leave both commented for
// headless MDB operation.
// #define UI_KEYPAD_LCD
// #define UI_SPI_TOUCHSCREEN

#if defined(UI_KEYPAD_LCD) && defined(UI_SPI_TOUCHSCREEN)
#error "Select only one local user interface"
#endif

#include "BillValidator.h"
#include "CashlessReader.h"
#include "CoinChanger.h"
#include "MDBSerial.h"
#include "MotorArray.h"
#include "VendMechanism.h"

#if defined(UI_KEYPAD_LCD)
#include "LocalUI.h"
#elif defined(UI_SPI_TOUCHSCREEN)
#include "TouchscreenUI.h"
#endif

MDBSerial mdb(1);
CoinChanger changer(mdb);
BillValidator validator(mdb);
CashlessReader onyx(mdb); // Nayax Onyx as MDB Cashless Device #1 (0x10)

// UART0 is used for diagnostics; UART1 is reserved for MDB by MDBSerial.
UART uart(0);

// Expandable motor bank.
// 74HC595 outputs: data 32, clock 33, latch 34, OE 31.
// 74HC165 home inputs: data 35, clock 36, parallel-load 37.
// Each output drives a protected MOSFET driver input, not a motor directly.
const uint8_t MOTOR_COUNT = 40;
MotorArray motors(MOTOR_COUNT, 32, 33, 34, 31, 35, 36, 37,
                  true, true, 8000, 30);

// Shared drop beam: pin 30, LOW while the falling product breaks the beam.
// The drop may occur before or shortly after the selected motor returns home.
VendMechanism dispenser(motors, 30, true, 40, 1200);

#if defined(UI_KEYPAD_LCD)
const uint8_t KEYPAD_ROWS[4] = {22, 23, 24, 25};
const uint8_t KEYPAD_COLUMNS[4] = {26, 27, 28, 29};
LocalUI activeUI(KEYPAD_ROWS, KEYPAD_COLUMNS, 0x27);
typedef LocalUI ActiveUI;
#elif defined(UI_SPI_TOUCHSCREEN)
// Mega hardware SPI: MISO 50, MOSI 51, SCK 52 and hardware SS 53.
// Display CS 10, DC 9, reset 8; touch CS 6 and IRQ 2.
TouchscreenUI activeUI(10, 9, 8, 6, 2);
typedef TouchscreenUI ActiveUI;
#endif

#if defined(UI_KEYPAD_LCD) || defined(UI_SPI_TOUCHSCREEN)
struct Product
{
  uint16_t code;
  const char *name;
  uint32_t priceCents;
  uint8_t motorIndex;
};

// Example catalogue. Replace these entries with the vending machine products.
const Product PRODUCTS[] = {
  {101, "BOOSTER PACK 1", 800, 0},
  {102, "BOOSTER PACK 2", 800, 1},
  {103, "PREMIUM BOOSTER", 1500, 2},
  {104, "ACCESSORY", 500, 3}
};
const uint8_t PRODUCT_COUNT = sizeof(PRODUCTS) / sizeof(PRODUCTS[0]);

enum LocalVendFlow
{
  SELECTING_PRODUCT,
  WAITING_FOR_SESSION,
  WAITING_FOR_APPROVAL,
  WAITING_FOR_DISPENSER
};

LocalVendFlow localVendFlow = SELECTING_PRODUCT;
const Product *selectedProduct = 0;

const Product *findProduct(uint16_t code)
{
  for (uint8_t i = 0; i < PRODUCT_COUNT; ++i)
    if (PRODUCTS[i].code == code)
      return &PRODUCTS[i];
  return 0;
}

bool centsToMdbUnits(uint32_t cents, uint16_t &units)
{
  uint32_t numerator = cents;
  uint32_t denominator = onyx.GetScaleFactor();
  uint8_t decimals = onyx.GetDecimalPlaces();

  if (!denominator)
    return false;

  if (decimals > 2)
    for (uint8_t i = 2; i < decimals; ++i)
      numerator *= 10UL;
  else
    for (uint8_t i = decimals; i < 2; ++i)
      denominator *= 10UL;

  if ((numerator % denominator) != 0)
    return false;

  uint32_t result = numerator / denominator;
  if (result > 0xFFFFUL)
    return false;

  units = (uint16_t)result;
  return true;
}

void resetLocalSelection()
{
  selectedProduct = 0;
  localVendFlow = SELECTING_PRODUCT;
  activeUI.ClearSelection();
}

bool reportProductDispensed(bool productDispensed);

void updateLocalUI(CashlessReader::State cashlessState)
{
  ActiveUI::Event event = activeUI.Update();

  if (event == ActiveUI::SELECTION_CHANGED)
  {
    const Product *product = findProduct(activeUI.GetSelection());
    if (product)
      activeUI.ShowSelection(product->code, product->name, product->priceCents);
  }
  else if (event == ActiveUI::CONFIRM &&
           localVendFlow == SELECTING_PRODUCT)
  {
    selectedProduct = findProduct(activeUI.GetSelection());
    if (selectedProduct)
    {
      localVendFlow = WAITING_FOR_SESSION;
      activeUI.ShowTapCard();
    }
    else
    {
      activeUI.ShowMessage("INVALID PRODUCT", "CHECK CODE", "CLEAR AND RETRY", "");
    }
  }
  else if (event == ActiveUI::CANCEL &&
           localVendFlow != WAITING_FOR_DISPENSER)
  {
    if (cashlessState == CashlessReader::SESSION_IDLE)
      onyx.SessionComplete();
    else if (cashlessState == CashlessReader::VEND_PENDING)
      onyx.CancelVend();
    resetLocalSelection();
    activeUI.ShowWelcome();
  }
  else if (event == ActiveUI::STATUS)
  {
    char stateLine[21];
    snprintf(stateLine, sizeof(stateLine), "CASHLESS STATE %u",
             (unsigned int)cashlessState);
    activeUI.ShowMessage("MACHINE STATUS", stateLine,
                         dispenser.IsBeamBroken() ? "BEAM BLOCKED" : "BEAM CLEAR",
                         "BACK");
  }

  if (localVendFlow == WAITING_FOR_SESSION &&
      cashlessState == CashlessReader::SESSION_IDLE)
  {
    uint16_t mdbPrice;
    if (!centsToMdbUnits(selectedProduct->priceCents, mdbPrice))
    {
      resetLocalSelection();
      activeUI.ShowFault("PRICE SCALE ERROR");
    }
    else if (onyx.RequestVend(mdbPrice, selectedProduct->code))
    {
      localVendFlow = WAITING_FOR_APPROVAL;
      activeUI.ShowAuthorising();
    }
  }

  if (cashlessState == CashlessReader::CANCEL_REQUESTED)
  {
    onyx.SessionComplete();
    dispenser.Stop();
    resetLocalSelection();
    activeUI.ShowMessage("PAYMENT CANCELLED", "SELECT AGAIN", "", "BACK");
    return;
  }

  if (cashlessState == CashlessReader::FAULT)
  {
    dispenser.Stop();
    resetLocalSelection();
    activeUI.ShowFault("NAYAX MDB FAULT");
    return;
  }

  if (localVendFlow == WAITING_FOR_APPROVAL)
  {
    if (cashlessState == CashlessReader::VEND_APPROVED)
    {
      localVendFlow = WAITING_FOR_DISPENSER;
      activeUI.ShowApproved();

      VendMechanism::Result startResult =
          dispenser.Start(selectedProduct->motorIndex);
      if (startResult == VendMechanism::SENSOR_BLOCKED)
      {
        reportProductDispensed(false);
        activeUI.ShowFault("DROP BEAM BLOCKED");
      }
      else if (startResult == VendMechanism::MOTOR_NOT_HOME)
      {
        reportProductDispensed(false);
        activeUI.ShowFault("MOTOR NOT HOME");
      }
      else if (startResult == VendMechanism::MOTOR_FAULT)
      {
        reportProductDispensed(false);
        activeUI.ShowFault("MOTOR ARRAY FAULT");
      }
      else
      {
        activeUI.ShowDispensing();
      }
    }
    else if (cashlessState == CashlessReader::VEND_DENIED)
    {
      onyx.SessionComplete();
      resetLocalSelection();
      activeUI.ShowDenied();
    }
  }

  if (localVendFlow == WAITING_FOR_DISPENSER)
  {
    VendMechanism::Result result = dispenser.Update();
    if (result == VendMechanism::DROP_CONFIRMED)
      reportProductDispensed(true);
    else if (result == VendMechanism::TIMED_OUT ||
             result == VendMechanism::MOTOR_FAULT)
      reportProductDispensed(false);
  }
}
#endif

void setup()
{
  mdb.begin();

  uart.begin();
  Logger::SetUART(&uart);

  dispenser.Begin();
  changer.Reset();
  validator.Reset();
  onyx.Reset();

#if defined(UI_KEYPAD_LCD) || defined(UI_SPI_TOUCHSCREEN)
  activeUI.Begin();
#endif

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

#if defined(UI_KEYPAD_LCD) || defined(UI_SPI_TOUCHSCREEN)
bool reportProductDispensed(bool productDispensed)
{
  if (localVendFlow != WAITING_FOR_DISPENSER || !selectedProduct)
    return false;

  uint16_t productCode = selectedProduct->code;
  dispenser.Stop();
  bool result = finishOnyxVend(productDispensed, productCode);
  resetLocalSelection();
  activeUI.ShowVendResult(productDispensed);
  return result;
}
#endif

void loop()
{
  unsigned long change;
  changer.Update(change);
  validator.Update(change);

  CashlessReader::State cashlessState = onyx.Update();

#if defined(UI_KEYPAD_LCD) || defined(UI_SPI_TOUCHSCREEN)
  updateLocalUI(cashlessState);
#else
  // Close sessions cancelled by the card reader or denied at authorization.
  if (cashlessState == CashlessReader::VEND_DENIED ||
      cashlessState == CashlessReader::CANCEL_REQUESTED)
  {
    onyx.SessionComplete();
  }
#endif

  delay(50);
}
