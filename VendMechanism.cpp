#include "VendMechanism.h"

VendMechanism::VendMechanism(MotorArray &motors, uint8_t beamPin,
                             bool beamBreakActiveLow,
                             unsigned long beamFilterMs,
                             unsigned long dropGraceMs)
    : m_motors(motors),
      m_beamPin(beamPin),
      m_beamBreakActiveLow(beamBreakActiveLow),
      m_beamFilterMs(beamFilterMs),
      m_dropGraceMs(dropGraceMs),
      m_beamChangedAt(0),
      m_homeConfirmedAt(0),
      m_lastBeamBroken(false),
      m_dropSeen(false),
      m_homeConfirmed(false),
      m_result(IDLE)
{
}

void VendMechanism::Begin()
{
    pinMode(m_beamPin, INPUT_PULLUP);
    m_motors.Begin();
    m_lastBeamBroken = IsBeamBroken();
    m_result = IDLE;
}

VendMechanism::Result VendMechanism::Start(uint8_t motorIndex, unsigned long motorTimeoutMs)
{
    if (IsBeamBroken())
    {
        m_result = SENSOR_BLOCKED;
        return m_result;
    }

    MotorArray::Result motorResult = m_motors.Start(motorIndex, motorTimeoutMs);
    if (motorResult == MotorArray::MOTOR_NOT_HOME)
    {
        m_result = MOTOR_NOT_HOME;
        return m_result;
    }
    if (motorResult != MotorArray::RUNNING)
    {
        m_result = MOTOR_FAULT;
        return m_result;
    }

    m_beamChangedAt = millis();
    m_homeConfirmedAt = 0;
    m_lastBeamBroken = false;
    m_dropSeen = false;
    m_homeConfirmed = false;
    m_result = RUNNING;
    return m_result;
}

VendMechanism::Result VendMechanism::Update()
{
    if (m_result != RUNNING)
        return m_result;

    unsigned long now = millis();
    bool beamBroken = IsBeamBroken();

    if (beamBroken != m_lastBeamBroken)
    {
        m_lastBeamBroken = beamBroken;
        m_beamChangedAt = now;
    }

    if (beamBroken && (now - m_beamChangedAt) >= m_beamFilterMs)
        m_dropSeen = true;

    MotorArray::Result motorResult = m_motors.Update();
    if (motorResult == MotorArray::HOME_CONFIRMED && !m_homeConfirmed)
    {
        m_homeConfirmed = true;
        m_homeConfirmedAt = now;
    }
    else if (motorResult == MotorArray::TIMED_OUT ||
             motorResult == MotorArray::MOTOR_NOT_HOME ||
             motorResult == MotorArray::INVALID_MOTOR)
    {
        Stop();
        m_result = MOTOR_FAULT;
        return m_result;
    }

    if (m_homeConfirmed && m_dropSeen)
    {
        Stop();
        m_result = DROP_CONFIRMED;
        return m_result;
    }

    if (m_homeConfirmed && (now - m_homeConfirmedAt) >= m_dropGraceMs)
    {
        Stop();
        m_result = TIMED_OUT;
    }

    return m_result;
}

void VendMechanism::Stop()
{
    m_motors.Stop();
    if (m_result == RUNNING)
        m_result = IDLE;
}

bool VendMechanism::IsBeamBroken() const
{
    bool inputHigh = digitalRead(m_beamPin) == HIGH;
    return m_beamBreakActiveLow ? !inputHigh : inputHigh;
}
