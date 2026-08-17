#include "TouchscreenUI.h"

#if defined(UI_SPI_TOUCHSCREEN)

#include <Arduino.h>
#include <stdio.h>

namespace
{
const uint16_t NAVY = 0x000F;
const uint16_t GREEN = 0x05E0;
const uint16_t RED = 0xF800;
const uint16_t ORANGE = 0xFD20;
const uint16_t GREY = 0x7BEF;
const uint16_t DARK_GREY = 0x39E7;
const unsigned long TOUCH_DEBOUNCE_MS = 180;
}

TouchscreenUI::TouchscreenUI(uint8_t tftCs, uint8_t tftDc, uint8_t tftReset,
                             uint8_t touchCs, uint8_t touchIrq,
                             int16_t minX, int16_t maxX,
                             int16_t minY, int16_t maxY)
    : m_display(tftCs, tftDc, tftReset),
      m_touch(touchCs, touchIrq),
      m_minX(minX),
      m_maxX(maxX),
      m_minY(minY),
      m_maxY(maxY),
      m_selection(0),
      m_mode(SELECT_SCREEN),
      m_touchWasDown(false),
      m_lastTouchAt(0)
{
}

void TouchscreenUI::Begin()
{
    m_display.begin();
    m_display.setRotation(1);
    m_touch.begin();
    m_touch.setRotation(1);
    ShowWelcome();
}

TouchscreenUI::Event TouchscreenUI::Update()
{
    int16_t x;
    int16_t y;
    if (!GetTouchPress(x, y))
        return NONE;

    if (m_mode == CANCEL_SCREEN)
    {
        if (x >= 210 && y >= 180)
            return CANCEL;
        return NONE;
    }

    if (m_mode == BACK_SCREEN)
    {
        ShowWelcome();
        return CANCEL;
    }

    if (m_mode != SELECT_SCREEN)
        return NONE;

    char key = keyAt(x, y);
    if (key >= '0' && key <= '9')
    {
        uint8_t digit = key - '0';
        if (m_selection <= 999)
            m_selection = (m_selection * 10U) + digit;
        drawSelectionText();
        return SELECTION_CHANGED;
    }

    if (key == 'C')
    {
        ClearSelection();
        return SELECTION_CHANGED;
    }

    if (key == 'B')
    {
        ClearSelection();
        return CANCEL;
    }

    if (key == 'A' && m_selection > 0)
        return CONFIRM;

    if (key == 'D')
        return STATUS;

    return NONE;
}

void TouchscreenUI::ClearSelection()
{
    m_selection = 0;
    if (m_mode == SELECT_SCREEN)
        drawSelectionText();
}

void TouchscreenUI::ShowWelcome()
{
    m_mode = SELECT_SCREEN;
    drawSelectionScreen();
    drawSelectionText();
}

void TouchscreenUI::ShowSelection(uint16_t code, const char *name,
                                  uint32_t priceCents)
{
    if (m_mode != SELECT_SCREEN)
        drawSelectionScreen();

    m_mode = SELECT_SCREEN;
    m_display.fillRect(0, 0, 320, 62, NAVY);
    m_display.setTextColor(ILI9341_WHITE, NAVY);
    m_display.setTextSize(2);
    m_display.setCursor(8, 5);
    m_display.print("PRODUCT ");
    m_display.print(code);
    m_display.setCursor(8, 26);
    m_display.print(name);
    m_display.setCursor(8, 47);
    m_display.print("$");
    m_display.print(priceCents / 100UL);
    m_display.print(".");
    if ((priceCents % 100UL) < 10)
        m_display.print("0");
    m_display.print(priceCents % 100UL);
}

void TouchscreenUI::ShowTapCard()
{
    drawMessage("PRODUCT SELECTED", "TAP CARD ON NAYAX",
                "WAIT FOR APPROVAL", "CANCEL", CANCEL_SCREEN);
}

void TouchscreenUI::ShowAuthorising()
{
    drawMessage("PAYMENT", "AUTHORISING...", "PLEASE WAIT", "CANCEL",
                CANCEL_SCREEN);
}

void TouchscreenUI::ShowApproved()
{
    drawMessage("PAYMENT APPROVED", "DISPENSING PRODUCT",
                "PLEASE WAIT", "", LOCKED_SCREEN);
}

void TouchscreenUI::ShowDenied()
{
    drawMessage("PAYMENT DENIED", "TRY AGAIN", "", "BACK", BACK_SCREEN);
}

void TouchscreenUI::ShowDispensing()
{
    drawMessage("DISPENSING...", "DROP SENSOR ARMED",
                "PLEASE WAIT", "", LOCKED_SCREEN);
}

void TouchscreenUI::ShowVendResult(bool success)
{
    if (success)
        drawMessage("THANK YOU", "TAKE YOUR PRODUCT", "", "BACK", BACK_SCREEN);
    else
        drawMessage("VEND FAILED", "PAYMENT REVERSING", "", "BACK", BACK_SCREEN);
}

