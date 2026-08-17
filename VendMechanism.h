#pragma once

#include <Arduino.h>

class VendMechanism
{
public:
    enum Result
    {
        IDLE,
        RUNNING,
        DROP_CONFIRMED,
        TIMED_OUT,
        SENSOR_BLOCKED
    };

    VendMechanism(uint8_t motorPin, uint8_t beamPin,
                  bool motorActiveHigh = true,
                  bool beamBreakActiveLow = true,
                  unsigned long timeoutMs = 5000,
                  unsigned long beamFilterMs = 40);

    void Begin();
    Result Start();
    Result Update();
    void Stop();

    Result GetResult() const { return m_result; }
    bool IsBeamBroken() const;

private:
    void setMotor(bool running);

    uint8_t m_motorPin;
    uint8_t m_beamPin;
    bool m_motorActiveHigh;
    bool m_beamBreakActiveLow;
    unsigned long m_timeoutMs;
    unsigned long m_beamFilterMs;
    unsigned long m_startedAt;
    unsigned long m_beamChangedAt;
    bool m_lastBeamBroken;
    Result m_result;
};
