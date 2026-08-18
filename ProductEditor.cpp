#include "ProductEditor.h"

#if defined(UI_SPI_TOUCHSCREEN)

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace
{
const uint16_t NAVY = 0x000F;
const uint16_t GREEN = 0x05E0;
const uint16_t RED = 0xF800;
const uint16_t ORANGE = 0xFD20;
const uint16_t GREY = 0x7BEF;
const uint16_t DARK_GREY = 0x39E7;
}

ProductEditor::ProductEditor(TouchscreenUI &ui)
    : m_ui(ui),
      m_active(false),
      m_page(LIST_PAGE),
      m_field(FIELD_NONE),
      m_listPage(0),
      m_slot(0),
      m_number(0)
{
    memset(&m_draft, 0, sizeof(m_draft));
}

void ProductEditor::Enter(ProductCatalog &catalog)
{
    m_active = true;
    m_page = LIST_PAGE;
    m_listPage = 0;
    drawList(catalog);
}

void ProductEditor::Exit()
{
    m_active = false;
}

ProductEditor::Event ProductEditor::Update(ProductCatalog &catalog,
                                           ProductRecord &testProduct,
                                           uint8_t &testSlot)
{
    if (!m_active)
        return NONE;

    int16_t x;
    int16_t y;
    if (!m_ui.GetTouchPress(x, y))
        return NONE;

    if (m_page == LIST_PAGE)
    {
        if (y >= 26 && y < 218)
        {
            uint8_t row = (y - 26) / 24;
            loadSlot(catalog, (m_listPage * 8) + row);
            return NONE;
        }

        if (y >= 218 && x < 105 && m_listPage > 0)
        {
            --m_listPage;
            drawList(catalog);
        }
        else if (y >= 218 && x >= 105 && x < 215 && m_listPage < 7)
        {
            ++m_listPage;
            drawList(catalog);
        }
        else if (y >= 218 && x >= 215)
        {
            return EXIT_REQUESTED;
        }
        return NONE;
    }

    if (m_page == PRODUCT_PAGE)
    {
        uint8_t column = x < 160 ? 0 : 1;
        uint8_t row = y < 35 ? 0xFF : (y - 35) / 40;

        if (row == 0 && column == 0) beginNumber(FIELD_CODE, m_draft.code);
        else if (row == 0 && column == 1)
        {
            m_page = NAME_PAGE;
            drawName();
        }
        else if (row == 1 && column == 0) beginNumber(FIELD_PRICE, m_draft.priceCents);
        else if (row == 1 && column == 1) beginNumber(FIELD_MOTOR, m_draft.motorIndex);
        else if (row == 2 && column == 0) beginNumber(FIELD_STOCK, m_draft.stock);
        else if (row == 2 && column == 1) beginNumber(FIELD_TIME, m_draft.motorTimeoutMs);
        else if (row == 3 && column == 0)
        {
            m_draft.SetEnabled(!m_draft.IsEnabled());
            drawProduct();
        }
        else if (row == 3 && column == 1)
        {
            if (catalog.Save(m_slot, m_draft))
            {
                m_page = LIST_PAGE;
                drawList(catalog);
                return SAVED;
            }
        }
        else if (row == 4 && column == 0)
        {
            testProduct = m_draft;
            testSlot = m_slot;
            return TEST_REQUESTED;
        }
        else if (row == 4 && column == 1)
        {
            m_page = LIST_PAGE;
            drawList(catalog);
        }
        return NONE;
    }

    if (m_page == NUMBER_PAGE)
    {
        char key = numberKey(x, y);
        if (key >= '0' && key <= '9')
        {
            if (m_number <= 9999999UL)
                m_number = (m_number * 10UL) + (key - '0');
            drawNumber();
        }
        else if (key == 'C')
        {
            m_number = 0;
            drawNumber();
        }
        else if (key == 'A')
        {
            applyNumber();
            m_page = PRODUCT_PAGE;
            drawProduct();
        }
        else if (key == 'B')
        {
            m_page = PRODUCT_PAGE;
            drawProduct();
        }
        return NONE;
    }

    if (m_page == NAME_PAGE)
    {
        char key = nameKey(x, y);
        size_t length = strlen(m_draft.name);

        if (key >= 'A' && key <= 'Z' && length < 16)
        {
            m_draft.name[length] = key;
            m_draft.name[length + 1] = 0;
            drawName();
        }
        else if (key == ' ' && length < 16)
        {
            m_draft.name[length] = ' ';
            m_draft.name[length + 1] = 0;
            drawName();
        }
        else if (key == '<' && length > 0)
        {
            m_draft.name[length - 1] = 0;
            drawName();
        }
        else if (key == 'A')
        {
            m_page = PRODUCT_PAGE;
            drawProduct();
        }
    }

    return NONE;
}

