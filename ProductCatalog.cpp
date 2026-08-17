#include "ProductCatalog.h"

#include <EEPROM.h>
#include <string.h>

void ProductCatalog::Begin()
{
    Header header;
    EEPROM.get(HEADER_ADDRESS, header);

    if (!validHeader(header))
    {
        format();
        seedDefaults();
    }
}

bool ProductCatalog::Get(uint8_t slot, ProductRecord &record) const
{
    if (slot >= MAX_PRODUCTS)
        return false;

    EEPROM.get(recordAddress(slot), record);
    return validRecord(record);
}

bool ProductCatalog::Save(uint8_t slot, ProductRecord record)
{
    if (slot >= MAX_PRODUCTS || record.code == 0 ||
        record.motorIndex >= 64 || record.motorTimeoutMs < 250)
        return false;

    record.name[sizeof(record.name) - 1] = 0;
    record.checksum = calculateRecordChecksum(record);
    EEPROM.put(recordAddress(slot), record);
    return true;
}

bool ProductCatalog::Remove(uint8_t slot)
{
    if (slot >= MAX_PRODUCTS)
        return false;

    ProductRecord empty;
    memset(&empty, 0xFF, sizeof(empty));
    EEPROM.put(recordAddress(slot), empty);
    return true;
}

bool ProductCatalog::FindByCode(uint16_t code, ProductRecord &record,
                                uint8_t *slot) const
{
    for (uint8_t index = 0; index < MAX_PRODUCTS; ++index)
    {
        if (Get(index, record) && record.code == code)
        {
            if (slot)
                *slot = index;
            return true;
        }
    }
    return false;
}

bool ProductCatalog::DecrementStock(uint8_t slot)
{
    ProductRecord record;
    if (!Get(slot, record) || record.stock == 0)
        return false;

    --record.stock;
    return Save(slot, record);
}

uint8_t ProductCatalog::Count() const
{
    uint8_t count = 0;
    ProductRecord record;
    for (uint8_t slot = 0; slot < MAX_PRODUCTS; ++slot)
        if (Get(slot, record))
            ++count;
    return count;
}

int ProductCatalog::recordAddress(uint8_t slot) const
{
    return sizeof(Header) + ((int)slot * sizeof(ProductRecord));
}

uint8_t ProductCatalog::calculateRecordChecksum(
    const ProductRecord &record) const
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&record);
    uint8_t checksum = 0x5A;

    for (uint8_t i = 0; i < sizeof(ProductRecord) - 1; ++i)
        checksum ^= bytes[i];

    return checksum;
}

uint8_t ProductCatalog::calculateHeaderChecksum(const Header &header) const
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&header);
    uint8_t checksum = 0xA5;

    for (uint8_t i = 0; i < sizeof(Header) - 1; ++i)
        checksum ^= bytes[i];

    return checksum;
}

bool ProductCatalog::validHeader(const Header &header) const
{
    return memcmp(header.magic, "MDBP", 4) == 0 &&
           header.version == VERSION &&
           header.slots == MAX_PRODUCTS &&
           header.recordSize == sizeof(ProductRecord) &&
           header.checksum == calculateHeaderChecksum(header);
}

bool ProductCatalog::validRecord(const ProductRecord &record) const
{
    return record.code != 0 && record.code != 0xFFFF &&
           record.motorIndex < 64 &&
           record.name[0] != (char)0xFF &&
           record.checksum == calculateRecordChecksum(record);
}

void ProductCatalog::format()
{
    Header header = {{'M', 'D', 'B', 'P'}, VERSION, MAX_PRODUCTS,
                     (uint8_t)sizeof(ProductRecord), 0};
    header.checksum = calculateHeaderChecksum(header);
    EEPROM.put(HEADER_ADDRESS, header);

    ProductRecord empty;
    memset(&empty, 0xFF, sizeof(empty));
    for (uint8_t slot = 0; slot < MAX_PRODUCTS; ++slot)
        EEPROM.put(recordAddress(slot), empty);
}

void ProductCatalog::seedDefaults()
{
    ProductRecord defaults[4];
    memset(defaults, 0, sizeof(defaults));

    defaults[0].code = 101;
    strncpy(defaults[0].name, "BOOSTER PACK 1", 16);
    defaults[0].priceCents = 800;
    defaults[0].motorIndex = 0;
    defaults[0].stock = 10;
    defaults[0].motorTimeoutMs = 8000;
    defaults[0].SetEnabled(true);

    defaults[1] = defaults[0];
    defaults[1].code = 102;
    strncpy(defaults[1].name, "BOOSTER PACK 2", 16);
    defaults[1].motorIndex = 1;

    defaults[2] = defaults[0];
    defaults[2].code = 103;
    strncpy(defaults[2].name, "PREMIUM BOOSTER", 16);
    defaults[2].priceCents = 1500;
    defaults[2].motorIndex = 2;

    defaults[3] = defaults[0];
    defaults[3].code = 104;
    strncpy(defaults[3].name, "ACCESSORY", 16);
    defaults[3].priceCents = 500;
    defaults[3].motorIndex = 3;

    for (uint8_t slot = 0; slot < 4; ++slot)
        Save(slot, defaults[slot]);
}
