#include "mcp23017.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

//Mocks
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

using namespace drivers::gpio::mcp23017;
using namespace testing;

MATCHER_P(IsGpioNumber, expected, "")
{
   int expectedMask = (1ULL << expected);
   return arg->pin_bit_mask == static_cast<gpio_num_t>(expectedMask);
}
MATCHER_P(MatchRegister, expected, "")
{
   return arg[0] == static_cast<uint8_t>(expected);
}
MATCHER_P(MatchTwoBytes, expected, "")
{
   return arg[0] == static_cast<uint8_t>(expected[0]) && arg[1] == static_cast<uint8_t>(expected[1]);
}
MATCHER_P(MatchThreeBytes, expected, "")
{
   return arg[0] == static_cast<uint8_t>(expected[0]) &&
          arg[1] == static_cast<uint8_t>(expected[1]) &&
          arg[2] == static_cast<uint8_t>(expected[2]);
}

std::unique_ptr<MockFunction<void(PIN, bool, void*)>> g_listenerMock;
void listenerCallback(PIN pin, bool state, void* context)
{
   g_listenerMock->Call(pin, state, context);
}

struct InitializationTestFixture : public ::testing::Test {
   void SetUp() override
   {
      i2c_mock_init();
      gpio_mock_init();
      rtos_mock_init();
   }

   void TearDown() override
   {
      rtos_mock_deinit();
      gpio_mock_deinit();
      i2c_mock_deinit();
   }
   i2cConfig getValidConfig()
   {
      i2cConfig config;
      config.sclPin = GPIO_NUM_21;
      config.sdaPin = GPIO_NUM_22;
      config.resetPin = GPIO_NUM_23;
      config.clockSpeedHz = 100000;
      config.interruptPin = GPIO_NUM_24;
      return config;
   }
   i2c_master_dev_handle TEST_DEVICE_HANDLE;
   i2c_master_bus_handle TEST_BUS_HANDLE;
};

struct mcp23017ParamBase : public ::testing::Test
{
public:
   mcp23017ParamBase(std::optional<uint16_t> interruptPin)
   {
      m_i2cConfig = {};
      m_i2cConfig.interruptPin = interruptPin;
   }
   void SetUp() override
   {
      g_listenerMock = std::make_unique<MockFunction<void(PIN, bool, void*)>>();
      i2c_mock_init();
      gpio_mock_init();
      rtos_mock_init();

      m_i2cConfig.sclPin = GPIO_NUM_21;
      m_i2cConfig.sdaPin = GPIO_NUM_22;
      m_i2cConfig.resetPin = GPIO_NUM_23;
      m_i2cConfig.clockSpeedHz = 100000;

      // Only MIRROR bit is set
      std::vector<uint8_t> configurationRegister = {static_cast<uint8_t>(Register::IOCON),
                                                0b01000000};
      std::vector<uint8_t> allPullupsEnabled = {static_cast<uint8_t>(Register::GPPUA),
                                                0xFF, 0xFF};
      std::vector<uint8_t> allInterruptsEnabled = {static_cast<uint8_t>(Register::GPINTENA),
                                                0xFF, 0xFF};

      EXPECT_CALL(*i2c_get_mock(), i2c_new_master_bus(_, _))
         .WillOnce(DoAll(SetArgPointee<1>(&TEST_BUS_HANDLE), Return(ESP_OK)));
      EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_add_device(_, _, _))
         .WillOnce(DoAll(SetArgPointee<2>(&TEST_DEVICE_HANDLE), Return(ESP_OK)));
      EXPECT_CALL(*gpio_get_mock(), gpio_config(IsGpioNumber(m_i2cConfig.resetPin)));
      if (m_i2cConfig.interruptPin.has_value())
      {
         EXPECT_CALL(*gpio_get_mock(), gpio_config(IsGpioNumber(*m_i2cConfig.interruptPin)));
         EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_add(_, _, _));
         EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit(&TEST_DEVICE_HANDLE,
                  MatchThreeBytes(allInterruptsEnabled), 3, _));
         EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_,_,_,_,_,_)).WillOnce(Return(pdPASS));
      }
      EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit(&TEST_DEVICE_HANDLE,
                  MatchTwoBytes(configurationRegister), 2, _));
      EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit(&TEST_DEVICE_HANDLE,
                  MatchThreeBytes(allPullupsEnabled), 3, _));
      {
         // device can be re-set
         InSequence seq;
         EXPECT_CALL(*gpio_get_mock(), gpio_set_level(static_cast<gpio_num_t>(m_i2cConfig.resetPin),false));
         EXPECT_CALL(*gpio_get_mock(), gpio_set_level(static_cast<gpio_num_t>(m_i2cConfig.resetPin),true));
      }
      m_testSubject = std::make_unique<mcp23017>(m_busNumber, m_i2cConfig);
      m_testSubject->setListener(PIN::GPIOA0, listenerCallback, nullptr);
   }

   void TearDown() override
   {
      m_testSubject->removeListener(PIN::GPIOA0);
      if (m_i2cConfig.interruptPin.has_value())
      {
         EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_remove(_));
         EXPECT_CALL(*gpio_get_mock(), gpio_reset_pin(static_cast<gpio_num_t>(*m_i2cConfig.interruptPin)));
      }
      EXPECT_CALL(*i2c_get_mock(), i2c_del_master_bus(_));
      EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_rm_device(_));
      EXPECT_CALL(*gpio_get_mock(), gpio_reset_pin(static_cast<gpio_num_t>(m_i2cConfig.resetPin)));
      m_testSubject.reset();
      rtos_mock_deinit();
      gpio_mock_deinit();
      i2c_mock_deinit();
      g_listenerMock.reset();
   }
   MockFunction<void(bool)> m_mockListener;
   const int m_busNumber = 1;
   i2cConfig m_i2cConfig;
   std::unique_ptr<IMCP23017> m_testSubject;
   static const int XFER_TIMEOUT_MS = 20;
   i2c_master_dev_handle TEST_DEVICE_HANDLE;
   i2c_master_bus_handle TEST_BUS_HANDLE;
};

