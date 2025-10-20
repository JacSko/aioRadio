#pragma once

/**
 * @file SPIDriver.h
 *
 * @brief SPI Driver designed to serve communication with external tuner chipset.
 *
 * @details
 * This drivers purpose is to allow efficient communication with an external tuner chipset via SPI protocol.
 * Additionaly, it handles reset and interrupt line through GPIOs defined in config.
 *
 */
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "GPIOProvider.h"
#include <cstdint>

namespace drivers::spi
{
/**
 * @struct SPIDriverConfig
 *
 * @brief Configuration structure for SPIDriver.
 *
 * @details
 * This structure holds the configuration parameters required to initialize and operate the SPIDriver.
 * It includes pin assignments for MISO, MOSI, SCK, CS, interrupt, and reset lines, as well as SPI clock speed and mode.
 */
struct SPIDriverConfig
{
   spi_host_device_t host;            /*< ESP SPI host ID */
   int misoPin;      /*< Pin number for Master In Slave Out (MISO) line */
   int mosiPin;      /*< Pin number for Master Out Slave In (MOSI) line */
   int sckPin;       /*< Pin number for Serial Clock (SCK) line */
   int csPin;        /*< Pin number for Chip Select (CS) line */
   int clockSpeedHz; /*< SPI clock speed */
   uint8_t mode;     /*< SPI mode (0-3) */
   ::drivers::gpio::GPIO interruptPin; /*< GPIO pin for interrupt line */
   ::drivers::gpio::GPIO resetPin;     /*< GPIO pin for reset line */
};

class SPIDriver : public ::drivers::gpio::GPIOListener
{
public:
   SPIDriver(const SPIDriverConfig& config, ::drivers::gpio::GPIOProvider& gpioProvider);
   ~SPIDriver();

   /** @brief Initializes the SPI driver and configures GPIOs.
    *
    * @return true if initialization is successful, false otherwise.
    */
   bool initialize();

   /** @brief Writes data to the SPI bus.
    *
    * @param[in] data - Pointer to the data buffer to be written.
    * @param[in] length - Length of the data buffer.
    * @return true if write operation is successful, false otherwise.
    */
   bool write(const uint8_t* data, size_t length);

   /** @brief Reads data from the SPI bus.
    *
    * @param[out] data - Pointer to the buffer where read data will be stored.
    * @param[in] length - Length of the data to be read.
    * @return true if read operation is successful, false otherwise.
    */
   bool read(uint8_t* data, size_t length);

   /** @bried Finalizes the SPI driver and releases resources.
    *  @return None
    */
   void finalize();

   /** @brief Resets the connected device using the reset GPIO pin.
    *
    * @return None
    */
   void reset();

   /** @brief Waits for an interrupt event on the interrupt GPIO pin.
    *
    * @param[in] timeout - Timeout in milliseconds to wait for the interrupt.
    * @return true if an interrupt event occurred within the timeout period, false otherwise.
    */
   bool waitForInterrupt(int timeout);

private:
   void onInterrupt(::drivers::gpio::GPIO id, ::drivers::gpio::State state) override;

   SPIDriverConfig m_config;
   ::drivers::gpio::GPIOProvider& m_gpioProvider;
   std::unique_ptr<::drivers::gpio::IGPIO> m_interruptPin;
   std::unique_ptr<::drivers::gpio::IGPIO> m_resetPin;
   SemaphoreHandle_t m_interruptMutex;
   spi_device_handle_t m_spiHandle;
   TaskHandle_t m_task;
};

} // namespace drivers::spi
