#include "socGPIO.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

//Mocks
#include "driver/gpio.h"

using namespace testing;

struct GPIOListenerMock : public drivers::gpio::GPIOListener
{
   MOCK_METHOD(void, onInterrupt, (drivers::gpio::GPIO id, drivers::gpio::State state), (override));
};

struct socGPIOFixture: public ::testing::Test
{
public:
   virtual void SetUp() override
   {
      gpio_mock_init();
      rtos_mock_init();

      EXPECT_CALL(*gpio_get_mock(), gpio_reset_pin(m_testGPIO))
         .WillRepeatedly(Return(ESP_OK));
      m_testSubject = std::make_unique<drivers::gpio::socGPIO>(m_testGPIO, "TEST_NAME");
   }

   virtual void TearDown() override
   {
      EXPECT_CALL(*gpio_get_mock(), gpio_reset_pin(m_testGPIO))
         .WillOnce(Return(ESP_OK));
      EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_remove(m_testGPIO))
         .WillOnce(Return(ESP_OK));
      EXPECT_CALL(*gpio_get_mock(), gpio_set_intr_type(m_testGPIO, GPIO_INTR_DISABLE))
         .WillOnce(Return(ESP_OK));
      m_testSubject.reset();
      rtos_mock_deinit();
      gpio_mock_deinit();
   }
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
   const gpio_num_t m_testGPIO = GPIO_NUM_1;
   taskControlBlock m_testTaskHandle;
   GPIOListenerMock m_listenerMock;
};

TEST_F(socGPIOFixture, configure_invalidGPIO_notConfigured)
{
   EXPECT_CALL(*gpio_get_mock(), is_valid_gpio(m_testGPIO)).WillOnce(Return(false));
   EXPECT_CALL(*gpio_get_mock(), gpio_config(_)).Times(0);
   EXPECT_FALSE(m_testSubject->configure(getDefaultConfig()));
}

TEST_F(socGPIOFixture, configure_gpioDoesNotSupportOutput_notConfigured)
{
   auto config = getDefaultConfig();
   config.direction = drivers::gpio::Direction::OUTPUT;

   EXPECT_CALL(*gpio_get_mock(), is_valid_gpio(m_testGPIO)).WillOnce(Return(true));
   EXPECT_CALL(*gpio_get_mock(), is_valid_output_gpio(m_testGPIO)).WillOnce(Return(false));
   EXPECT_CALL(*gpio_get_mock(), gpio_config(_)).Times(0);
   EXPECT_FALSE(m_testSubject->configure(config));
}

TEST_F(socGPIOFixture, configure_ListenerProvidedForOutput_notConfigured)
{
   auto config = getDefaultConfig();
   config.direction = drivers::gpio::Direction::OUTPUT;
   config.listener = &m_listenerMock;

   EXPECT_CALL(*gpio_get_mock(), is_valid_gpio(m_testGPIO)).WillOnce(Return(true));
   EXPECT_CALL(*gpio_get_mock(), is_valid_output_gpio(m_testGPIO)).WillOnce(Return(true));
   EXPECT_CALL(*gpio_get_mock(), gpio_config(_)).Times(0);
   EXPECT_FALSE(m_testSubject->configure(config));
}

TEST_F(socGPIOFixture, setState_setsHighAndLow)
{
   EXPECT_CALL(*gpio_get_mock(), gpio_set_level(m_testGPIO, 1)).WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->setState(drivers::gpio::State::HIGH));

   EXPECT_CALL(*gpio_get_mock(), gpio_set_level(m_testGPIO, 0)).WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->setState(drivers::gpio::State::LOW));
}

TEST_F(socGPIOFixture, setState_gpioSetLevelFails_returnsFalse)
{
   EXPECT_CALL(*gpio_get_mock(), gpio_set_level(m_testGPIO, 1)).WillOnce(Return(ESP_FAIL));
   EXPECT_FALSE(m_testSubject->setState(drivers::gpio::State::HIGH));
}

TEST_F(socGPIOFixture, getState_returnsHighAndLow)
{
   EXPECT_CALL(*gpio_get_mock(), gpio_get_level(m_testGPIO)).WillOnce(Return(1));
   auto state = m_testSubject->getState();
   ASSERT_TRUE(state.has_value());
   EXPECT_EQ(state.value(), drivers::gpio::State::HIGH);

   EXPECT_CALL(*gpio_get_mock(), gpio_get_level(m_testGPIO)).WillOnce(Return(0));
   state = m_testSubject->getState();
   ASSERT_TRUE(state.has_value());
   EXPECT_EQ(state.value(), drivers::gpio::State::LOW);
}

