#pragma once
#include <gmock/gmock.h>
#include "IGPIO.h"

namespace drivers::gpio
{

class IGPIOMock : public IGPIO
{
public:
    MOCK_METHOD(bool, configure, (const Config& config), (override));
    MOCK_METHOD(bool, setState, (State state), (override));
    MOCK_METHOD(std::optional<State>, getState, (), (override));
    MOCK_METHOD(bool, enableInterrupts, (GPIOListener* listener, InterruptEdge edge), (override));
    MOCK_METHOD(bool, disableInterrupts, (), (override));
    MOCK_METHOD(bool, setPullMode, (PullMode mode), (override));
    MOCK_METHOD(bool, setDirection, (Direction direction), (override));
};

} // namespace drivers::gpio