struct BitManipulationTestParams
{
   Register registerToManipulate;
   uint8_t initialRegisterValue;
   uint8_t expectedRegisterValue;
   PIN pinToManipulate;
   std::optional<Direction> directionToSet;
   std::optional<Polarity> polarityToSet;
   std::optional<Pullup> pullupToSet;
   std::optional<InterruptCompare> interruptCompareToSet;
   std::function<bool(IMCP23017*, const BitManipulationTestParams&)> methodToTest;
};
struct mcp23017ParamFixture : public ::testing::WithParamInterface<std::optional<uint16_t>>,
                             public mcp23017ParamBase
{
   mcp23017ParamFixture():
   mcp23017ParamBase::mcp23017ParamBase(GetParam())
   {
   }
};
struct mcp23017BitManipulationFixture: public ::testing::WithParamInterface<BitManipulationTestParams>,
                                            public mcp23017ParamBase
{
   mcp23017BitManipulationFixture():
   mcp23017ParamBase::mcp23017ParamBase(std::nullopt)
   {
   }
};


TEST_F(InitializationTestFixture, InvalidSCLPin_busNotCreated)
{
   i2cConfig config = getValidConfig();
   config.sclPin = GPIO_NUM_MAX; // Invalid GPIO
   config.interruptPin = std::nullopt;

   // Bus and device should not be created
   EXPECT_CALL(*i2c_get_mock(), i2c_new_master_bus(_, _)).Times(0);
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_add_device(_, _, _)).Times(0);

   std::unique_ptr<mcp23017> m_testSubject = std::make_unique<mcp23017>(1, config);

   // so should not be deleted as well
   EXPECT_CALL(*i2c_get_mock(), i2c_del_master_bus(_)).Times(0);
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_rm_device(_)).Times(0);

   m_testSubject.reset();
}

