#pragma once

#include <Arduino.h>

class MotorArray
{
public:
    static const uint8_t MAX_MOTORS = 64;

    enum Result
    {
        IDLE,
        RUNNING,
        HOME_CONFIRMED,
        INVALID_MOTOR,
        MOTOR_NOT_HOME,
        TIMED_OUT
    };

    MotorArray(uint8_t motorCount,
               uint8_t outputDataPin, uint8_t outputClockPin,
               uint8_t outputLatchPin, uint8_t outputEnablePin,
               uint8_t homeDataPin, uint8_t homeClockPin,
               uint8_t homeLoadPin,
               bool outputActiveHigh = true,
               bool homeActiveLow = true,
               unsigned long cycleTimeoutMs = 8000,
               unsigned long homeFilterMs = 30);

    void Begin();
    Result Start(uint8_t motorIndex, unsigned long timeoutMs = 0);
    Result Update();
    void Stop();

    bool IsHome(uint8_t motorIndex);
    uint8_t GetMotorCount() const { return m_motorCount; }
    uint8_t GetActiveMotor() const { return m_activeMotor; }
    Result GetResult() const { return m_result; }

private:
    enum CycleState
    {
        CYCLE_IDLE,
        WAITING_TO_LEAVE_HOME,
        WAITING_TO_RETURN_HOME
    };

    void setOnlyMotor(uint8_t motorIndex);
    void clearOutputs();
    void flushOutputs();
    void setOutputEnable(bool enabled);
    bool readHomeBit(uint8_t motorIndex);

    uint8_t m_motorCount;
    uint8_t m_outputDataPin;
    uint8_t m_outputClockPin;
    uint8_t m_outputLatchPin;
    uint8_t m_outputEnablePin;
    uint8_t m_homeDataPin;
    uint8_t m_homeClockPin;
    uint8_t m_homeLoadPin;
    bool m_outputActiveHigh;
    bool m_homeActiveLow;
    unsigned long m_cycleTimeoutMs;
    unsigned long m_activeTimeoutMs;
    unsigned long m_homeFilterMs;

    uint8_t m_outputs[8];
    uint8_t m_activeMotor;
    unsigned long m_startedAt;
    unsigned long m_homeChangedAt;
    bool m_lastHomeRaw;
    CycleState m_cycleState;
    Result m_result;
};