TEST_F(socGPIOFixture, getState_gpioGetLevelFails_returnsNullopt)
{
   EXPECT_CALL(*gpio_get_mock(), gpio_get_level(m_testGPIO)).WillOnce(Return(-1));
   auto state = m_testSubject->getState();
   EXPECT_FALSE(state.has_value());
}

TEST_F(socGPIOFixture, setPullMode_validModes)
{
   EXPECT_CALL(*gpio_get_mock(), gpio_set_pull_mode(m_testGPIO, GPIO_FLOATING)).WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->setPullMode(drivers::gpio::PullMode::NONE));

   EXPECT_CALL(*gpio_get_mock(), gpio_set_pull_mode(m_testGPIO, GPIO_PULLUP_ONLY)).WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->setPullMode(drivers::gpio::PullMode::PULL_UP));

   EXPECT_CALL(*gpio_get_mock(), gpio_set_pull_mode(m_testGPIO, GPIO_PULLDOWN_ONLY)).WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->setPullMode(drivers::gpio::PullMode::PULL_DOWN));
}

TEST_F(socGPIOFixture, setPullMode_invalidMode_returnsFalse)
{
   // Simulate TableConverter returning std::nullopt for an invalid mode
   EXPECT_FALSE(m_testSubject->setPullMode(static_cast<drivers::gpio::PullMode>(-1)));
}

TEST_F(socGPIOFixture, setDirection_setsInputAndOutput)
{
   EXPECT_CALL(*gpio_get_mock(), gpio_set_direction(m_testGPIO, GPIO_MODE_INPUT)).WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->setDirection(drivers::gpio::Direction::INPUT));

   EXPECT_CALL(*gpio_get_mock(), gpio_set_direction(m_testGPIO, GPIO_MODE_OUTPUT)).WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->setDirection(drivers::gpio::Direction::OUTPUT));
}

TEST_F(socGPIOFixture, setDirection_gpioSetDirectionFails_returnsFalse)
{
   EXPECT_CALL(*gpio_get_mock(), gpio_set_direction(m_testGPIO, GPIO_MODE_INPUT)).WillOnce(Return(ESP_FAIL));
   EXPECT_FALSE(m_testSubject->setDirection(drivers::gpio::Direction::INPUT));
}

TEST_F(socGPIOFixture, enableInterrupts_success)
{
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_add(m_testGPIO, _, _)).WillOnce(Return(ESP_OK));
   EXPECT_CALL(*gpio_get_mock(), gpio_set_intr_type(m_testGPIO, GPIO_INTR_POSEDGE)).WillOnce(Return(ESP_OK));
   EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_, _, _, _, _, _)).WillOnce(Return(pdTRUE));
   EXPECT_TRUE(m_testSubject->enableInterrupts(&m_listenerMock, drivers::gpio::InterruptEdge::RISING));
}

TEST_F(socGPIOFixture, enableInterrupts_listenerEmpty_interruptNotAttached)
{
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_add(m_testGPIO, _, _)).Times(0);
   EXPECT_FALSE(m_testSubject->enableInterrupts(nullptr, drivers::gpio::InterruptEdge::RISING));
}

TEST_F(socGPIOFixture, enableInterrupts_invalidEdge_returnsFalse)
{
   // Simulate TableConverter returns std::nullopt for invalid edge
   EXPECT_FALSE(m_testSubject->enableInterrupts(&m_listenerMock, static_cast<drivers::gpio::InterruptEdge>(-1)));
}

TEST_F(socGPIOFixture, enableInterrupts_createTaskFails_returnsFalse)
{
   EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_, _, _, _, _, _)).WillOnce(Return(pdFALSE));
   EXPECT_FALSE(m_testSubject->enableInterrupts(&m_listenerMock, drivers::gpio::InterruptEdge::RISING));
}

TEST_F(socGPIOFixture, enableInterrupts_isrHandlerAddFails_returnsFalse)
{
   EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_, _, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<5>(&m_testTaskHandle), Return(pdTRUE)));
   EXPECT_CALL(*rtos_get_mock(), vTaskDelete(&m_testTaskHandle));
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_add(m_testGPIO, _, _)).WillOnce(Return(ESP_FAIL));
   EXPECT_FALSE(m_testSubject->enableInterrupts(&m_listenerMock, drivers::gpio::InterruptEdge::RISING));
}

TEST_F(socGPIOFixture, enableInterrupts_setIntrTypeFails_returnsFalse)
{
   EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_, _, _, _, _, _)).WillOnce(Return(pdTRUE));
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_add(m_testGPIO, _, _)).WillOnce(Return(ESP_OK));
   EXPECT_CALL(*gpio_get_mock(), gpio_set_intr_type(m_testGPIO, GPIO_INTR_POSEDGE)).WillOnce(Return(ESP_FAIL));
   EXPECT_CALL(*gpio_get_mock(), gpio_set_intr_type(m_testGPIO, GPIO_INTR_DISABLE));
   EXPECT_FALSE(m_testSubject->enableInterrupts(&m_listenerMock, drivers::gpio::InterruptEdge::RISING));
}