TEST_F(InitializationTestFixture, InvalidSDAPin_busNotCreated)
{
   i2cConfig config = getValidConfig();
   config.sdaPin = GPIO_NUM_MAX; // Invalid GPIO
   config.interruptPin= std::nullopt;

   // Bus and device should not be created
   EXPECT_CALL(*i2c_get_mock(), i2c_new_master_bus(_, _)).Times(0);
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_add_device(_, _, _)).Times(0);

   std::unique_ptr<mcp23017> m_testSubject = std::make_unique<mcp23017>(1, config);
   // so should not be deleted as well
   EXPECT_CALL(*i2c_get_mock(), i2c_del_master_bus(_)).Times(0);
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_rm_device(_)).Times(0);

   m_testSubject.reset();
}

TEST_F(InitializationTestFixture, InvalidResetPin_busNotCreated)
{
   i2cConfig config = getValidConfig();
   config.resetPin = GPIO_NUM_MAX; // Invalid GPIO
   config.interruptPin = std::nullopt;

   // Bus and device should not be created
   EXPECT_CALL(*i2c_get_mock(), i2c_new_master_bus(_, _)).Times(0);
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_add_device(_, _, _)).Times(0);

   std::unique_ptr<mcp23017> m_testSubject = std::make_unique<mcp23017>(1, config);

   // so should not be deleted as well
   EXPECT_CALL(*i2c_get_mock(), i2c_del_master_bus(_)).Times(0);
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_rm_device(_)).Times(0);

   m_testSubject.reset();
}

TEST_F(InitializationTestFixture, InvalidSCLSpeed_busNotCreated)
{
   i2cConfig config = getValidConfig();
   config.clockSpeedHz = 0; // Invalid speed
   config.interruptPin = std::nullopt;

   // Bus and device should not be created
   EXPECT_CALL(*i2c_get_mock(), i2c_new_master_bus(_, _)).Times(0);
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_add_device(_, _, _)).Times(0);

   std::unique_ptr<mcp23017> m_testSubject = std::make_unique<mcp23017>(1, config);

   // so should not be deleted as well
   EXPECT_CALL(*i2c_get_mock(), i2c_del_master_bus(_)).Times(0);
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_rm_device(_)).Times(0);

   m_testSubject.reset();
}

TEST_F(InitializationTestFixture, busHandleEmpty_deviceNotCreated)
{
   // Bus and device should not be created
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_add_device(_, _, _)).Times(0);

   std::unique_ptr<mcp23017> m_testSubject = std::make_unique<mcp23017>(nullptr, getValidConfig());

   // so should not be deleted as well
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_rm_device(_)).Times(0);

   m_testSubject.reset();
}

