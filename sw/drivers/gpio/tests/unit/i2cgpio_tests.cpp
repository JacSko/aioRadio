#include <cstring>

#include "i2cGPIO.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "IMCP23017Mock.h"

using namespace testing;

struct GPIOListenerMock : public drivers::gpio::GPIOListener
{
   MOCK_METHOD(void, onInterrupt, (drivers::gpio::GPIO id, drivers::gpio::State state), (override));
};

struct i2cGPIOFixture: public ::testing::Test
{
public:
   virtual void SetUp() override
   {
      rtos_mock_init();
      m_testSubject = std::make_unique<drivers::gpio::i2cGPIO>(m_testGPIO, m_driverMock, "TEST_NAME");
      configureDefaultState();
   }

   virtual void TearDown() override
   {
      EXPECT_CALL(m_driverMock, removeListener(m_gpioMask));
      m_testSubject.reset();
      rtos_mock_deinit();
   }
   virtual void configureDefaultState(){}
   drivers::gpio::Config getDefaultConfig()
   {
      drivers::gpio::Config config;
      config.direction = drivers::gpio::Direction::INPUT;
      config.pullMode = drivers::gpio::PullMode::NONE;
      config.state = drivers::gpio::State::LOW;
      config.listener = nullptr;
      config.interruptEdge = drivers::gpio::InterruptEdge::RISING;
      return config;
   }
   std::unique_ptr<drivers::gpio::IGPIO> m_testSubject;
   const uint16_t m_testGPIO = 0xA1;
   const drivers::gpio::mcp23017::PIN m_gpioMask = drivers::gpio::mcp23017::PIN::GPIOA1;
   taskControlBlock m_testTaskHandle;
   QueueDefinition m_testQueueHandle;
   GPIOListenerMock m_listenerMock;
   drivers::gpio::mcp23017::IMCP23017Mock m_driverMock;
};

class i2cGPIOInputInterruptRisingEdgeFixture : public i2cGPIOFixture
{
public:
   virtual void configureDefaultState() override
   {
      auto config = getDefaultConfig();
      config.listener = &m_listenerMock;
      config.interruptEdge = drivers::gpio::InterruptEdge::RISING;

      EXPECT_CALL(m_driverMock, setDirection(m_gpioMask, _)).WillOnce(Return(true));
      EXPECT_CALL(m_driverMock, setPullup(m_gpioMask, _)).WillOnce(Return(true));
      EXPECT_CALL(m_driverMock, setInterruptCompare(m_gpioMask, drivers::gpio::mcp23017::InterruptCompare::COMPARE_PREVIOUS));
      EXPECT_CALL(*rtos_get_mock(), xQueueCreate(_, _)).WillOnce(Return(&m_testQueueHandle));
      EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_, _, _, _, _, _)).WillOnce(Return(pdTRUE));
      EXPECT_CALL(m_driverMock, readPin(m_gpioMask)).WillOnce(Return(true));
      EXPECT_TRUE(m_testSubject->configure(config));
   }
};
class i2cGPIOInputInterruptFallingEdgeFixture : public i2cGPIOFixture
{
public:
   virtual void configureDefaultState() override
   {
      auto config = getDefaultConfig();
      config.listener = &m_listenerMock;
      config.interruptEdge = drivers::gpio::InterruptEdge::FALLING;

      EXPECT_CALL(m_driverMock, setDirection(m_gpioMask, _)).WillOnce(Return(true));
      EXPECT_CALL(m_driverMock, setPullup(m_gpioMask, _)).WillOnce(Return(true));
      EXPECT_CALL(m_driverMock, setInterruptCompare(m_gpioMask, drivers::gpio::mcp23017::InterruptCompare::COMPARE_PREVIOUS));
      EXPECT_CALL(*rtos_get_mock(), xQueueCreate(_, _)).WillOnce(Return(&m_testQueueHandle));
      EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_, _, _, _, _, _)).WillOnce(Return(pdTRUE));
      EXPECT_CALL(m_driverMock, readPin(m_gpioMask)).WillOnce(Return(true));
      EXPECT_TRUE(m_testSubject->configure(config));
   }
};

