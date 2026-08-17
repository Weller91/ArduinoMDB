#pragma once

#if defined(UI_SPI_TOUCHSCREEN)

#include "ProductCatalog.h"
#include "TouchscreenUI.h"

class ProductEditor
{
public:
    enum Event
    {
        NONE,
        SAVED,
        TEST_REQUESTED,
        EXIT_REQUESTED
    };

    explicit ProductEditor(TouchscreenUI &ui);

    void Enter(ProductCatalog &catalog);
    void Exit();
    bool IsActive() const { return m_active; }
    Event Update(ProductCatalog &catalog,
                 ProductRecord &testProduct, uint8_t &testSlot);

private:
    enum Page
    {
        LIST_PAGE,
        PRODUCT_PAGE,
        NUMBER_PAGE,
        NAME_PAGE
    };

    enum Field
    {
        FIELD_NONE,
        FIELD_CODE,
        FIELD_PRICE,
        FIELD_MOTOR,
        FIELD_STOCK,
        FIELD_TIME
    };

    void drawList(ProductCatalog &catalog);
    void drawProduct();
    void drawNumber();
    void drawName();
    void drawButton(const char *label, int16_t x, int16_t y,
                    int16_t width, int16_t height, uint16_t colour);
    void loadSlot(ProductCatalog &catalog, uint8_t slot);
    void beginNumber(Field field, uint32_t value);
    void applyNumber();
    char numberKey(int16_t x, int16_t y) const;
    char nameKey(int16_t x, int16_t y) const;

    TouchscreenUI &m_ui;
    bool m_active;
    Page m_page;
    Field m_field;
    uint8_t m_listPage;
    uint8_t m_slot;
    ProductRecord m_draft;
    uint32_t m_number;
};

#endif
