#pragma once
#include <memory>

#include "gmock/gmock.h"
#include "esp_common.h"

#define SPI_DEVICE_HALFDUPLEX              (1<<4)

typedef enum {
    SPI_DMA_DISABLED = 0,
    SPI_DMA_CH_AUTO  = 3,
} spi_common_dma_t;

typedef enum {
    SPI1_HOST = 0,
    SPI2_HOST = 1,
    SPI3_HOST = 2,
    SPI_HOST_MAX,
} spi_host_device_t;

typedef struct {
   int mosi_io_num;
   int miso_io_num;
   int sclk_io_num;
   int quadwp_io_num;
   int quadhd_io_num;
   int max_transfer_sz;
} spi_bus_config_t;

typedef struct {
   uint8_t command_bits;
   uint8_t address_bits;
   uint8_t dummy_bits;
   uint8_t mode;
   uint16_t duty_cycle_pos;
   uint16_t cs_ena_pretrans;
   uint8_t cs_ena_posttrans;
   int clock_speed_hz;
   int input_delay_ns;
   int spics_io_num;
   uint32_t flags;
   int queue_size;
} spi_device_interface_config_t;

struct spi_device_t
{
};
typedef struct spi_device_t *spi_device_handle_t;  ///< Handle for a device on a SPI bus

struct spi_transaction_t {
    uint32_t flags;
    size_t length;
    size_t rxlength;
    void *user;
    union {
        const void *tx_buffer;
        uint8_t tx_data[4];
    };
    union {
        void *rx_buffer;
        uint8_t rx_data[4];
    };
};


struct SPIMasterMock
{
   MOCK_METHOD(esp_err_t, spi_bus_initialize, (int host_id, const spi_bus_config_t *bus_config, spi_common_dma_t dma_chan), ());
   MOCK_METHOD(esp_err_t, spi_bus_add_device, (spi_host_device_t host_id, const spi_device_interface_config_t *dev_config, spi_device_handle_t *handle), ());
   MOCK_METHOD(esp_err_t, spi_bus_remove_device, (spi_device_handle_t),());
   MOCK_METHOD(esp_err_t, spi_device_transmit, (spi_device_handle_t handle, spi_transaction_t *trans), ());
};

void spiMasterMockInit();
void spiMasterMockDeinit();
SPIMasterMock* getSPIMasterMock();
int spi_bus_initialize(int host_id, const spi_bus_config_t *bus_config, spi_common_dma_t dma_chan);
int spi_bus_add_device(spi_host_device_t host_id, const spi_device_interface_config_t *dev_config, spi_device_handle_t *handle);
int spi_bus_remove_device(spi_device_handle_t handle);
int spi_device_transmit(spi_device_handle_t handle, spi_transaction_t *trans);
