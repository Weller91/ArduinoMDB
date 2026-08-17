#include "LocalUI.h"

#include <stdio.h>
#include <string.h>

namespace
{
const char KEYS[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

const uint8_t LCD_RS = 0x01;
const uint8_t LCD_ENABLE = 0x04;
const uint8_t LCD_BACKLIGHT = 0x08;
const uint8_t LCD_DATA_MASK = 0xF0;
const unsigned long DEBOUNCE_MS = 30;
}

LocalUI::LocalUI(const uint8_t rowPins[4], const uint8_t columnPins[4],
                 uint8_t lcdAddress)
    : m_lcdAddress(lcdAddress),
      m_backlight(LCD_BACKLIGHT),
      m_selection(0),
      m_rawKey(0),
      m_stableKey(0),
      m_keyChangedAt(0)
{
    for (uint8_t i = 0; i < 4; ++i)
    {
        m_rows[i] = rowPins[i];
        m_columns[i] = columnPins[i];
    }
}

void LocalUI::Begin()
{
    Wire.begin();

    for (uint8_t i = 0; i < 4; ++i)
    {
        pinMode(m_rows[i], OUTPUT);
        digitalWrite(m_rows[i], HIGH);
        pinMode(m_columns[i], INPUT_PULLUP);
    }

    lcdInit();
    ShowWelcome();
}

LocalUI::Event LocalUI::Update()
{
    char raw = scanKeypad();
    unsigned long now = millis();

    if (raw != m_rawKey)
    {
        m_rawKey = raw;
        m_keyChangedAt = now;
    }

    if ((now - m_keyChangedAt) < DEBOUNCE_MS || raw == m_stableKey)
        return NONE;

    m_stableKey = raw;
    if (!m_stableKey)
        return NONE;

    char key = m_stableKey;

    if (key >= '0' && key <= '9')
    {
        uint8_t digit = key - '0';
        if (m_selection <= 999)
            m_selection = (m_selection * 10U) + digit;
        renderSelection();
        return SELECTION_CHANGED;
    }

    if (key == 'C')
    {
        ClearSelection();
        return SELECTION_CHANGED;
    }

    if (key == '*' || key == 'B')
    {
        ClearSelection();
        return CANCEL;
    }

    if ((key == '#' || key == 'A') && m_selection > 0)
        return CONFIRM;

    if (key == 'D')
        return STATUS;

    return NONE;
}

void LocalUI::ClearSelection()
{
    m_selection = 0;
    renderSelection();
}

void LocalUI::ShowWelcome()
{
    ShowMessage("ARDUINO MDB READY", "Enter product code", "#/A confirm  C clear", "*/B cancel D status");
}

void LocalUI::ShowSelection(uint16_t code, const char *name, uint32_t priceCents)
{
    char first[21];
    char third[21];
    snprintf(first, sizeof(first), "PRODUCT %u", code);
    snprintf(third, sizeof(third), "PRICE $%lu.%02lu",
             (unsigned long)(priceCents / 100UL),
             (unsigned long)(priceCents % 100UL));
    ShowMessage(first, name, third, "#/A TO BUY");
}

void LocalUI::ShowTapCard()
{
    ShowMessage("PRODUCT SELECTED", "TAP CARD ON NAYAX", "* OR B TO CANCEL", "");
}

void LocalUI::ShowAuthorising()
{
    ShowMessage("PAYMENT", "AUTHORISING...", "PLEASE WAIT", "");
}

void LocalUI::ShowApproved()
{
    ShowMessage("PAYMENT APPROVED", "DISPENSE PRODUCT", "", "");
}

void LocalUI::ShowDenied()
{
    ShowMessage("PAYMENT DENIED", "TRY AGAIN", "", "");
}

void LocalUI::ShowDispensing()
{
    ShowMessage("DISPENSING...", "PLEASE WAIT", "", "");
}

void LocalUI::ShowVendResult(bool success)
{
    if (success)
        ShowMessage("THANK YOU", "TAKE YOUR PRODUCT", "", "");
    else
        ShowMessage("VEND FAILED", "PAYMENT REVERSING", "", "");
}

void LocalUI::ShowFault(const char *message)
{
    ShowMessage("OUT OF SERVICE", message, "", "");
}

void LocalUI::ShowMessage(const char *line1, const char *line2,
                          const char *line3, const char *line4)
{
    lcdClear();
    lcdPrintLine(0, line1);
    lcdPrintLine(1, line2);
    lcdPrintLine(2, line3);
    lcdPrintLine(3, line4);
}

void LocalUI::renderSelection()
{
    char line[21];
    if (m_selection)
        snprintf(line, sizeof(line), "CODE: %u", m_selection);
    else
        snprintf(line, sizeof(line), "CODE: ----");

    ShowMessage("SELECT PRODUCT", line, "#/A confirm C clear", "*/B cancel");
}

char LocalUI::scanKeypad()
{
    char found = 0;

    for (uint8_t row = 0; row < 4; ++row)
    {
        digitalWrite(m_rows[row], LOW);
        delayMicroseconds(3);

        for (uint8_t column = 0; column < 4; ++column)
        {
            if (digitalRead(m_columns[column]) == LOW)
            {
                found = KEYS[row][column];
                break;
            }
        }

        digitalWrite(m_rows[row], HIGH);
        if (found)
            break;
    }

    return found;
}

void LocalUI::lcdInit()
{
    delay(50);
    expanderWrite(m_backlight);
    delay(1000);

    lcdWrite4Bits(0x30);
    delayMicroseconds(4500);
    lcdWrite4Bits(0x30);
    delayMicroseconds(4500);
    lcdWrite4Bits(0x30);
    delayMicroseconds(150);
    lcdWrite4Bits(0x20);

    lcdCommand(0x28); // 4-bit, 2-line controller mode (supports 20x4 mapping)
    lcdCommand(0x08); // display off
    lcdClear();
    lcdCommand(0x06); // left-to-right entry
    lcdCommand(0x0C); // display on, cursor off
}

void LocalUI::lcdClear()
{
    lcdCommand(0x01);
    delayMicroseconds(2000);
}

void LocalUI::lcdSetCursor(uint8_t column, uint8_t row)
{
    static const uint8_t rowOffsets[4] = {0x00, 0x40, 0x14, 0x54};
    if (row > 3)
        row = 3;
    lcdCommand(0x80 | (column + rowOffsets[row]));
}

void LocalUI::lcdPrintLine(uint8_t row, const char *text)
{
    lcdSetCursor(0, row);
    uint8_t column = 0;

    while (text && *text && column < 20)
    {
        lcdData((uint8_t)*text++);
        ++column;
    }

    while (column++ < 20)
        lcdData(' ');
}

void LocalUI::lcdCommand(uint8_t value)
{
    lcdSend(value, 0);
}

void LocalUI::lcdData(uint8_t value)
{
    lcdSend(value, LCD_RS);
}

void LocalUI::lcdSend(uint8_t value, uint8_t mode)
{
    lcdWrite4Bits((value & LCD_DATA_MASK) | mode);
    lcdWrite4Bits(((value << 4) & LCD_DATA_MASK) | mode);
}

void LocalUI::lcdWrite4Bits(uint8_t value)
{
    expanderWrite(value);
    lcdPulseEnable(value);
}

void LocalUI::lcdPulseEnable(uint8_t value)
{
    expanderWrite(value | LCD_ENABLE);
    delayMicroseconds(1);
    expanderWrite(value & ~LCD_ENABLE);
    delayMicroseconds(50);
}

void LocalUI::expanderWrite(uint8_t value)
{
    Wire.beginTransmission(m_lcdAddress);
    Wire.write(value | m_backlight);
    Wire.endTransmission();
}