void TouchscreenUI::ShowFault(const char *message)
{
    drawMessage("OUT OF SERVICE", message, "", "BACK", BACK_SCREEN);
}

void TouchscreenUI::ShowMessage(const char *line1, const char *line2,
                                const char *line3, const char *line4)
{
    drawMessage(line1, line2, line3, line4, BACK_SCREEN);
}

void TouchscreenUI::drawSelectionScreen()
{
    m_display.fillScreen(NAVY);
    m_display.drawFastHLine(0, 63, 320, ILI9341_WHITE);

    const char *digits[12] = {
        "1", "2", "3", "4", "5", "6",
        "7", "8", "9", "", "0", ""
    };

    for (uint8_t row = 0; row < 4; ++row)
    {
        for (uint8_t column = 0; column < 3; ++column)
        {
            uint8_t index = row * 3 + column;
            if (digits[index][0])
                drawKey(digits[index], 8 + (column * 62), 70 + (row * 41),
                        54, 35, DARK_GREY);
        }
    }

    drawKey("BUY", 205, 70, 105, 35, GREEN);
    drawKey("CLEAR", 205, 111, 105, 35, ORANGE);
    drawKey("CANCEL", 205, 152, 105, 35, RED);
    drawKey("STATUS", 205, 193, 105, 35, GREY);
}

void TouchscreenUI::drawSelectionText()
{
    m_display.fillRect(0, 0, 320, 62, NAVY);
    m_display.setTextColor(ILI9341_WHITE, NAVY);
    m_display.setTextSize(2);
    m_display.setCursor(8, 8);
    m_display.print("SELECT PRODUCT");
    m_display.setCursor(8, 35);
    m_display.print("CODE: ");
    if (m_selection)
        m_display.print(m_selection);
    else
        m_display.print("----");
}

void TouchscreenUI::drawKey(const char *label, int16_t x, int16_t y,
                            int16_t width, int16_t height, uint16_t colour)
{
    m_display.fillRoundRect(x, y, width, height, 4, colour);
    m_display.drawRoundRect(x, y, width, height, 4, ILI9341_WHITE);
    m_display.setTextColor(ILI9341_WHITE, colour);
    m_display.setTextSize(2);
    int16_t textWidth = strlen(label) * 12;
    m_display.setCursor(x + ((width - textWidth) / 2), y + 10);
    m_display.print(label);
}

void TouchscreenUI::drawMessage(const char *line1, const char *line2,
                                const char *line3, const char *line4,
                                ScreenMode mode)
{
    m_mode = mode;
    m_display.fillScreen(NAVY);
    m_display.setTextColor(ILI9341_WHITE, NAVY);
    m_display.setTextSize(2);

    const char *lines[3] = {line1, line2, line3};
    for (uint8_t i = 0; i < 3; ++i)
    {
        m_display.setCursor(12, 25 + (i * 38));
        m_display.print(lines[i]);
    }

    if (mode == CANCEL_SCREEN)
        drawKey(line4, 210, 180, 100, 45, RED);
    else if (mode == BACK_SCREEN)
        drawKey(line4 && line4[0] ? line4 : "BACK", 210, 180, 100, 45, GREY);
}

char TouchscreenUI::keyAt(int16_t x, int16_t y) const
{
    if (x >= 205 && x <= 310)
    {
        if (y >= 70 && y <= 105) return 'A';
        if (y >= 111 && y <= 146) return 'C';
        if (y >= 152 && y <= 187) return 'B';
        if (y >= 193 && y <= 228) return 'D';
    }

    if (x < 8 || x > 194 || y < 70 || y > 234)
        return 0;

    uint8_t column = (x - 8) / 62;
    uint8_t row = (y - 70) / 41;
    if (column > 2 || row > 3)
        return 0;

    static const char digits[4][3] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {0, '0', 0}
    };
    return digits[row][column];
}

bool TouchscreenUI::GetTouchPress(int16_t &x, int16_t &y)
{
    bool down = readTouch(x, y);
    if (!down)
    {
        m_touchWasDown = false;
        return false;
    }

    if (m_touchWasDown || (millis() - m_lastTouchAt) < TOUCH_DEBOUNCE_MS)
        return false;

    m_touchWasDown = true;
    m_lastTouchAt = millis();
    return true;
}

bool TouchscreenUI::readTouch(int16_t &x, int16_t &y)
{
    if (!m_touch.touched())
        return false;

    TS_Point point = m_touch.getPoint();
    x = constrain(map(point.x, m_minX, m_maxX, 0, 319), 0, 319);
    y = constrain(map(point.y, m_minY, m_maxY, 0, 239), 0, 239);
    return true;
}

#endif
