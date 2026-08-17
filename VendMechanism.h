#pragma once

#include <Arduino.h>
#include "MotorArray.h"

class VendMechanism
{
public:
    enum Result
    {
        IDLE,
        RUNNING,
        DROP_CONFIRMED,
        TIMED_OUT,
        SENSOR_BLOCKED,
        MOTOR_NOT_HOME,
        MOTOR_FAULT
    };

    VendMechanism(MotorArray &motors, uint8_t beamPin,
                  bool beamBreakActiveLow = true,
                  unsigned long beamFilterMs = 40,
                  unsigned long dropGraceMs = 1200);

    void Begin();
    Result Start(uint8_t motorIndex);
    Result Update();
    void Stop();

    Result GetResult() const { return m_result; }
    bool IsBeamBroken() const;
    uint8_t GetActiveMotor() const { return m_motors.GetActiveMotor(); }

private:
    MotorArray &m_motors;
    uint8_t m_beamPin;
    bool m_beamBreakActiveLow;
    unsigned long m_beamFilterMs;
    unsigned long m_dropGraceMs;
    unsigned long m_beamChangedAt;
    unsigned long m_homeConfirmedAt;
    bool m_lastBeamBroken;
    bool m_dropSeen;
    bool m_homeConfirmed;
    Result m_result;
};