TEST_F(InitializationTestFixture, externalBusInvalidConfig_deviceNotCreated)
{
   i2cConfig config = getValidConfig();
   config.clockSpeedHz = 0; // Invalid speed
   config.interruptPin = std::nullopt;

   // Bus and device should not be created
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_add_device(_, _, _)).Times(0);

   std::shared_ptr<i2c_master_bus_handle_t> busHandle = std::make_shared<i2c_master_bus_handle_t>();
   std::unique_ptr<mcp23017> m_testSubject = std::make_unique<mcp23017>(busHandle.get(), config);

   // so should not be deleted as well
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_rm_device(_)).Times(0);

   m_testSubject.reset();
}
TEST_F(InitializationTestFixture, CannotCreateInterruptTask_interruptNotAttached)
{
   i2cConfig config = getValidConfig();

   // Bus and device should be created
   EXPECT_CALL(*i2c_get_mock(), i2c_new_master_bus(_, _))
      .WillOnce(DoAll(SetArgPointee<1>(&TEST_BUS_HANDLE), Return(ESP_OK)));
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_add_device(_, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(&TEST_DEVICE_HANDLE), Return(ESP_OK)));
   // device can be re-set
   EXPECT_CALL(*gpio_get_mock(), gpio_set_level(static_cast<gpio_num_t>(config.resetPin),_)).Times(2);
   // and configuration can be sent
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit(_, _, _, _)).Times(AtLeast(1));
   // reset pin should be opened
   EXPECT_CALL(*gpio_get_mock(), gpio_config(IsGpioNumber(config.resetPin)));
   EXPECT_CALL(*gpio_get_mock(), gpio_reset_pin(static_cast<gpio_num_t>(config.resetPin)));
   // task cant't be created
   EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_,_,_,_,_,_)).WillOnce(Return(pdFAIL));
   // so gpio interrupt should not be attached
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_add(_, _, _)).Times(0);
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_remove(_)).Times(0);
   EXPECT_CALL(*gpio_get_mock(), gpio_reset_pin(static_cast<gpio_num_t>(*config.interruptPin))).Times(0);

   std::unique_ptr<mcp23017> m_testSubject = std::make_unique<mcp23017>(1, config);

   // i2c stuff should be deleted as well
   EXPECT_CALL(*i2c_get_mock(), i2c_del_master_bus(_));
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_rm_device(_));

   m_testSubject.reset();
}
TEST_F(InitializationTestFixture, InvalidInterruptGPIO_interruptNotAttached)
{
   i2cConfig config = getValidConfig();
   config.interruptPin = GPIO_NUM_MAX; // Invalid GPIO

   // Bus and device should be created
   EXPECT_CALL(*i2c_get_mock(), i2c_new_master_bus(_, _))
      .WillOnce(DoAll(SetArgPointee<1>(&TEST_BUS_HANDLE), Return(ESP_OK)));
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_add_device(_, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(&TEST_DEVICE_HANDLE), Return(ESP_OK)));
   // device can be re-set
   EXPECT_CALL(*gpio_get_mock(), gpio_set_level(static_cast<gpio_num_t>(config.resetPin),_)).Times(2);
   // and configuration can be sent
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit(_, _, _, _)).Times(AtLeast(1));
   // reset pin should be opened
   EXPECT_CALL(*gpio_get_mock(), gpio_config(IsGpioNumber(config.resetPin)));
   EXPECT_CALL(*gpio_get_mock(), gpio_reset_pin(static_cast<gpio_num_t>(config.resetPin)));
   // but gpio interrupt should not be attached
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_add(_, _, _)).Times(0);
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_remove(_)).Times(0);
   EXPECT_CALL(*gpio_get_mock(), gpio_reset_pin(static_cast<gpio_num_t>(*config.interruptPin))).Times(0);
   // and task should not be created
   EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_,_,_,_,_,_)).Times(0);

   std::unique_ptr<mcp23017> m_testSubject = std::make_unique<mcp23017>(1, config);

   // i2c stuff should be deleted as well
   EXPECT_CALL(*i2c_get_mock(), i2c_del_master_bus(_));
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_rm_device(_));

   m_testSubject.reset();
}

TEST_F(InitializationTestFixture, CorrectInterruptGPIO_interruptAttached)
{
   i2cConfig config = getValidConfig();

   // Bus and device should be created
   EXPECT_CALL(*i2c_get_mock(), i2c_new_master_bus(_, _))
      .WillOnce(DoAll(SetArgPointee<1>(&TEST_BUS_HANDLE), Return(ESP_OK)));
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_add_device(_, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(&TEST_DEVICE_HANDLE), Return(ESP_OK)));
   // device can be re-set
   EXPECT_CALL(*gpio_get_mock(), gpio_set_level(static_cast<gpio_num_t>(config.resetPin),_)).Times(2);
   // and configuration can be sent
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit(_, _, _, _)).Times(AtLeast(1));
   // reset pin should be opened
   EXPECT_CALL(*gpio_get_mock(), gpio_config(IsGpioNumber(config.resetPin)));
   EXPECT_CALL(*gpio_get_mock(), gpio_reset_pin(static_cast<gpio_num_t>(config.resetPin)));
   // reset and interrupt pins opened
   EXPECT_CALL(*gpio_get_mock(), gpio_config(IsGpioNumber(*config.interruptPin)));
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_add(_, _, _));
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_remove(_));
   EXPECT_CALL(*gpio_get_mock(), gpio_reset_pin(static_cast<gpio_num_t>(*config.interruptPin)));
   // task can be created
   EXPECT_CALL(*rtos_get_mock(), xTaskCreate(_,_,_,_,_,_)).WillOnce(Return(pdPASS));
   std::unique_ptr<mcp23017> m_testSubject = std::make_unique<mcp23017>(1, config);

   // so should be deleted as well
   EXPECT_CALL(*i2c_get_mock(), i2c_del_master_bus(_));
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_rm_device(_));

   m_testSubject.reset();
}

