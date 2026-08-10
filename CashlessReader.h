#pragma once

#include "MDBDevice.h"

class CashlessReader : public MDBDevice
{
public:
    enum State
    {
        OFFLINE,
        INACTIVE,
        DISABLED,
        ENABLED,
        SESSION_IDLE,
        VEND_PENDING,
        VEND_APPROVED,
        VEND_DENIED,
        CANCEL_REQUESTED,
        ENDING_SESSION,
        FAULT
    };

    explicit CashlessReader(MDBSerial &mdb, uint8_t address = 0x10);

    bool Reset();
    State Update();
    void Print();

    bool Enable();
    bool Disable();
    bool CancelReader();

    bool RequestVend(uint16_t price, uint16_t itemNumber = 0xFFFF);
    bool CancelVend();
    bool VendSuccess(uint16_t itemNumber = 0xFFFF);
    bool VendFailure();
    bool SessionComplete();

    State GetState() const { return m_state; }
    uint16_t GetAvailableFunds() const { return m_availableFunds; }
    uint16_t GetApprovedAmount() const { return m_approvedAmount; }
    uint8_t GetScaleFactor() const { return m_scaleFactor; }
    uint8_t GetDecimalPlaces() const { return m_decimalPlaces; }
    uint16_t GetCountryCode() const { return m_country; }

protected:
    int poll();

private:
    bool setupConfiguration();
    bool setupPrices();
    bool requestId();
    int readResponse(uint8_t expectedBytes = DATA_MAX);
    int handleResponse(int count);

    enum
    {
        CMD_RESET = 0x00,
        CMD_SETUP = 0x01,
        CMD_POLL = 0x02,
        CMD_VEND = 0x03,
        CMD_READER = 0x04,
        CMD_EXPANSION = 0x07,

        SETUP_CONFIG_DATA = 0x00,
        SETUP_MAX_MIN_PRICES = 0x01,

        VEND_REQUEST = 0x00,
        VEND_CANCEL = 0x01,
        VEND_SUCCESS = 0x02,
        VEND_FAILURE = 0x03,
        VEND_SESSION_COMPLETE = 0x04,

        READER_DISABLE = 0x00,
        READER_ENABLE = 0x01,
        READER_CANCEL = 0x02,

        RESP_JUST_RESET = 0x00,
        RESP_READER_CONFIG = 0x01,
        RESP_DISPLAY_REQUEST = 0x02,
        RESP_BEGIN_SESSION = 0x03,
        RESP_SESSION_CANCEL_REQUEST = 0x04,
        RESP_VEND_APPROVED = 0x05,
        RESP_VEND_DENIED = 0x06,
        RESP_END_SESSION = 0x07,
        RESP_CANCELLED = 0x08,
        RESP_PERIPHERAL_ID = 0x09,
        RESP_MALFUNCTION = 0x0A,
        RESP_OUT_OF_SEQUENCE = 0x0B
    };

    uint8_t m_address;
    State m_state;
    uint8_t m_scaleFactor;
    uint8_t m_decimalPlaces;
    uint8_t m_maxResponseTime;
    uint8_t m_miscOptions;
    uint16_t m_availableFunds;
    uint16_t m_approvedAmount;
    uint16_t m_maxPrice;
    uint16_t m_minPrice;
};
