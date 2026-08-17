#include "MotorArray.h"

#include <string.h>

MotorArray::MotorArray(uint8_t motorCount,
                       uint8_t outputDataPin, uint8_t outputClockPin,
                       uint8_t outputLatchPin, uint8_t outputEnablePin,
                       uint8_t homeDataPin, uint8_t homeClockPin,
                       uint8_t homeLoadPin,
                       bool outputActiveHigh,
                       bool homeActiveLow,
                       unsigned long cycleTimeoutMs,
                       unsigned long homeFilterMs)
    : m_motorCount(motorCount > MAX_MOTORS ? MAX_MOTORS : motorCount),
      m_outputDataPin(outputDataPin),
      m_outputClockPin(outputClockPin),
      m_outputLatchPin(outputLatchPin),
      m_outputEnablePin(outputEnablePin),
      m_homeDataPin(homeDataPin),
      m_homeClockPin(homeClockPin),
      m_homeLoadPin(homeLoadPin),
      m_outputActiveHigh(outputActiveHigh),
      m_homeActiveLow(homeActiveLow),
      m_cycleTimeoutMs(cycleTimeoutMs),
      m_activeTimeoutMs(cycleTimeoutMs),
      m_homeFilterMs(homeFilterMs),
      m_activeMotor(0xFF),
      m_startedAt(0),
      m_homeChangedAt(0),
      m_lastHomeRaw(false),
      m_cycleState(CYCLE_IDLE),
      m_result(IDLE)
{
    clearOutputs();
}

void MotorArray::Begin()
{
    pinMode(m_outputDataPin, OUTPUT);
    pinMode(m_outputClockPin, OUTPUT);
    pinMode(m_outputLatchPin, OUTPUT);
    pinMode(m_outputEnablePin, OUTPUT);

    pinMode(m_homeDataPin, INPUT);
    pinMode(m_homeClockPin, OUTPUT);
    pinMode(m_homeLoadPin, OUTPUT);

    digitalWrite(m_outputClockPin, LOW);
    digitalWrite(m_outputLatchPin, HIGH);
    digitalWrite(m_homeClockPin, LOW);
    digitalWrite(m_homeLoadPin, HIGH);

    clearOutputs();
    flushOutputs();
    setOutputEnable(false);
}

MotorArray::Result MotorArray::Start(uint8_t motorIndex, unsigned long timeoutMs)
{
    if (motorIndex >= m_motorCount)
    {
        m_result = INVALID_MOTOR;
        return m_result;
    }

    if (!IsHome(motorIndex))
    {
        m_result = MOTOR_NOT_HOME;
        return m_result;
    }

    m_activeMotor = motorIndex;
    m_activeTimeoutMs = timeoutMs >= 250 ? timeoutMs : m_cycleTimeoutMs;
    m_startedAt = millis();
    m_homeChangedAt = m_startedAt;
    m_lastHomeRaw = true;
    m_cycleState = WAITING_TO_LEAVE_HOME;
    m_result = RUNNING;

    setOnlyMotor(motorIndex);
    setOutputEnable(true);
    return m_result;
}

MotorArray::Result MotorArray::Update()
{
    if (m_result != RUNNING || m_activeMotor >= m_motorCount)
        return m_result;

    unsigned long now = millis();
    bool home = IsHome(m_activeMotor);

    if (home != m_lastHomeRaw)
    {
        m_lastHomeRaw = home;
        m_homeChangedAt = now;
    }

    if ((now - m_homeChangedAt) >= m_homeFilterMs)
    {
        if (m_cycleState == WAITING_TO_LEAVE_HOME && !home)
        {
            m_cycleState = WAITING_TO_RETURN_HOME;
        }
        else if (m_cycleState == WAITING_TO_RETURN_HOME && home)
        {
            Stop();
            m_result = HOME_CONFIRMED;
            return m_result;
        }
    }

    if ((now - m_startedAt) >= m_activeTimeoutMs)
    {
        Stop();
        m_result = TIMED_OUT;
    }

    return m_result;
}

void MotorArray::Stop()
{
    clearOutputs();
    flushOutputs();
    setOutputEnable(false);
    m_activeMotor = 0xFF;
    m_cycleState = CYCLE_IDLE;

    if (m_result == RUNNING)
        m_result = IDLE;
}

bool MotorArray::IsHome(uint8_t motorIndex)
{
    if (motorIndex >= m_motorCount)
        return false;
    return readHomeBit(motorIndex);
}

void MotorArray::setOnlyMotor(uint8_t motorIndex)
{
    clearOutputs();

    uint8_t byteIndex = motorIndex / 8;
    uint8_t bitIndex = motorIndex % 8;
    m_outputs[byteIndex] |= (1U << bitIndex);
    flushOutputs();
}

void MotorArray::clearOutputs()
{
    memset(m_outputs, 0, sizeof(m_outputs));
}

void MotorArray::flushOutputs()
{
    uint8_t registerCount = (m_motorCount + 7) / 8;
    digitalWrite(m_outputLatchPin, LOW);

    for (int8_t byteIndex = registerCount - 1; byteIndex >= 0; --byteIndex)
    {
        uint8_t value = m_outputActiveHigh
            ? m_outputs[byteIndex]
            : (uint8_t)~m_outputs[byteIndex];

        for (int8_t bit = 7; bit >= 0; --bit)
        {
            digitalWrite(m_outputClockPin, LOW);
            digitalWrite(m_outputDataPin, (value >> bit) & 0x01);
            digitalWrite(m_outputClockPin, HIGH);
        }
    }

    digitalWrite(m_outputLatchPin, HIGH);
    digitalWrite(m_outputClockPin, LOW);
}

void MotorArray::setOutputEnable(bool enabled)
{
    // 74HC595 OE is active LOW. This pin is a hardware-wide safety inhibit.
    digitalWrite(m_outputEnablePin, enabled ? LOW : HIGH);
}

bool MotorArray::readHomeBit(uint8_t motorIndex)
{
    digitalWrite(m_homeLoadPin, LOW);
    delayMicroseconds(2);
    digitalWrite(m_homeLoadPin, HIGH);
    delayMicroseconds(2);

    // A 74HC165 presents H/D7 first after parallel load. Reverse the bit
    // position inside each register so software channel 0 maps to A/D0,
    // channel 1 to B/D1, and so on.
    uint8_t registerIndex = motorIndex / 8;
    uint8_t channelInRegister = motorIndex % 8;
    uint8_t shiftPosition = (registerIndex * 8) + (7 - channelInRegister);

    bool raw = false;
    for (uint8_t position = 0; position <= shiftPosition; ++position)
    {
        raw = digitalRead(m_homeDataPin) == HIGH;
        digitalWrite(m_homeClockPin, HIGH);
        delayMicroseconds(1);
        digitalWrite(m_homeClockPin, LOW);
    }

    return m_homeActiveLow ? !raw : raw;
}