TEST_F(InitializationTestFixture, CannotOpenResetPin_DeviceNotCreated)
{
   i2cConfig config = getValidConfig();

   EXPECT_CALL(*i2c_get_mock(), i2c_new_master_bus(_, _))
      .WillOnce(DoAll(SetArgPointee<1>(&TEST_BUS_HANDLE), Return(ESP_OK)));
   EXPECT_CALL(*gpio_get_mock(), gpio_config(IsGpioNumber(config.resetPin))).WillOnce(Return(ESP_FAIL));
   EXPECT_CALL(*gpio_get_mock(), gpio_config(IsGpioNumber(*config.interruptPin))).Times(0);
   EXPECT_CALL(*gpio_get_mock(), gpio_isr_handler_add(_, _, _)).Times(0);
   EXPECT_CALL(*i2c_get_mock(), i2c_master_bus_add_device(_, _, _)).Times(0);

   std::unique_ptr<mcp23017> m_testSubject = std::make_unique<mcp23017>(1, config);

   // so should be deleted as well
   EXPECT_CALL(*i2c_get_mock(), i2c_del_master_bus(_));

   m_testSubject.reset();
}

TEST_P(mcp23017ParamFixture, probeTests)
{
   EXPECT_CALL(*i2c_get_mock(), i2c_master_probe(_, _, _))
       .WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->probe());

   EXPECT_CALL(*i2c_get_mock(), i2c_master_probe(_, _, _))
       .WillOnce(Return(ESP_FAIL));
   EXPECT_FALSE(m_testSubject->probe());
}

TEST_P(mcp23017ParamFixture, interruptProcessing_cannotReadRegister_noListenerCalled)
{
   if (!m_i2cConfig.interruptPin.has_value())
   {
      GTEST_SKIP() << "Interrupt config not provided, skipping test.";
   }
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit_receive(&TEST_DEVICE_HANDLE,
               MatchRegister(Register::GPIOA), 1, _, 2, _))
       .WillOnce(Return(ESP_FAIL));
   EXPECT_CALL(*g_listenerMock, Call(_,_,_)).Times(0);
   mcp23017* testSubjectPtr = dynamic_cast<mcp23017*>(m_testSubject.get());
   testSubjectPtr->processInterrupt();
}

TEST_P(mcp23017ParamFixture, interruptProcessing_inputStateChanged_listenerCalled)
{
   if (!m_i2cConfig.interruptPin.has_value())
   {
      GTEST_SKIP() << "Interrupt config not provided, skipping test.";
   }
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit_receive(&TEST_DEVICE_HANDLE,
               MatchRegister(Register::GPIOA), 1, _, 2, _))
       .WillOnce(DoAll(SetArgPointee<3>(0x0001), Return(ESP_OK)));
   EXPECT_CALL(*g_listenerMock, Call(PIN::GPIOA0, true, _));
   mcp23017* testSubjectPtr = dynamic_cast<mcp23017*>(m_testSubject.get());
   testSubjectPtr->processInterrupt();
}

TEST_P(mcp23017ParamFixture, writingPinState_cannotReadRegister_requestNotSent)
{
   if (m_i2cConfig.interruptPin.has_value())
   {
      GTEST_SKIP() << "Interrupt config provided, skipping test because there is no register read when interrupts are enabled.";
   }
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit_receive(&TEST_DEVICE_HANDLE,
               MatchRegister(Register::GPIOA), 1, _, 1, _))
       .WillOnce(Return(ESP_FAIL));
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit(&TEST_DEVICE_HANDLE,
               _, _, _)).Times(0);
   EXPECT_FALSE(m_testSubject->writePin(PIN::GPIOA0, true));
}