TEST_F(i2cGPIOFixture, configure_pullDownRequested_notConfigured)
{
   auto config = getDefaultConfig();
   config.pullMode = drivers::gpio::PullMode::PULL_DOWN;
   EXPECT_FALSE(m_testSubject->configure(config));
}

TEST_F(i2cGPIOFixture, configure_ListenerProvidedForOutput_notConfigured)
{
   auto config = getDefaultConfig();
   config.direction = drivers::gpio::Direction::OUTPUT;
   config.listener = &m_listenerMock;

   EXPECT_FALSE(m_testSubject->configure(config));
}

TEST_F(i2cGPIOFixture, setState_setsHighAndLow)
{
   EXPECT_CALL(m_driverMock, writePin(m_gpioMask, true)).WillOnce(Return(true));
   EXPECT_TRUE(m_testSubject->setState(drivers::gpio::State::HIGH));

   EXPECT_CALL(m_driverMock, writePin(m_gpioMask, false)).WillOnce(Return(true));
   EXPECT_TRUE(m_testSubject->setState(drivers::gpio::State::LOW));
}

TEST_F(i2cGPIOFixture, setState_gpioSetLevelFails_returnsFalse)
{
   EXPECT_CALL(m_driverMock, writePin(m_gpioMask, true)).WillOnce(Return(false));
   EXPECT_FALSE(m_testSubject->setState(drivers::gpio::State::HIGH));
}

TEST_F(i2cGPIOFixture, getState_returnsHighAndLow)
{
   EXPECT_CALL(m_driverMock, readPin(m_gpioMask)).WillOnce(Return(true));
   auto state = m_testSubject->getState();
   ASSERT_TRUE(state.has_value());
   EXPECT_EQ(state.value(), drivers::gpio::State::HIGH);

   EXPECT_CALL(m_driverMock, readPin(m_gpioMask)).WillOnce(Return(false));
   state = m_testSubject->getState();
   ASSERT_TRUE(state.has_value());
   EXPECT_EQ(state.value(), drivers::gpio::State::LOW);
}

TEST_F(i2cGPIOFixture, getState_gpioGetLevelFails_returnsNullopt)
{
   EXPECT_CALL(m_driverMock, readPin(m_gpioMask)).WillOnce(Return(std::nullopt));
   auto state = m_testSubject->getState();
   EXPECT_FALSE(state.has_value());
}

TEST_F(i2cGPIOFixture, setPullMode_validModes)
{
   EXPECT_CALL(m_driverMock, setPullup(m_gpioMask, ::drivers::gpio::mcp23017::Pullup::DISABLED)).WillOnce(Return(true));
   EXPECT_TRUE(m_testSubject->setPullMode(drivers::gpio::PullMode::NONE));

   EXPECT_CALL(m_driverMock, setPullup(m_gpioMask, ::drivers::gpio::mcp23017::Pullup::ENABLED)).WillOnce(Return(true));
   EXPECT_TRUE(m_testSubject->setPullMode(drivers::gpio::PullMode::PULL_UP));
}

TEST_F(i2cGPIOFixture, setPullMode_invalidMode_returnsFalse)
{
   // Simulate TableConverter returning std::nullopt for an invalid mode
   EXPECT_FALSE(m_testSubject->setPullMode(static_cast<drivers::gpio::PullMode>(-1)));

   EXPECT_CALL(m_driverMock, setPullup(m_gpioMask, _)).Times(0);
   EXPECT_FALSE(m_testSubject->setPullMode(drivers::gpio::PullMode::PULL_DOWN));
}

TEST_F(i2cGPIOFixture, setDirection_setsInputAndOutput)
{
   EXPECT_CALL(m_driverMock, setDirection(m_gpioMask, drivers::gpio::mcp23017::Direction::INPUT)).WillOnce(Return(true));
   EXPECT_TRUE(m_testSubject->setDirection(drivers::gpio::Direction::INPUT));

   EXPECT_CALL(m_driverMock, setDirection(m_gpioMask, drivers::gpio::mcp23017::Direction::OUTPUT)).WillOnce(Return(true));
   EXPECT_TRUE(m_testSubject->setDirection(drivers::gpio::Direction::OUTPUT));
}

