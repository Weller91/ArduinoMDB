#pragma once

#include <Arduino.h>
#include <Wire.h>

class LocalUI
{
public:
    enum Event
    {
        NONE,
        SELECTION_CHANGED,
        CONFIRM,
        CANCEL,
        STATUS
    };

    LocalUI(const uint8_t rowPins[4], const uint8_t columnPins[4],
            uint8_t lcdAddress = 0x27);

    void Begin();
    Event Update();

    uint16_t GetSelection() const { return m_selection; }
    void ClearSelection();

    void ShowWelcome();
    void ShowSelection(uint16_t code, const char *name, uint32_t priceCents);
    void ShowTapCard();
    void ShowAuthorising();
    void ShowApproved();
    void ShowDenied();
    void ShowDispensing();
    void ShowVendResult(bool success);
    void ShowFault(const char *message);
    void ShowMessage(const char *line1, const char *line2 = "",
                     const char *line3 = "", const char *line4 = "");

private:
    char scanKeypad();
    void renderSelection();

    void lcdInit();
    void lcdClear();
    void lcdSetCursor(uint8_t column, uint8_t row);
    void lcdPrintLine(uint8_t row, const char *text);
    void lcdCommand(uint8_t value);
    void lcdData(uint8_t value);
    void lcdSend(uint8_t value, uint8_t mode);
    void lcdWrite4Bits(uint8_t value);
    void lcdPulseEnable(uint8_t value);
    void expanderWrite(uint8_t value);

    uint8_t m_rows[4];
    uint8_t m_columns[4];
    uint8_t m_lcdAddress;
    uint8_t m_backlight;

    uint16_t m_selection;
    char m_rawKey;
    char m_stableKey;
    unsigned long m_keyChangedAt;
};