TEST_P(mcp23017ParamFixture, writingPinState_cannotWriteNewValue_falseReturned)
{
   if (!m_i2cConfig.interruptPin.has_value())
   {
      EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit_receive(&TEST_DEVICE_HANDLE,
                  MatchRegister(Register::GPIOA), 1, _, 1, _))
          .WillOnce(DoAll(SetArgPointee<3>(0b00000000), Return(ESP_OK)));
   }
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit(&TEST_DEVICE_HANDLE,
               MatchTwoBytes(std::vector<uint8_t>{static_cast<uint8_t>(Register::GPIOA), 0b00000001}), 2, _))
       .WillOnce(Return(ESP_FAIL));
   EXPECT_FALSE(m_testSubject->writePin(PIN::GPIOA0, true));
}

TEST_P(mcp23017ParamFixture, writingPinState_writeSuccess_trueReturned)
{
   if (!m_i2cConfig.interruptPin.has_value())
   {
      EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit_receive(&TEST_DEVICE_HANDLE,
                  MatchRegister(Register::GPIOA), 1, _, 1, _))
          .WillOnce(DoAll(SetArgPointee<3>(0b00000000), Return(ESP_OK)));
   }
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit(&TEST_DEVICE_HANDLE,
               MatchTwoBytes(std::vector<uint8_t>{static_cast<uint8_t>(Register::GPIOA), 0b00000001}), 2, _))
       .WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->writePin(PIN::GPIOA0, true));
}

TEST_P(mcp23017ParamFixture, writingPinState_twoPinsWritesSuccess_correctValuesWritten)
{
   if (!m_i2cConfig.interruptPin.has_value())
   {
      EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit_receive(&TEST_DEVICE_HANDLE,
                  MatchRegister(Register::GPIOA), 1, _, 1, _))
          .WillOnce(DoAll(SetArgPointee<3>(0b00000000), Return(ESP_OK)))
          .WillOnce(DoAll(SetArgPointee<3>(0b00000001), Return(ESP_OK)));
   }
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit(&TEST_DEVICE_HANDLE,
               MatchTwoBytes(std::vector<uint8_t>{static_cast<uint8_t>(Register::GPIOA), 0b00000001}), 2, _))
       .WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->writePin(PIN::GPIOA0, true));

   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit(&TEST_DEVICE_HANDLE,
               MatchTwoBytes(std::vector<uint8_t>{static_cast<uint8_t>(Register::GPIOA), 0b00010001}), 2, _))
       .WillOnce(Return(ESP_OK));
   EXPECT_TRUE(m_testSubject->writePin(PIN::GPIOA4, true));
}

TEST_P(mcp23017ParamFixture, readingPinState_interruptActive_cachedValueReturned)
{
   if (!m_i2cConfig.interruptPin.has_value())
   {
      GTEST_SKIP() << "Interrupt config not provided, skipping test because there is no cached value when interrupts are disabled.";
   }
   // Simulate an interrupt that sets GPIOA0 high
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit_receive(&TEST_DEVICE_HANDLE,
               MatchRegister(Register::GPIOA), 1, _, 2, _))
       .WillOnce(DoAll(SetArgPointee<3>(0b00000001), Return(ESP_OK)));
   mcp23017* testSubjectPtr = dynamic_cast<mcp23017*>(m_testSubject.get());
   testSubjectPtr->processInterrupt();
   // Now read the pin state, should return cached value without I2C read
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit_receive(&TEST_DEVICE_HANDLE,
               MatchRegister(Register::GPIOA), 1, _, 2, _)).Times(0);
   auto pinState = m_testSubject->readPin(PIN::GPIOA0);
   ASSERT_TRUE(pinState.has_value());
   EXPECT_TRUE(pinState.value());
}
TEST_P(mcp23017ParamFixture, readingPinState_interruptInactive_valueReadFromDevice)
{
   if (m_i2cConfig.interruptPin.has_value())
   {
      GTEST_SKIP() << "Interrupt config provided, skipping test because there is no I2C read when interrupts are enabled.";
   }
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit_receive(&TEST_DEVICE_HANDLE,
               MatchRegister(Register::GPIOA), 1, _, 1, _))
       .WillOnce(DoAll(SetArgPointee<3>(0b00000001), Return(ESP_OK)));
   auto pinState = m_testSubject->readPin(PIN::GPIOA0);
   ASSERT_TRUE(pinState.has_value());
   EXPECT_TRUE(pinState.value());
}