TEST_F(socGPIOFixture, disableInterrupts_success)
{
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_remove(m_testGPIO)).WillOnce(Return(ESP_OK));
   EXPECT_CALL(*gpio_get_mock(), gpio_set_intr_type(m_testGPIO, GPIO_INTR_DISABLE)).WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->disableInterrupts());
}

TEST_F(socGPIOFixture, disableInterrupts_failure)
{
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_remove(m_testGPIO)).WillOnce(Return(ESP_FAIL));
   EXPECT_FALSE(m_testSubject->disableInterrupts());
}

TEST_F(socGPIOFixture, configure_validInputConfig_success)
{
   auto config = getDefaultConfig();
   EXPECT_CALL(*gpio_get_mock(), is_valid_gpio(m_testGPIO)).WillOnce(Return(true));
   EXPECT_CALL(*gpio_get_mock(), gpio_config(_)).WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->configure(config));
}

TEST_F(socGPIOFixture, configure_gpioConfigFails_returnsFalse)
{
   auto config = getDefaultConfig();
   EXPECT_CALL(*gpio_get_mock(), is_valid_gpio(m_testGPIO)).WillOnce(Return(true));
   EXPECT_CALL(*gpio_get_mock(), gpio_config(_)).WillOnce(Return(ESP_FAIL));
   EXPECT_FALSE(m_testSubject->configure(config));
}

TEST_F(socGPIOFixture, configure_outputConfig_callsSetState)
{
   auto config = getDefaultConfig();
   config.direction = drivers::gpio::Direction::OUTPUT;
   EXPECT_CALL(*gpio_get_mock(), is_valid_gpio(m_testGPIO)).WillOnce(Return(true));
   EXPECT_CALL(*gpio_get_mock(), is_valid_output_gpio(m_testGPIO)).WillOnce(Return(true));
   EXPECT_CALL(*gpio_get_mock(), gpio_config(_)).WillOnce(Return(ESP_OK));
   EXPECT_CALL(*gpio_get_mock(), gpio_set_level(m_testGPIO, 0)).WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->configure(config));
}

TEST_F(socGPIOFixture, configure_withListener_callsEnableInterrupts)
{
   auto config = getDefaultConfig();
   config.listener = &m_listenerMock;
   EXPECT_CALL(*gpio_get_mock(), is_valid_gpio(m_testGPIO)).WillOnce(Return(true));
   EXPECT_CALL(*gpio_get_mock(), gpio_config(_)).WillOnce(Return(ESP_OK));
   EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_, _, _, _, _, _)).WillOnce(Return(pdTRUE));
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_add(m_testGPIO, _, _)).WillOnce(Return(ESP_OK));
   EXPECT_CALL(*gpio_get_mock(), gpio_set_intr_type(m_testGPIO, GPIO_INTR_POSEDGE)).WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->configure(config));
}

TEST_F(socGPIOFixture, handleInterrupt_gpioStateChange_listenerNotified)
{
   auto config = getDefaultConfig();
   config.direction = drivers::gpio::Direction::INPUT;
   config.listener = &m_listenerMock;

   drivers::gpio::socGPIO* socgpio = static_cast<drivers::gpio::socGPIO*>(m_testSubject.get());

   EXPECT_CALL(*gpio_get_mock(), is_valid_gpio(m_testGPIO)).WillOnce(Return(true));
   EXPECT_CALL(*gpio_get_mock(), gpio_config(_)).WillOnce(Return(ESP_OK));
   EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_, _, _, _, _, _)).WillOnce(Return(pdTRUE));
   EXPECT_CALL(*gpio_get_mock(), gpio_set_intr_type(m_testGPIO, GPIO_INTR_POSEDGE)).WillOnce(Return(ESP_OK));
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_add(m_testGPIO, _, _)).WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->configure(config));

   // Case 1: Listener attached, GPIO HIGH
   EXPECT_CALL(*gpio_get_mock(), gpio_get_level(m_testGPIO)).WillOnce(Return(1));
   EXPECT_CALL(m_listenerMock, onInterrupt(_, drivers::gpio::State::HIGH));
   socgpio->handleInterrupt();

   // Case 2: Listener attached, GPIO LOW
   EXPECT_CALL(*gpio_get_mock(), gpio_get_level(m_testGPIO)).WillOnce(Return(0));
   EXPECT_CALL(m_listenerMock, onInterrupt(_, drivers::gpio::State::LOW));
   socgpio->handleInterrupt();
}