TEST_F(i2cGPIOFixture, setDirection_gpioSetDirectionFails_returnsFalse)
{
   EXPECT_CALL(m_driverMock, setDirection(m_gpioMask, _)).WillOnce(Return(false));
   EXPECT_FALSE(m_testSubject->setDirection(drivers::gpio::Direction::INPUT));
}

TEST_F(i2cGPIOFixture, enableInterrupts_success)
{
   EXPECT_CALL(m_driverMock, setInterruptCompare(m_gpioMask, drivers::gpio::mcp23017::InterruptCompare::COMPARE_PREVIOUS));
   EXPECT_CALL(m_driverMock, setListener(m_gpioMask, _, _));
   EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_, _, _, _, _, _)).WillOnce(Return(pdTRUE));
   EXPECT_CALL(*rtos_get_mock(), xQueueCreate(_, _)).WillOnce(Return(&m_testQueueHandle));
   EXPECT_TRUE(m_testSubject->enableInterrupts(&m_listenerMock, drivers::gpio::InterruptEdge::RISING));

   EXPECT_CALL(*rtos_get_mock(), vQueueDelete(&m_testQueueHandle));
}

TEST_F(i2cGPIOFixture, enableInterrupts_invalidDirection_returnsFalse)
{
   auto config = getDefaultConfig();
   config.direction = drivers::gpio::Direction::OUTPUT;
   m_testSubject->configure(config);

   EXPECT_FALSE(m_testSubject->enableInterrupts(&m_listenerMock, drivers::gpio::InterruptEdge::RISING));
}

TEST_F(i2cGPIOFixture, enableInterrupts_nullListener_returnsFalse)
{
   EXPECT_FALSE(m_testSubject->enableInterrupts(nullptr, drivers::gpio::InterruptEdge::RISING));
}

TEST_F(i2cGPIOFixture, enableInterrupts_createQueueFails_returnsFalse)
{
   EXPECT_CALL(*rtos_get_mock(), xQueueCreate(_, _)).WillOnce(Return(nullptr));
   EXPECT_FALSE(m_testSubject->enableInterrupts(&m_listenerMock, drivers::gpio::InterruptEdge::RISING));
}

TEST_F(i2cGPIOFixture, enableInterrupts_createTaskFails_returnsFalse)
{
   EXPECT_CALL(*rtos_get_mock(), xQueueCreate(_, _)).WillOnce(Return(&m_testQueueHandle));
   EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_, _, _, _, _, _)).WillOnce(Return(pdFALSE));
   EXPECT_CALL(*rtos_get_mock(), vQueueDelete(&m_testQueueHandle));
   EXPECT_FALSE(m_testSubject->enableInterrupts(&m_listenerMock, drivers::gpio::InterruptEdge::RISING));
}

TEST_F(i2cGPIOFixture, disableInterrupts_success)
{
   EXPECT_CALL(m_driverMock, removeListener(m_gpioMask));
   EXPECT_TRUE(m_testSubject->disableInterrupts());
}

TEST_F(i2cGPIOFixture, configure_validInputConfig_success)
{
   auto config = getDefaultConfig();
   EXPECT_CALL(m_driverMock, setDirection(m_gpioMask, drivers::gpio::mcp23017::Direction::INPUT)).WillOnce(Return(true));
   EXPECT_CALL(m_driverMock, setPullup(m_gpioMask, drivers::gpio::mcp23017::Pullup::DISABLED)).WillOnce(Return(true));
   EXPECT_CALL(m_driverMock, removeListener(m_gpioMask));
   EXPECT_TRUE(m_testSubject->configure(config));
}

TEST_F(i2cGPIOFixture, configure_validOutputConfig_success)
{
   auto config = getDefaultConfig();
   config.direction = drivers::gpio::Direction::OUTPUT;
   EXPECT_CALL(m_driverMock, setDirection(m_gpioMask, drivers::gpio::mcp23017::Direction::OUTPUT)).WillOnce(Return(true));
   EXPECT_CALL(m_driverMock, setPullup(m_gpioMask, drivers::gpio::mcp23017::Pullup::DISABLED)).WillOnce(Return(true));
   EXPECT_CALL(m_driverMock, writePin(m_gpioMask, false)).WillOnce(Return(true));
   EXPECT_CALL(m_driverMock, removeListener(m_gpioMask));
   EXPECT_TRUE(m_testSubject->configure(config));
}