TEST_P(mcp23017BitManipulationFixture, BitManipulationTests)
{
   BitManipulationTestParams params = GetParam();

   const std::vector<uint8_t> expectedWrite = {
       static_cast<uint8_t>(params.registerToManipulate),
       params.expectedRegisterValue,
   };
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit_receive(&TEST_DEVICE_HANDLE,
               MatchRegister(params.registerToManipulate), 1, _, 1, _))
       .WillOnce(DoAll(SetArgPointee<3>(params.initialRegisterValue), Return(ESP_OK)));
   EXPECT_CALL(*i2c_get_mock(), i2c_master_transmit(&TEST_DEVICE_HANDLE,
               MatchTwoBytes(expectedWrite), 2, _))
       .WillOnce(Return(ESP_OK));
   EXPECT_TRUE(params.methodToTest(m_testSubject.get(), params));
}

auto setDirectionMethod = [](IMCP23017* instance, const BitManipulationTestParams& params)
{
   return instance->setDirection(params.pinToManipulate, *(params.directionToSet));
};
auto setPolarityMethod = [](IMCP23017* instance, const BitManipulationTestParams& params)
{
   return instance->setPolarity(params.pinToManipulate, *(params.polarityToSet));
};
auto setPullupMethod = [](IMCP23017* instance, const BitManipulationTestParams& params)
{
   return instance->setPullup(params.pinToManipulate, *(params.pullupToSet));
};
auto setInterruptCompareMethod = [](IMCP23017* instance, const BitManipulationTestParams& params)
{
   return instance->setInterruptCompare(params.pinToManipulate, *(params.interruptCompareToSet));
};

