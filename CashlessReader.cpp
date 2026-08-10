#include "CashlessReader.h"

#include <Arduino.h>

CashlessReader::CashlessReader(MDBSerial &mdb, uint8_t address)
    : MDBDevice(mdb),
      m_address(address),
      m_state(OFFLINE),
      m_scaleFactor(1),
      m_decimalPlaces(2),
      m_maxResponseTime(0),
      m_miscOptions(0),
      m_availableFunds(0),
      m_approvedAmount(0),
      m_maxPrice(0xFFFF),
      m_minPrice(0)
{
}

bool CashlessReader::Reset()
{
    m_state = OFFLINE;
    m_availableFunds = 0;
    m_approvedAmount = 0;

    m_mdb->SendCommand(m_address, CMD_RESET);
    if (m_mdb->GetResponse() != ACK)
    {
        warning << F("CASHLESS: RESET NOT ACKNOWLEDGED") << endl;
        return false;
    }

    // Wait for the reader's JUST RESET response. An Onyx may take several
    // seconds to bring its payment application online after power-up.
    for (int attempt = 0; attempt < 100; ++attempt)
    {
        if (poll() == RESP_JUST_RESET)
            break;
        delay(100);
    }

    if (m_state != INACTIVE)
    {
        warning << F("CASHLESS: NO JUST RESET RESPONSE") << endl;
        return false;
    }

    if (!setupConfiguration())
        return false;
    if (!setupPrices())
        return false;

    // Recommended by MDB/ICP after setup. Some Level 1 readers only ACK it,
    // so identification failure is logged but does not prevent vending.
    if (!requestId())
        warning << F("CASHLESS: ID REQUEST NOT COMPLETED") << endl;

    if (!Enable())
        return false;

    debug << F("CASHLESS: INITIALIZED") << endl;
    return true;
}

CashlessReader::State CashlessReader::Update()
{
    poll();
    return m_state;
}

bool CashlessReader::setupConfiguration()
{
    // MDB Level 1 VMC, with no VMC display exposed to the reader.
    int data[] = { 0x01, 0x00, 0x00, 0x00 };
    m_mdb->SendCommand(m_address, CMD_SETUP, SETUP_CONFIG_DATA, data, 4);

    int result = readResponse(8);
    if (result == RESP_READER_CONFIG)
        return true;
    if (result != ACK)
    {
        warning << F("CASHLESS: CONFIG COMMAND FAILED") << endl;
        return false;
    }

    // The reader may defer READER CONFIGURATION DATA to a later POLL.
    for (int attempt = 0; attempt < 50; ++attempt)
    {
        if (poll() == RESP_READER_CONFIG)
            return true;
        delay(100);
    }

    warning << F("CASHLESS: CONFIG RESPONSE TIMEOUT") << endl;
    return false;
}

bool CashlessReader::setupPrices()
{
    int data[] = {
        (m_maxPrice >> 8) & 0xFF,
        m_maxPrice & 0xFF,
        (m_minPrice >> 8) & 0xFF,
        m_minPrice & 0xFF
    };

    m_mdb->SendCommand(m_address, CMD_SETUP, SETUP_MAX_MIN_PRICES, data, 4);
    if (m_mdb->GetResponse() != ACK)
    {
        warning << F("CASHLESS: MAX/MIN PRICE SETUP FAILED") << endl;
        return false;
    }

    m_state = DISABLED;
    return true;
}

bool CashlessReader::requestId()
{
    m_mdb->SendCommand(m_address, CMD_EXPANSION, 0x00);
    int result = readResponse(DATA_MAX);
    if (result == RESP_PERIPHERAL_ID)
        return true;
    if (result != ACK)
        return false;

    for (int attempt = 0; attempt < 20; ++attempt)
    {
        if (poll() == RESP_PERIPHERAL_ID)
            return true;
        delay(100);
    }
    return false;
}

bool CashlessReader::Enable()
{
    m_mdb->SendCommand(m_address, CMD_READER, READER_ENABLE);
    if (m_mdb->GetResponse() != ACK)
        return false;

    m_state = ENABLED;
    return true;
}

bool CashlessReader::Disable()
{
    m_mdb->SendCommand(m_address, CMD_READER, READER_DISABLE);
    if (m_mdb->GetResponse() != ACK)
        return false;

    m_state = DISABLED;
    return true;
}

bool CashlessReader::CancelReader()
{
    m_mdb->SendCommand(m_address, CMD_READER, READER_CANCEL);
    int result = readResponse(1);
    if (result == RESP_CANCELLED)
        return true;
    return result == ACK;
}

bool CashlessReader::RequestVend(uint16_t price, uint16_t itemNumber)
{
    if (m_state != SESSION_IDLE)
        return false;

    int data[] = {
        (price >> 8) & 0xFF,
        price & 0xFF,
        (itemNumber >> 8) & 0xFF,
        itemNumber & 0xFF
    };

    m_approvedAmount = 0;
    m_mdb->SendCommand(m_address, CMD_VEND, VEND_REQUEST, data, 4);
    int result = readResponse(3);
    if (result == ACK)
    {
        m_state = VEND_PENDING;
        return true;
    }

    return result == RESP_VEND_APPROVED || result == RESP_VEND_DENIED;
}

bool CashlessReader::CancelVend()
{
    if (m_state != VEND_PENDING)
        return false;

    m_mdb->SendCommand(m_address, CMD_VEND, VEND_CANCEL);
    int result = readResponse(1);
    return result == ACK || result == RESP_VEND_DENIED;
}

