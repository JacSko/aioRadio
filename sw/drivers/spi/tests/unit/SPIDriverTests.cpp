#include "SPIDriver.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "IGPIOMock.h"
#include "GPIOProviderMock.h"
//Mocks
#include "driver/spi_master.h"

using namespace testing;
using namespace drivers::spi;

MATCHER_P(SPI_BUS_CFG, expected, "")
{
   return arg->mosi_io_num == expected.mosiPin &&
          arg->miso_io_num == expected.misoPin &&
          arg->sclk_io_num == expected.sckPin;
}
MATCHER_P(SPI_DEV_CFG, expected, "")
{
   return arg->mode == expected.mode &&
          arg->clock_speed_hz == expected.clockSpeedHz &&
          arg->flags == SPI_DEVICE_HALFDUPLEX &&
          arg->spics_io_num == expected.csPin;
}

struct SPIDriverFixture : public ::testing::Test
{
public:
   SPIDriverFixture():
   m_gpioProviderMock({})
   {}
   virtual void SetUp() override
   {
      spiMasterMockInit();
      m_interruptPinMock = new drivers::gpio::IGPIOMock();
      m_resetPinMock = new drivers::gpio::IGPIOMock();
      EXPECT_CALL(*drivers::gpio::getGPIOProviderMock(), create(drivers::gpio::GPIO::TUNER_RST))
         .WillOnce(Return(ByMove(std::unique_ptr<drivers::gpio::IGPIO>(m_resetPinMock))));
      EXPECT_CALL(*drivers::gpio::getGPIOProviderMock(), create(drivers::gpio::GPIO::TUNER_INT))
         .WillOnce(Return(ByMove(std::unique_ptr<drivers::gpio::IGPIO>(m_interruptPinMock))));
      SPIDriverConfig config = getDefaultConfig();
      EXPECT_CALL(*getSPIMasterMock(), spi_bus_initialize(config.host,SPI_BUS_CFG(config),SPI_DMA_CH_AUTO)).WillOnce(Return(ESP_OK));
      EXPECT_CALL(*getSPIMasterMock(), spi_bus_add_device(config.host,SPI_DEV_CFG(config),_)).WillOnce(DoAll(SetArgPointee<2>(&m_testDevice), Return(ESP_OK)));

      drivers::gpio::Config interruptConfig;
      drivers::gpio::Config resetConfig;
      EXPECT_CALL(*m_interruptPinMock, configure(_)).WillOnce(DoAll(SaveArg<0>(&interruptConfig),Return(true)));
      EXPECT_CALL(*m_resetPinMock, configure(_)).WillOnce(DoAll(SaveArg<0>(&resetConfig),Return(true)));
      m_testSubject = std::make_unique<drivers::spi::SPIDriver>(config, m_gpioProviderMock);

      EXPECT_EQ(interruptConfig.direction, drivers::gpio::Direction::INPUT);
      EXPECT_EQ(interruptConfig.pullMode, drivers::gpio::PullMode::NONE);
      EXPECT_EQ(interruptConfig.interruptEdge, drivers::gpio::InterruptEdge::FALLING);
      m_interruptListener = interruptConfig.listener;
      EXPECT_EQ(resetConfig.direction, drivers::gpio::Direction::OUTPUT);
      EXPECT_EQ(resetConfig.state, drivers::gpio::State::HIGH);
   }

   virtual void TearDown() override
   {
      EXPECT_CALL(*getSPIMasterMock(), spi_bus_remove_device(&m_testDevice)).WillOnce(Return(ESP_OK));
      m_testSubject.reset();
      spiMasterMockDeinit();
   }

   static SPIDriverConfig getDefaultConfig()
   {
      SPIDriverConfig config = {};
      config.host = SPI1_HOST;
      config.clockSpeedHz = 1000000;
      config.mode = 0;
      config.mosiPin = 1;
      config.misoPin = 2;
      config.sckPin = 3;
      config.csPin = 4;
      config.interruptPin = drivers::gpio::GPIO::TUNER_INT;
      config.resetPin = drivers::gpio::GPIO::TUNER_RST;
      return config;
   }

   drivers::gpio::IGPIOMock* m_interruptPinMock;
   drivers::gpio::IGPIOMock* m_resetPinMock;
   drivers::gpio::GPIOProvider m_gpioProviderMock;
   spi_device_t m_testDevice;
   drivers::gpio::GPIOListener* m_interruptListener;
   std::unique_ptr<drivers::spi::SPIDriver> m_testSubject;
};

struct SPIDriverInitialization : public SPIDriverFixture
{
   virtual void SetUp() override
   {
      spiMasterMockInit();
      m_interruptPinMock = new drivers::gpio::IGPIOMock();
      m_resetPinMock = new drivers::gpio::IGPIOMock();
   }
   virtual void TearDown() override
   {
      spiMasterMockDeinit();
   }
};


TEST_F(SPIDriverInitialization, cannotCreateInterruptPin_spiBusNotInitialized)
{
   EXPECT_CALL(*drivers::gpio::getGPIOProviderMock(), create(drivers::gpio::GPIO::TUNER_RST))
      .WillOnce(Return(ByMove(std::unique_ptr<drivers::gpio::IGPIO>(m_resetPinMock))));
   EXPECT_CALL(*drivers::gpio::getGPIOProviderMock(), create(drivers::gpio::GPIO::TUNER_INT))
      .WillOnce(Return(nullptr));
   SPIDriverConfig config = SPIDriverFixture::getDefaultConfig();
   EXPECT_CALL(*getSPIMasterMock(), spi_bus_initialize(_,_,_)).Times(0);
   EXPECT_CALL(*getSPIMasterMock(), spi_bus_add_device(_,_,_)).Times(0);
   m_testSubject = std::make_unique<drivers::spi::SPIDriver>(config, m_gpioProviderMock);
   m_testSubject.reset();
}