INSTANTIATE_TEST_SUITE_P(
   BitManipulationTests,
   mcp23017BitManipulationFixture,
   ::testing::Values(
BitManipulationTestParams{Register::IODIRA, 0xFF, 0xF7, PIN::GPIOA3, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRA, 0xFF, 0xF7, PIN::GPIOA3, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRA, 0xF7, 0xFF, PIN::GPIOA3, Direction::INPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRA, 0xF7, 0xF7, PIN::GPIOA3, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRA, 0xFF, 0xFF, PIN::GPIOA3, Direction::INPUT,  {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRA, 0xFF, 0xFD, PIN::GPIOA1, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRA, 0xFF, 0xFB, PIN::GPIOA2, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRA, 0xFF, 0xF7, PIN::GPIOA3, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRA, 0xFF, 0xEF, PIN::GPIOA4, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRA, 0xFF, 0xDF, PIN::GPIOA5, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRA, 0xFF, 0xBF, PIN::GPIOA6, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRA, 0xFF, 0x7F, PIN::GPIOA7, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRB, 0xFF, 0xFE, PIN::GPIOB0, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRB, 0xFF, 0xFD, PIN::GPIOB1, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRB, 0xFF, 0xFB, PIN::GPIOB2, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRB, 0xFF, 0xF7, PIN::GPIOB3, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRB, 0xFF, 0xEF, PIN::GPIOB4, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRB, 0xFF, 0xDF, PIN::GPIOB5, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRB, 0xFF, 0xBF, PIN::GPIOB6, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IODIRB, 0xFF, 0x7F, PIN::GPIOB7, Direction::OUTPUT, {}, {}, {}, setDirectionMethod},
BitManipulationTestParams{Register::IPOLA, 0x00, 0x08, PIN::GPIOA3, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLA, 0x08, 0x00, PIN::GPIOA3, {}, Polarity::NORMAL, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLB, 0x02, 0x02, PIN::GPIOB1, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLB, 0x00, 0x00, PIN::GPIOB1, {}, Polarity::NORMAL, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLA, 0x00, 0x01, PIN::GPIOA0, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLA, 0x00, 0x02, PIN::GPIOA1, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLA, 0x00, 0x04, PIN::GPIOA2, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLA, 0x00, 0x08, PIN::GPIOA3, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLA, 0x00, 0x10, PIN::GPIOA4, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLA, 0x00, 0x20, PIN::GPIOA5, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLA, 0x00, 0x40, PIN::GPIOA6, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLA, 0x00, 0x80, PIN::GPIOA7, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLB, 0x00, 0x01, PIN::GPIOB0, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLB, 0x00, 0x02, PIN::GPIOB1, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLB, 0x00, 0x04, PIN::GPIOB2, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLB, 0x00, 0x08, PIN::GPIOB3, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLB, 0x00, 0x10, PIN::GPIOB4, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLB, 0x00, 0x20, PIN::GPIOB5, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLB, 0x00, 0x40, PIN::GPIOB6, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::IPOLB, 0x00, 0x80, PIN::GPIOB7, {}, Polarity::INVERTED, {}, {}, setPolarityMethod},
BitManipulationTestParams{Register::GPPUA, 0x00, 0x08, PIN::GPIOA3, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUA, 0x08, 0x00, PIN::GPIOA3, {}, {}, Pullup::DISABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUB, 0x02, 0x02, PIN::GPIOB1, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUB, 0x00, 0x00, PIN::GPIOB1, {}, {}, Pullup::DISABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUA, 0x00, 0x01, PIN::GPIOA0, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUA, 0x00, 0x02, PIN::GPIOA1, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUA, 0x00, 0x04, PIN::GPIOA2, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUA, 0x00, 0x08, PIN::GPIOA3, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUA, 0x00, 0x10, PIN::GPIOA4, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUA, 0x00, 0x20, PIN::GPIOA5, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUA, 0x00, 0x40, PIN::GPIOA6, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUA, 0x00, 0x80, PIN::GPIOA7, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUB, 0x00, 0x01, PIN::GPIOB0, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUB, 0x00, 0x02, PIN::GPIOB1, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUB, 0x00, 0x04, PIN::GPIOB2, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUB, 0x00, 0x08, PIN::GPIOB3, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUB, 0x00, 0x10, PIN::GPIOB4, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUB, 0x00, 0x20, PIN::GPIOB5, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUB, 0x00, 0x40, PIN::GPIOB6, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::GPPUB, 0x00, 0x80, PIN::GPIOB7, {}, {}, Pullup::ENABLED, {}, setPullupMethod},
BitManipulationTestParams{Register::INTCONA, 0x00, 0x08, PIN::GPIOA3, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONA, 0x08, 0x00, PIN::GPIOA3, {}, {}, {}, InterruptCompare::COMPARE_PREVIOUS, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONB, 0x02, 0x02, PIN::GPIOB1, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONB, 0x00, 0x00, PIN::GPIOB1, {}, {}, {}, InterruptCompare::COMPARE_PREVIOUS, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONA, 0x00, 0x01, PIN::GPIOA0, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONA, 0x00, 0x02, PIN::GPIOA1, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONA, 0x00, 0x04, PIN::GPIOA2, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONA, 0x00, 0x08, PIN::GPIOA3, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONA, 0x00, 0x10, PIN::GPIOA4, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONA, 0x00, 0x20, PIN::GPIOA5, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONA, 0x00, 0x40, PIN::GPIOA6, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONA, 0x00, 0x80, PIN::GPIOA7, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONB, 0x00, 0x01, PIN::GPIOB0, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONB, 0x00, 0x02, PIN::GPIOB1, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONB, 0x00, 0x04, PIN::GPIOB2, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONB, 0x00, 0x08, PIN::GPIOB3, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONB, 0x00, 0x10, PIN::GPIOB4, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONB, 0x00, 0x20, PIN::GPIOB5, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONB, 0x00, 0x40, PIN::GPIOB6, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod},
BitManipulationTestParams{Register::INTCONB, 0x00, 0x80, PIN::GPIOB7, {}, {}, {}, InterruptCompare::COMPARE_DEFAULT, setInterruptCompareMethod}
)
);

INSTANTIATE_TEST_SUITE_P(
   InterruptConfigTests,
   mcp23017ParamFixture,
   ::testing::Values(
       GPIO_NUM_24,
       std::nullopt
   )
);
