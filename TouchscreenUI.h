#pragma once

#if defined(UI_SPI_TOUCHSCREEN)

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

class TouchscreenUI
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

    TouchscreenUI(uint8_t tftCs, uint8_t tftDc, uint8_t tftReset,
                  uint8_t touchCs, uint8_t touchIrq,
                  int16_t minX = 250, int16_t maxX = 3800,
                  int16_t minY = 250, int16_t maxY = 3800);

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

    Adafruit_ILI9341 &Display() { return m_display; }
    bool GetTouchPress(int16_t &x, int16_t &y);

private:
    enum ScreenMode
    {
        SELECT_SCREEN,
        CANCEL_SCREEN,
        LOCKED_SCREEN,
        BACK_SCREEN
    };

    void drawSelectionScreen();
    void drawSelectionText();
    void drawKey(const char *label, int16_t x, int16_t y,
                 int16_t width, int16_t height, uint16_t colour);
    void drawMessage(const char *line1, const char *line2,
                     const char *line3, const char *line4,
                     ScreenMode mode);
    char keyAt(int16_t x, int16_t y) const;
    bool readTouch(int16_t &x, int16_t &y);

    Adafruit_ILI9341 m_display;
    XPT2046_Touchscreen m_touch;
    int16_t m_minX;
    int16_t m_maxX;
    int16_t m_minY;
    int16_t m_maxY;
    uint16_t m_selection;
    ScreenMode m_mode;
    bool m_touchWasDown;
    unsigned long m_lastTouchAt;
};

#endif