TEST_F(SPIDriverInitialization, cannotCreateResetPin_spiBusNotInitialized)
{
   EXPECT_CALL(*drivers::gpio::getGPIOProviderMock(), create(drivers::gpio::GPIO::TUNER_RST))
      .WillOnce(Return(nullptr));
   EXPECT_CALL(*drivers::gpio::getGPIOProviderMock(), create(drivers::gpio::GPIO::TUNER_INT))
      .WillOnce(Return(ByMove(std::unique_ptr<drivers::gpio::IGPIO>(m_interruptPinMock))));
   SPIDriverConfig config = SPIDriverFixture::getDefaultConfig();
   EXPECT_CALL(*getSPIMasterMock(), spi_bus_initialize(_,_,_)).Times(0);
   EXPECT_CALL(*getSPIMasterMock(), spi_bus_add_device(_,_,_)).Times(0);
   m_testSubject = std::make_unique<drivers::spi::SPIDriver>(config, m_gpioProviderMock);
   m_testSubject.reset();
}

TEST_F(SPIDriverInitialization, cannotInitializeSpiBus_deviceNotAdded)
{
   EXPECT_CALL(*drivers::gpio::getGPIOProviderMock(), create(drivers::gpio::GPIO::TUNER_RST))
      .WillOnce(Return(ByMove(std::unique_ptr<drivers::gpio::IGPIO>(m_resetPinMock))));
   EXPECT_CALL(*drivers::gpio::getGPIOProviderMock(), create(drivers::gpio::GPIO::TUNER_INT))
      .WillOnce(Return(ByMove(std::unique_ptr<drivers::gpio::IGPIO>(m_interruptPinMock))));
   SPIDriverConfig config = SPIDriverFixture::getDefaultConfig();
   EXPECT_CALL(*getSPIMasterMock(), spi_bus_initialize(_,_,_)).WillOnce(Return(ESP_FAIL));
   EXPECT_CALL(*getSPIMasterMock(), spi_bus_add_device(_,_,_)).Times(0);
   m_testSubject = std::make_unique<drivers::spi::SPIDriver>(config, m_gpioProviderMock);
   m_testSubject.reset();
}

TEST_F(SPIDriverInitialization, cannotAddSpiDevice_gpiosNotConfigured)
{
   EXPECT_CALL(*drivers::gpio::getGPIOProviderMock(), create(drivers::gpio::GPIO::TUNER_RST))
      .WillOnce(Return(ByMove(std::unique_ptr<drivers::gpio::IGPIO>(m_resetPinMock))));
   EXPECT_CALL(*drivers::gpio::getGPIOProviderMock(), create(drivers::gpio::GPIO::TUNER_INT))
      .WillOnce(Return(ByMove(std::unique_ptr<drivers::gpio::IGPIO>(m_interruptPinMock))));
   SPIDriverConfig config = SPIDriverFixture::getDefaultConfig();
   EXPECT_CALL(*getSPIMasterMock(), spi_bus_initialize(_,_,_)).WillOnce(Return(ESP_OK));
   EXPECT_CALL(*getSPIMasterMock(), spi_bus_add_device(_,_,_)).WillOnce(Return(ESP_FAIL));
   EXPECT_CALL(*m_interruptPinMock, configure(_)).Times(0);
   EXPECT_CALL(*m_resetPinMock, configure(_)).Times(0);
   m_testSubject = std::make_unique<drivers::spi::SPIDriver>(config, m_gpioProviderMock);
   m_testSubject.reset();
}

TEST_F(SPIDriverInitialization, cannotConfigureInterruptPin_resetPinNotConfigured)
{
   EXPECT_CALL(*drivers::gpio::getGPIOProviderMock(), create(drivers::gpio::GPIO::TUNER_RST))
      .WillOnce(Return(ByMove(std::unique_ptr<drivers::gpio::IGPIO>(m_resetPinMock))));
   EXPECT_CALL(*drivers::gpio::getGPIOProviderMock(), create(drivers::gpio::GPIO::TUNER_INT))
      .WillOnce(Return(ByMove(std::unique_ptr<drivers::gpio::IGPIO>(m_interruptPinMock))));
   SPIDriverConfig config = SPIDriverFixture::getDefaultConfig();
   EXPECT_CALL(*getSPIMasterMock(), spi_bus_initialize(_,_,_)).WillOnce(Return(ESP_OK));
   EXPECT_CALL(*getSPIMasterMock(), spi_bus_add_device(_,_,_)).WillOnce(Return(ESP_OK));
   EXPECT_CALL(*m_interruptPinMock, configure(_)).WillOnce(Return(false));
   EXPECT_CALL(*m_resetPinMock, configure(_)).Times(0);
   m_testSubject = std::make_unique<drivers::spi::SPIDriver>(config, m_gpioProviderMock);
   m_testSubject.reset();
}

TEST_F(SPIDriverFixture, write_emptyBufferProvided_returnsFalse)
{
   bool result = m_testSubject->write(nullptr, 5);
   EXPECT_FALSE(result);
}

TEST_F(SPIDriverFixture, read_emptyBufferProvided_returnsFalse)
{
   bool result = m_testSubject->read(nullptr, 5);
   EXPECT_FALSE(result);
}

TEST_F(SPIDriverFixture, write_lengthIsZero_returnsFalse)
{
   uint8_t data[] = {};
   bool result = m_testSubject->write(data, 0);
   EXPECT_FALSE(result);
}

TEST_F(SPIDriverFixture, read_lengthIsZero_returnsFalse)
{
   uint8_t data[] = {};
   bool result = m_testSubject->read(data, 0);
   EXPECT_FALSE(result);
}

