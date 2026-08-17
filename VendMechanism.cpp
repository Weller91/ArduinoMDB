#include "VendMechanism.h"

VendMechanism::VendMechanism(uint8_t motorPin, uint8_t beamPin,
                             bool motorActiveHigh,
                             bool beamBreakActiveLow,
                             unsigned long timeoutMs,
                             unsigned long beamFilterMs)
    : m_motorPin(motorPin),
      m_beamPin(beamPin),
      m_motorActiveHigh(motorActiveHigh),
      m_beamBreakActiveLow(beamBreakActiveLow),
      m_timeoutMs(timeoutMs),
      m_beamFilterMs(beamFilterMs),
      m_startedAt(0),
      m_beamChangedAt(0),
      m_lastBeamBroken(false),
      m_result(IDLE)
{
}

void VendMechanism::Begin()
{
    pinMode(m_motorPin, OUTPUT);
    pinMode(m_beamPin, INPUT_PULLUP);
    setMotor(false);
    m_lastBeamBroken = IsBeamBroken();
    m_result = IDLE;
}

VendMechanism::Result VendMechanism::Start()
{
    if (IsBeamBroken())
    {
        setMotor(false);
        m_result = SENSOR_BLOCKED;
        return m_result;
    }

    m_startedAt = millis();
    m_beamChangedAt = m_startedAt;
    m_lastBeamBroken = false;
    m_result = RUNNING;
    setMotor(true);
    return m_result;
}

VendMechanism::Result VendMechanism::Update()
{
    if (m_result != RUNNING)
        return m_result;

    unsigned long now = millis();
    bool broken = IsBeamBroken();

    if (broken != m_lastBeamBroken)
    {
        m_lastBeamBroken = broken;
        m_beamChangedAt = now;
    }

    if (broken && (now - m_beamChangedAt) >= m_beamFilterMs)
    {
        setMotor(false);
        m_result = DROP_CONFIRMED;
        return m_result;
    }

    if ((now - m_startedAt) >= m_timeoutMs)
    {
        setMotor(false);
        m_result = TIMED_OUT;
    }

    return m_result;
}

void VendMechanism::Stop()
{
    setMotor(false);
    m_result = IDLE;
}

bool VendMechanism::IsBeamBroken() const
{
    bool inputHigh = digitalRead(m_beamPin) == HIGH;
    return m_beamBreakActiveLow ? !inputHigh : inputHigh;
}

void VendMechanism::setMotor(bool running)
{
    bool outputHigh = running ? m_motorActiveHigh : !m_motorActiveHigh;
    digitalWrite(m_motorPin, outputHigh ? HIGH : LOW);
}
