// Set to 1 to enable the optional 20x4 I2C LCD and 4x4 keypad.
#ifndef ENABLE_LOCAL_UI
#define ENABLE_LOCAL_UI 0
#endif

#include "BillValidator.h"
#include "CashlessReader.h"
#include "CoinChanger.h"
#include "MDBSerial.h"

#if ENABLE_LOCAL_UI
#include "LocalUI.h"
#endif

MDBSerial mdb(1);
CoinChanger changer(mdb);
BillValidator validator(mdb);
CashlessReader onyx(mdb); // Nayax Onyx as MDB Cashless Device #1 (0x10)

// UART0 is used for diagnostics; UART1 is reserved for MDB by MDBSerial.
UART uart(0);

#if ENABLE_LOCAL_UI
const uint8_t KEYPAD_ROWS[4] = {22, 23, 24, 25};
const uint8_t KEYPAD_COLUMNS[4] = {26, 27, 28, 29};
LocalUI localUI(KEYPAD_ROWS, KEYPAD_COLUMNS, 0x27);

struct Product
{
  uint16_t code;
  const char *name;
  uint32_t priceCents;
};

// Example catalogue. Replace these entries with the vending machine products.
const Product PRODUCTS[] = {
  {101, "BOOSTER PACK 1", 800},
  {102, "BOOSTER PACK 2", 800},
  {103, "PREMIUM BOOSTER", 1500},
  {104, "ACCESSORY", 500}
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
  localUI.ClearSelection();
}

void updateLocalUI(CashlessReader::State cashlessState)
{
  LocalUI::Event event = localUI.Update();

  if (event == LocalUI::SELECTION_CHANGED)
  {
    const Product *product = findProduct(localUI.GetSelection());
    if (product)
      localUI.ShowSelection(product->code, product->name, product->priceCents);
  }
  else if (event == LocalUI::CONFIRM &&
           localVendFlow == SELECTING_PRODUCT)
  {
    selectedProduct = findProduct(localUI.GetSelection());
    if (selectedProduct)
    {
      localVendFlow = WAITING_FOR_SESSION;
      localUI.ShowTapCard();
    }
    else
    {
      localUI.ShowMessage("INVALID PRODUCT", "CHECK CODE", "C TO CLEAR", "");
    }
  }
  else if (event == LocalUI::CANCEL)
  {
    if (cashlessState == CashlessReader::SESSION_IDLE)
      onyx.SessionComplete();
    else if (cashlessState == CashlessReader::VEND_PENDING)
      onyx.CancelVend();
    resetLocalSelection();
    localUI.ShowWelcome();
  }
  else if (event == LocalUI::STATUS)
  {
    char stateLine[21];
    snprintf(stateLine, sizeof(stateLine), "CASHLESS STATE %u",
             (unsigned int)cashlessState);
    localUI.ShowMessage("MACHINE STATUS", stateLine, "MDB ONLINE", "");
  }

  if (localVendFlow == WAITING_FOR_SESSION &&
      cashlessState == CashlessReader::SESSION_IDLE)
  {
    uint16_t mdbPrice;
    if (!centsToMdbUnits(selectedProduct->priceCents, mdbPrice))
    {
      resetLocalSelection();
      localUI.ShowFault("PRICE SCALE ERROR");
    }
    else if (onyx.RequestVend(mdbPrice, selectedProduct->code))
    {
      localVendFlow = WAITING_FOR_APPROVAL;
      localUI.ShowAuthorising();
    }
  }

  if (cashlessState == CashlessReader::CANCEL_REQUESTED)
  {
    onyx.SessionComplete();
    resetLocalSelection();
    localUI.ShowMessage("PAYMENT CANCELLED", "SELECT AGAIN", "", "");
    return;
  }

  if (cashlessState == CashlessReader::FAULT)
  {
    resetLocalSelection();
    localUI.ShowFault("NAYAX MDB FAULT");
    return;
  }

  if (localVendFlow == WAITING_FOR_APPROVAL)
  {
    if (cashlessState == CashlessReader::VEND_APPROVED)
    {
      localVendFlow = WAITING_FOR_DISPENSER;
      localUI.ShowApproved();

      // Trigger the physical product mechanism here. When its delivery sensor
      // confirms the result, call reportProductDispensed(true) or (false).
    }
    else if (cashlessState == CashlessReader::VEND_DENIED)
    {
      onyx.SessionComplete();
      resetLocalSelection();
      localUI.ShowDenied();
    }
  }
}
#endif

void setup()
{
  mdb.begin();

  uart.begin();
  Logger::SetUART(&uart);

  changer.Reset();
  validator.Reset();
  onyx.Reset();

#if ENABLE_LOCAL_UI
  localUI.Begin();
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

#if ENABLE_LOCAL_UI
bool reportProductDispensed(bool productDispensed)
{
  if (localVendFlow != WAITING_FOR_DISPENSER || !selectedProduct)
    return false;

  uint16_t productCode = selectedProduct->code;
  bool result = finishOnyxVend(productDispensed, productCode);
  resetLocalSelection();
  localUI.ShowVendResult(productDispensed);
  return result;
}
#endif

void loop()
{
  unsigned long change;
  changer.Update(change);
  validator.Update(change);

  CashlessReader::State cashlessState = onyx.Update();

#if ENABLE_LOCAL_UI
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