void ProductEditor::drawList(ProductCatalog &catalog)
{
    Adafruit_ILI9341 &display = m_ui.Display();
    display.fillScreen(NAVY);
    display.setTextColor(ILI9341_WHITE, NAVY);
    display.setTextSize(2);
    display.setCursor(5, 5);
    display.print("PRODUCTS ");
    display.print((m_listPage * 8) + 1);
    display.print("-");
    display.print((m_listPage * 8) + 8);

    for (uint8_t row = 0; row < 8; ++row)
    {
        uint8_t slot = (m_listPage * 8) + row;
        ProductRecord record;
        int16_t y = 27 + (row * 24);
        display.drawRect(2, y, 316, 22, DARK_GREY);
        display.setCursor(5, y + 4);
        display.setTextSize(1);
        display.print(slot);
        display.print(": ");

        if (catalog.Get(slot, record))
        {
            display.print(record.code);
            display.print(" ");
            display.print(record.name);
            display.print(" $");
            display.print(record.priceCents / 100UL);
            display.print(".");
            if ((record.priceCents % 100UL) < 10) display.print("0");
            display.print(record.priceCents % 100UL);
            display.print(" M");
            display.print(record.motorIndex);
            display.print(" Q");
            display.print(record.stock);
        }
        else
        {
            display.print("<EMPTY>");
        }
    }

    drawButton("PREV", 2, 218, 100, 21, GREY);
    drawButton("NEXT", 110, 218, 100, 21, GREY);
    drawButton("EXIT", 218, 218, 100, 21, RED);
}

void ProductEditor::drawProduct()
{
    Adafruit_ILI9341 &display = m_ui.Display();
    display.fillScreen(NAVY);
    display.setTextColor(ILI9341_WHITE, NAVY);
    display.setTextSize(2);
    display.setCursor(5, 7);
    display.print("EDIT SLOT ");
    display.print(m_slot);

    char label[24];

    snprintf(label, sizeof(label), "CODE %u", m_draft.code);
    drawButton(label, 3, 35, 154, 34, DARK_GREY);
    snprintf(label, sizeof(label), "NAME %.10s", m_draft.name);
    drawButton(label, 163, 35, 154, 34, DARK_GREY);

    snprintf(label, sizeof(label), "PRICE $%lu.%02lu",
             (unsigned long)(m_draft.priceCents / 100UL),
             (unsigned long)(m_draft.priceCents % 100UL));
    drawButton(label, 3, 75, 154, 34, DARK_GREY);
    snprintf(label, sizeof(label), "MOTOR %u", m_draft.motorIndex);
    drawButton(label, 163, 75, 154, 34, DARK_GREY);

    snprintf(label, sizeof(label), "STOCK %u", m_draft.stock);
    drawButton(label, 3, 115, 154, 34, DARK_GREY);
    snprintf(label, sizeof(label), "TIME %ums", m_draft.motorTimeoutMs);
    drawButton(label, 163, 115, 154, 34, DARK_GREY);

    drawButton(m_draft.IsEnabled() ? "ENABLED" : "DISABLED",
               3, 155, 154, 34, m_draft.IsEnabled() ? GREEN : ORANGE);
    drawButton("SAVE", 163, 155, 154, 34, GREEN);
    drawButton("TEST VEND", 3, 195, 154, 34, ORANGE);
    drawButton("BACK", 163, 195, 154, 34, GREY);
}

void ProductEditor::drawNumber()
{
    Adafruit_ILI9341 &display = m_ui.Display();
    display.fillScreen(NAVY);
    display.setTextColor(ILI9341_WHITE, NAVY);
    display.setTextSize(3);
    display.setCursor(8, 15);
    display.print(m_number);

    const char *digits[12] = {
        "1", "2", "3", "4", "5", "6",
        "7", "8", "9", "C", "0", ""
    };

    for (uint8_t row = 0; row < 4; ++row)
        for (uint8_t column = 0; column < 3; ++column)
        {
            uint8_t index = row * 3 + column;
            if (digits[index][0])
                drawButton(digits[index], 5 + column * 58,
                           65 + row * 42, 52, 36,
                           digits[index][0] == 'C' ? ORANGE : DARK_GREY);
        }

    drawButton("OK", 190, 75, 125, 55, GREEN);
    drawButton("BACK", 190, 150, 125, 55, GREY);
}