TEST_F(i2cGPIOFixture, configure_listenerProvided_enablesInterrupts)
{
   auto config = getDefaultConfig();
   config.listener = &m_listenerMock;
   EXPECT_CALL(m_driverMock, setDirection(m_gpioMask, drivers::gpio::mcp23017::Direction::INPUT)).WillOnce(Return(true));
   EXPECT_CALL(m_driverMock, setPullup(m_gpioMask, drivers::gpio::mcp23017::Pullup::DISABLED)).WillOnce(Return(true));
   EXPECT_CALL(*rtos_get_mock(), xQueueCreate(_, _)).WillOnce(Return(&m_testQueueHandle));
   EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_, _, _, _, _, _)).WillOnce(Return(pdTRUE));
   EXPECT_CALL(m_driverMock, setInterruptCompare(m_gpioMask, drivers::gpio::mcp23017::InterruptCompare::COMPARE_PREVIOUS));
   EXPECT_CALL(m_driverMock, setListener(m_gpioMask, _, _));
   EXPECT_TRUE(m_testSubject->configure(config));

   EXPECT_CALL(*rtos_get_mock(), vQueueDelete(&m_testQueueHandle));
}

TEST_F(i2cGPIOFixture, configure_outputConfig_callsSetState)
{
   auto config = getDefaultConfig();
   config.direction = drivers::gpio::Direction::OUTPUT;
   config.state = drivers::gpio::State::HIGH;
   EXPECT_CALL(m_driverMock, setDirection(m_gpioMask, drivers::gpio::mcp23017::Direction::OUTPUT)).WillOnce(Return(true));
   EXPECT_CALL(m_driverMock, setPullup(m_gpioMask, drivers::gpio::mcp23017::Pullup::DISABLED)).WillOnce(Return(true));
   EXPECT_CALL(m_driverMock, writePin(m_gpioMask, true)).WillOnce(Return(true));
   EXPECT_CALL(m_driverMock, removeListener(m_gpioMask));
   EXPECT_TRUE(m_testSubject->configure(config));
}

TEST_F(i2cGPIOFixture, configure_setDirectionFails_returnsFalse)
{
   auto config = getDefaultConfig();
   EXPECT_CALL(m_driverMock, setDirection(m_gpioMask, _)).WillOnce(Return(false));
   EXPECT_FALSE(m_testSubject->configure(config));
}

TEST_F(i2cGPIOFixture, configure_setPullModeFails_returnsFalse)
{
   auto config = getDefaultConfig();
   EXPECT_CALL(m_driverMock, setDirection(m_gpioMask, _)).WillOnce(Return(true));
   EXPECT_CALL(m_driverMock, setPullup(m_gpioMask, _)).WillOnce(Return(false));
   EXPECT_FALSE(m_testSubject->configure(config));
}

TEST_F(i2cGPIOInputInterruptRisingEdgeFixture, lowStateNotified_noCallbackCalled)
{
   auto rawTestSubject = static_cast<drivers::gpio::i2cGPIO*>(m_testSubject.get());
   std::pair<drivers::gpio::mcp23017::PIN, bool> interruptEvent = std::make_pair(m_gpioMask, false);
   EXPECT_CALL(*rtos_get_mock(), xQueueReceive(&m_testQueueHandle, _, _))
       .WillOnce([&](QueueHandle_t, void* pvBuffer, uint32_t)->BaseType_t {
           std::memcpy(pvBuffer, &interruptEvent, sizeof(interruptEvent));
           return pdTRUE;
       });
   EXPECT_CALL(m_listenerMock, onInterrupt(_, _)).Times(0);
   rawTestSubject->handleInterrupt();
}

TEST_F(i2cGPIOInputInterruptRisingEdgeFixture, highStateNotified_noStateChange_noCallbackCalled)
{
   auto rawTestSubject = static_cast<drivers::gpio::i2cGPIO*>(m_testSubject.get());
   std::pair<drivers::gpio::mcp23017::PIN, bool> interruptEvent = std::make_pair(m_gpioMask, true);
   EXPECT_CALL(*rtos_get_mock(), xQueueReceive(&m_testQueueHandle, _, _))
       .WillOnce([&](QueueHandle_t, void* pvBuffer, uint32_t)->BaseType_t {
           std::memcpy(pvBuffer, &interruptEvent, sizeof(interruptEvent));
           return pdTRUE;
       });
   EXPECT_CALL(m_listenerMock, onInterrupt(_, _)).Times(0);
   rawTestSubject->handleInterrupt();
}

