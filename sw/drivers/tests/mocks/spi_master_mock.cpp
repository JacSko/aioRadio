#include "driver/spi_master.h"


std::unique_ptr<SPIMasterMock> g_spiMasterMock = nullptr;

void spiMasterMockInit()
{
   g_spiMasterMock = std::make_unique<SPIMasterMock>();
}

void spiMasterMockDeinit()
{
   g_spiMasterMock.reset();
}

SPIMasterMock* getSPIMasterMock()
{
   return g_spiMasterMock.get();
}

int spi_bus_initialize(int host_id, const spi_bus_config_t *bus_config, spi_common_dma_t dma_chan)
{
   return g_spiMasterMock->spi_bus_initialize(host_id, bus_config, dma_chan);
}

int spi_bus_add_device(spi_host_device_t host_id, const spi_device_interface_config_t *dev_config, spi_device_handle_t *handle)
{
   return g_spiMasterMock->spi_bus_add_device(host_id, dev_config, handle);
}
int spi_bus_remove_device(spi_device_handle_t handle)
{
   return g_spiMasterMock->spi_bus_remove_device(handle);
}
int spi_device_transmit(spi_device_handle_t handle, spi_transaction_t *trans)
{
   return g_spiMasterMock->spi_device_transmit(handle, trans);
}