void ProductEditor::drawName()
{
    Adafruit_ILI9341 &display = m_ui.Display();
    display.fillScreen(NAVY);
    display.setTextColor(ILI9341_WHITE, NAVY);
    display.setTextSize(2);
    display.setCursor(5, 8);
    display.print(m_draft.name);

    for (uint8_t index = 0; index < 26; ++index)
    {
        char label[2] = {(char)('A' + index), 0};
        uint8_t row = index / 7;
        uint8_t column = index % 7;
        drawButton(label, 3 + column * 45, 45 + row * 39,
                   41, 34, DARK_GREY);
    }

    drawButton("<", 3, 205, 70, 32, ORANGE);
    drawButton("SPACE", 80, 205, 130, 32, GREY);
    drawButton("OK", 217, 205, 100, 32, GREEN);
}

void ProductEditor::drawButton(const char *label, int16_t x, int16_t y,
                               int16_t width, int16_t height, uint16_t colour)
{
    Adafruit_ILI9341 &display = m_ui.Display();
    display.fillRoundRect(x, y, width, height, 3, colour);
    display.drawRoundRect(x, y, width, height, 3, ILI9341_WHITE);
    display.setTextColor(ILI9341_WHITE, colour);
    display.setTextSize(1);
    int16_t textWidth = strlen(label) * 6;
    display.setCursor(x + max(3, (width - textWidth) / 2),
                      y + (height / 2) - 4);
    display.print(label);
}

void ProductEditor::loadSlot(ProductCatalog &catalog, uint8_t slot)
{
    m_slot = slot;
    if (!catalog.Get(slot, m_draft))
    {
        memset(&m_draft, 0, sizeof(m_draft));
        m_draft.code = 100 + slot;
        strncpy(m_draft.name, "NEW PRODUCT", 16);
        m_draft.motorIndex = slot;
        m_draft.motorTimeoutMs = 8000;
        m_draft.SetEnabled(false);
    }

    m_page = PRODUCT_PAGE;
    drawProduct();
}

void ProductEditor::beginNumber(Field field, uint32_t value)
{
    m_field = field;
    m_number = value;
    m_page = NUMBER_PAGE;
    drawNumber();
}

void ProductEditor::applyNumber()
{
    if (m_field == FIELD_CODE && m_number <= 65535UL)
        m_draft.code = (uint16_t)m_number;
    else if (m_field == FIELD_PRICE)
        m_draft.priceCents = m_number;
    else if (m_field == FIELD_MOTOR && m_number < 64)
        m_draft.motorIndex = (uint8_t)m_number;
    else if (m_field == FIELD_STOCK && m_number <= 65535UL)
        m_draft.stock = (uint16_t)m_number;
    else if (m_field == FIELD_TIME && m_number >= 250 && m_number <= 65535UL)
        m_draft.motorTimeoutMs = (uint16_t)m_number;
}

char ProductEditor::numberKey(int16_t x, int16_t y) const
{
    if (x >= 190)
    {
        if (y >= 75 && y <= 130) return 'A';
        if (y >= 150 && y <= 205) return 'B';
        return 0;
    }

    if (x < 5 || x > 179 || y < 65 || y > 233)
        return 0;

    uint8_t column = (x - 5) / 58;
    uint8_t row = (y - 65) / 42;
    static const char keys[4][3] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'C', '0', 0}
    };
    return column < 3 && row < 4 ? keys[row][column] : 0;
}

char ProductEditor::nameKey(int16_t x, int16_t y) const
{
    if (y >= 205)
    {
        if (x < 75) return '<';
        if (x < 215) return ' ';
        return 'A';
    }

    if (x < 3 || y < 45 || y >= 201)
        return 0;

    uint8_t column = (x - 3) / 45;
    uint8_t row = (y - 45) / 39;
    uint8_t index = row * 7 + column;
    return index < 26 ? ('A' + index) : 0;
}

#endif