TEST_F(i2cGPIOInputInterruptRisingEdgeFixture, highandLowStateNotified_callbackCalled)
{
   auto rawTestSubject = static_cast<drivers::gpio::i2cGPIO*>(m_testSubject.get());
   std::pair<drivers::gpio::mcp23017::PIN, bool> interruptEvent = std::make_pair(m_gpioMask, false);
   EXPECT_CALL(*rtos_get_mock(), xQueueReceive(&m_testQueueHandle, _, _))
       .WillOnce([&](QueueHandle_t, void* pvBuffer, uint32_t)->BaseType_t {
           std::memcpy(pvBuffer, &interruptEvent, sizeof(interruptEvent));
           return pdTRUE;
       });
   EXPECT_CALL(m_listenerMock, onInterrupt(_, _)).Times(0);
   rawTestSubject->handleInterrupt();

   interruptEvent.second = true;
   EXPECT_CALL(*rtos_get_mock(), xQueueReceive(&m_testQueueHandle, _, _))
       .WillOnce([&](QueueHandle_t, void* pvBuffer, uint32_t)->BaseType_t {
           std::memcpy(pvBuffer, &interruptEvent, sizeof(interruptEvent));
           return pdTRUE;
       });
   EXPECT_CALL(m_listenerMock, onInterrupt(static_cast<drivers::gpio::GPIO>(m_testGPIO), drivers::gpio::State::HIGH));
   rawTestSubject->handleInterrupt();
}

TEST_F(i2cGPIOInputInterruptFallingEdgeFixture, highStateNotified_noCallbackCalled)
{
   auto rawTestSubject = static_cast<drivers::gpio::i2cGPIO*>(m_testSubject.get());
   std::pair<drivers::gpio::mcp23017::PIN, bool> interruptEvent = std::make_pair(m_gpioMask, true);
   EXPECT_CALL(*rtos_get_mock(), xQueueReceive(&m_testQueueHandle, _, _))
       .WillOnce([&](QueueHandle_t, void* pvBuffer, uint32_t)->BaseType_t {
           std::memcpy(pvBuffer, &interruptEvent, sizeof(interruptEvent));
           return pdTRUE;
       });
   EXPECT_CALL(m_listenerMock, onInterrupt(_, _)).Times(0);
   rawTestSubject->handleInterrupt();
}

TEST_F(i2cGPIOInputInterruptFallingEdgeFixture, lowAndHighStateNotified_callbackCalled)
{
   auto rawTestSubject = static_cast<drivers::gpio::i2cGPIO*>(m_testSubject.get());
   std::pair<drivers::gpio::mcp23017::PIN, bool> interruptEvent = std::make_pair(m_gpioMask, false);
   EXPECT_CALL(*rtos_get_mock(), xQueueReceive(&m_testQueueHandle, _, _))
       .WillOnce([&](QueueHandle_t, void* pvBuffer, uint32_t)->BaseType_t {
           std::memcpy(pvBuffer, &interruptEvent, sizeof(interruptEvent));
           return pdTRUE;
       });
   EXPECT_CALL(m_listenerMock, onInterrupt(static_cast<drivers::gpio::GPIO>(m_testGPIO), drivers::gpio::State::LOW));
   rawTestSubject->handleInterrupt();

   interruptEvent.second = true;
   EXPECT_CALL(*rtos_get_mock(), xQueueReceive(&m_testQueueHandle, _, _))
       .WillOnce([&](QueueHandle_t, void* pvBuffer, uint32_t)->BaseType_t {
           std::memcpy(pvBuffer, &interruptEvent, sizeof(interruptEvent));
           return pdTRUE;
       });
   EXPECT_CALL(m_listenerMock, onInterrupt(_,_)).Times(0);
   rawTestSubject->handleInterrupt();
}
