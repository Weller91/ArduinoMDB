#pragma once

#include <Arduino.h>

struct ProductRecord
{
    uint16_t code;
    char name[17];
    uint32_t priceCents;
    uint8_t motorIndex;
    uint16_t stock;
    uint16_t motorTimeoutMs;
    uint8_t flags;
    uint8_t checksum;

    bool IsEnabled() const { return (flags & 0x01) != 0; }
    void SetEnabled(bool enabled)
    {
        if (enabled) flags |= 0x01;
        else flags &= (uint8_t)~0x01;
    }
} __attribute__((packed));

class ProductCatalog
{
public:
    static const uint8_t MAX_PRODUCTS = 64;
    static const uint8_t INVALID_SLOT = 0xFF;

    void Begin();
    bool Get(uint8_t slot, ProductRecord &record) const;
    bool Save(uint8_t slot, ProductRecord record);
    bool Remove(uint8_t slot);
    bool FindByCode(uint16_t code, ProductRecord &record,
                    uint8_t *slot = 0) const;
    bool DecrementStock(uint8_t slot);
    uint8_t Count() const;

private:
    struct Header
    {
        char magic[4];
        uint8_t version;
        uint8_t slots;
        uint8_t recordSize;
        uint8_t checksum;
    } __attribute__((packed));

    static const uint8_t VERSION = 1;
    static const int HEADER_ADDRESS = 0;

    int recordAddress(uint8_t slot) const;
    uint8_t calculateRecordChecksum(const ProductRecord &record) const;
    uint8_t calculateHeaderChecksum(const Header &header) const;
    bool validHeader(const Header &header) const;
    bool validRecord(const ProductRecord &record) const;
    void format();
    void seedDefaults();
};