bool CashlessReader::VendSuccess(uint16_t itemNumber)
{
    if (m_state != VEND_APPROVED)
        return false;

    int data[] = {
        (itemNumber >> 8) & 0xFF,
        itemNumber & 0xFF
    };

    m_mdb->SendCommand(m_address, CMD_VEND, VEND_SUCCESS, data, 2);
    if (m_mdb->GetResponse() != ACK)
        return false;

    m_state = SESSION_IDLE;
    return true;
}

bool CashlessReader::VendFailure()
{
    if (m_state != VEND_APPROVED)
        return false;

    m_mdb->SendCommand(m_address, CMD_VEND, VEND_FAILURE);
    if (m_mdb->GetResponse() != ACK)
        return false;

    m_state = SESSION_IDLE;
    return true;
}

bool CashlessReader::SessionComplete()
{
    if (m_state != SESSION_IDLE &&
        m_state != VEND_DENIED &&
        m_state != CANCEL_REQUESTED)
        return false;

    m_mdb->SendCommand(m_address, CMD_VEND, VEND_SESSION_COMPLETE);
    int result = readResponse(1);
    if (result == RESP_END_SESSION)
        return true;
    if (result != ACK)
        return false;

    m_state = ENDING_SESSION;
    return true;
}

int CashlessReader::poll()
{
    m_mdb->SendCommand(m_address, CMD_POLL);
    return readResponse(DATA_MAX);
}

int CashlessReader::readResponse(uint8_t expectedBytes)
{
    int answer = m_mdb->GetResponse(m_buffer, &m_count, expectedBytes);
    if (answer == ACK)
        return ACK;
    if (answer <= 0 || m_count <= 0)
        return answer;

    // Every data response must be acknowledged by the VMC.
    m_mdb->Ack();
    return handleResponse(m_count);
}

int CashlessReader::handleResponse(int count)
{
    uint8_t response = (uint8_t)m_buffer[0];

    switch (response)
    {
    case RESP_JUST_RESET:
        m_state = INACTIVE;
        debug << F("CASHLESS: JUST RESET") << endl;
        break;

    case RESP_READER_CONFIG:
        if (count < 8)
        {
            m_state = FAULT;
            return -6;
        }
        m_feature_level = m_buffer[1];
        m_country = ((uint8_t)m_buffer[2] << 8) | (uint8_t)m_buffer[3];
        m_scaleFactor = (uint8_t)m_buffer[4];
        m_decimalPlaces = (uint8_t)m_buffer[5];
        m_maxResponseTime = (uint8_t)m_buffer[6];
        m_miscOptions = (uint8_t)m_buffer[7];
        m_state = DISABLED;
        debug << F("CASHLESS: CONFIG RECEIVED") << endl;
        break;

    case RESP_BEGIN_SESSION:
        if (count >= 3)
            m_availableFunds = ((uint8_t)m_buffer[1] << 8) |
                               (uint8_t)m_buffer[2];
        m_state = SESSION_IDLE;
        debug << F("CASHLESS: BEGIN SESSION, FUNDS ") << m_availableFunds << endl;
        break;

    case RESP_SESSION_CANCEL_REQUEST:
        m_state = CANCEL_REQUESTED;
        debug << F("CASHLESS: SESSION CANCEL REQUEST") << endl;
        break;

    case RESP_VEND_APPROVED:
        if (count >= 3)
            m_approvedAmount = ((uint8_t)m_buffer[1] << 8) |
                               (uint8_t)m_buffer[2];
        m_state = VEND_APPROVED;
        debug << F("CASHLESS: VEND APPROVED, AMOUNT ") << m_approvedAmount << endl;
        break;

    case RESP_VEND_DENIED:
        m_state = VEND_DENIED;
        warning << F("CASHLESS: VEND DENIED") << endl;
        break;

    case RESP_END_SESSION:
        m_availableFunds = 0;
        m_approvedAmount = 0;
        m_state = ENABLED;
        debug << F("CASHLESS: END SESSION") << endl;
        break;

    case RESP_CANCELLED:
        m_state = ENABLED;
        debug << F("CASHLESS: CANCELLED") << endl;
        break;

    case RESP_PERIPHERAL_ID:
        debug << F("CASHLESS: PERIPHERAL ID RECEIVED") << endl;
        break;

    case RESP_DISPLAY_REQUEST:
        debug << F("CASHLESS: DISPLAY REQUEST IGNORED") << endl;
        break;

    case RESP_MALFUNCTION:
        m_state = FAULT;
        error << F("CASHLESS: MALFUNCTION ") <<
            (count > 1 ? (int)(uint8_t)m_buffer[1] : -1) << endl;
        break;

    case RESP_OUT_OF_SEQUENCE:
        m_state = FAULT;
        error << F("CASHLESS: COMMAND OUT OF SEQUENCE") << endl;
        break;

    default:
        warning << F("CASHLESS: UNSUPPORTED RESPONSE ") << (int)response << endl;
        break;
    }

    return response;
}

void CashlessReader::Print()
{
    debug << F("## CASHLESS READER ##") << endl;
    debug << F("address: ") << (int)m_address << endl;
    debug << F("state: ") << (int)m_state << endl;
    debug << F("feature level: ") << (int)m_feature_level << endl;
    debug << F("country/currency: ") << m_country << endl;
    debug << F("scale factor: ") << (int)m_scaleFactor << endl;
    debug << F("decimal places: ") << (int)m_decimalPlaces << endl;
    debug << F("available funds: ") << m_availableFunds << endl;
    debug << F("approved amount: ") << m_approvedAmount << endl;
}
