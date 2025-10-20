#include "gmock/gmock.h"
#include "IMCP23017.h"

namespace drivers::gpio::mcp23017
{

class IMCP23017Mock : public IMCP23017 {
public:
   MOCK_METHOD(bool, probe, (), (override));
   MOCK_METHOD(void, reset, (), (override));
   MOCK_METHOD(bool, setDirection, (PIN, Direction), (override));
   MOCK_METHOD(bool, setPolarity, (PIN, Polarity), (override));
   MOCK_METHOD(bool, setPullup, (PIN, Pullup), (override));
   MOCK_METHOD(bool, setInterruptCompare, (PIN, InterruptCompare), (override));
   MOCK_METHOD(bool, writePin, (PIN, bool), (override));
   MOCK_METHOD(std::optional<bool>, readPin, (PIN), (override));
   MOCK_METHOD(void, setListener, (PIN, Listener, void*), (override));
   MOCK_METHOD(void, removeListener, (PIN), (override));
};

} // namespace drivers::gpio::mcp23017
