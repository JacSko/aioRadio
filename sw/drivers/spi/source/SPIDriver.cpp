#include "SPIDriver.h"
#include "Log.h"

namespace drivers::spi
{

static const char* TAG = "SPIDriver";
static const uint16_t SPI_BUFFER_SIZE = 4096;

SPIDriver::SPIDriver(const SPIDriverConfig& config, ::drivers::gpio::GPIOProvider& gpioProvider):
m_config(config),
m_gpioProvider(gpioProvider),
m_interruptPin(m_gpioProvider.create(m_config.interruptPin)),
m_resetPin(m_gpioProvider.create(m_config.resetPin)),
m_interruptMutex(nullptr),
m_spiHandle(nullptr)
{
   AIO_LOGE_IF(!m_interruptPin || !m_resetPin, TAG, "Failed to create GPIO pins for SPI driver");
   if (m_interruptPin && m_resetPin && !initialize())
   {
      AIO_LOGE(TAG, "Failed to initialize SPIDriver");
   }
}

SPIDriver::~SPIDriver()
{
   finalize();
}

bool SPIDriver::initialize()
{
   spi_bus_config_t buscfg = {
      .mosi_io_num = m_config.mosiPin,
      .miso_io_num = m_config.misoPin,
      .sclk_io_num = m_config.sckPin,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = SPI_BUFFER_SIZE,
   };
   if (spi_bus_initialize(m_config.host, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK)
   {
       AIO_LOGE(TAG, "Failed to initialize SPI bus");
       return false;
   }

   spi_device_interface_config_t devcfg = {
       .mode = m_config.mode,
       .clock_speed_hz = m_config.clockSpeedHz,
       .spics_io_num = m_config.csPin,
       .flags = SPI_DEVICE_HALFDUPLEX,
       .queue_size = 1,
   };
   if (spi_bus_add_device(m_config.host, &devcfg, &m_spiHandle) != ESP_OK)
   {
      AIO_LOGE(TAG, "Cannot add SPI device to bus %d", static_cast<int>(m_config.host));
      return false;
   }

   drivers::gpio::Config interruptConfig = {};
   interruptConfig.direction = gpio::Direction::INPUT;
   interruptConfig.pullMode = gpio::PullMode::NONE;
   interruptConfig.interruptEdge = gpio::InterruptEdge::FALLING;
   interruptConfig.listener = this;
   if (!m_interruptPin->configure(interruptConfig))
   {
      AIO_LOGE(TAG, "Cannot configure interrupt pin!");
      return false;
   }

   drivers::gpio::Config resetConfig = {};
   resetConfig.direction = gpio::Direction::OUTPUT;
   resetConfig.state = gpio::State::HIGH;
   if (!m_resetPin->configure(resetConfig))
   {
      AIO_LOGE(TAG, "Cannot configure reset pin!");
      return false;
   }

   m_interruptMutex = xSemaphoreCreateBinary();
   if (m_interruptMutex == nullptr)
   {
      AIO_LOGE(TAG, "Failed to create interrupt mutex");
      return false;
   }

   AIO_LOGI(TAG, "SPIDriver initialized successfully");
   return true;
}

void SPIDriver::finalize()
{
   if (m_spiHandle)
   {
      spi_bus_remove_device(m_spiHandle);
      m_spiHandle = nullptr;
   }
   if (m_interruptMutex != nullptr)
   {
      vSemaphoreDelete(m_interruptMutex);
      m_interruptMutex = nullptr;
   }
}

void SPIDriver::onInterrupt(::drivers::gpio::GPIO id, ::drivers::gpio::State state)
{
    if (id == m_config.interruptPin  && state == ::drivers::gpio::State::LOW)
    {
       int result = xSemaphoreGive(m_interruptMutex);
       AIO_LOGE_IF(result != pdTRUE, TAG, "Got interrupt, but no one is waiting for it!");
    }
}

bool SPIDriver::write(const uint8_t* data, size_t length)
{
   if (!m_spiHandle || !data || length == 0)
   {
      AIO_LOGE(TAG, "Invalid parameters for SPI write: handle=%p, data=%p, length=%zu", m_spiHandle, data, length);
      return false;
   }

   spi_transaction_t transaction = {};
   transaction.length = length * 8; // length in bits
   transaction.tx_buffer = data;
   transaction.rx_buffer = nullptr;
   return spi_device_transmit(m_spiHandle, &transaction) == ESP_OK;
}

bool SPIDriver::read(uint8_t* data, size_t length)
{
   if (!m_spiHandle || !data || length == 0)
   {
      AIO_LOGE(TAG, "Invalid parameters for SPI read: handle=%p, data=%p, length=%zu", m_spiHandle, data, length);
      return false;
   }

   spi_transaction_t transaction = {};
   transaction.length = 0; // length in bits
   transaction.tx_buffer = nullptr;
   transaction.rxlength = length * 8; // length in bits
   transaction.rx_buffer = data;
   return spi_device_transmit(m_spiHandle, &transaction) == ESP_OK;
}

void SPIDriver::reset()
{
   m_resetPin->setState(::drivers::gpio::State::LOW);
   vTaskDelay(pdMS_TO_TICKS(100));
   m_resetPin->setState(::drivers::gpio::State::HIGH);
   vTaskDelay(pdMS_TO_TICKS(100));
}

bool SPIDriver::waitForInterrupt(int timeout)
{
   int result = xSemaphoreTake(m_interruptMutex, pdMS_TO_TICKS(timeout));
   if (result == pdTRUE)
   {
      return true;
   }
   AIO_LOGE(TAG, "Timeout waiting for interrupt");
   return false;
}



} // namespace drivers::spi
