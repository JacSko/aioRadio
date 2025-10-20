#include "driver/i2c_master.h"

static i2cMock* g_i2cMock;

i2cMock* i2c_get_mock()
{
    return g_i2cMock;
}
void i2c_mock_init()
{
    g_i2cMock = new i2cMock();
}
void i2c_mock_deinit()
{
    delete g_i2cMock;
}
int i2c_new_master_bus(const i2c_master_bus_config_t* busConfig, i2c_master_bus_handle_t* busHandle)
{
    return g_i2cMock->i2c_new_master_bus(busConfig, busHandle);
}
int i2c_del_master_bus(i2c_master_bus_handle_t handle)
{
    return g_i2cMock->i2c_del_master_bus(handle);
}
int i2c_master_bus_add_device(i2c_master_bus_handle_t bus, const i2c_device_config_t* config, i2c_master_dev_handle_t* handle)
{
    return g_i2cMock->i2c_master_bus_add_device(bus, config, handle);
}
int i2c_master_bus_rm_device(i2c_master_dev_handle_t handle)
{
    return g_i2cMock->i2c_master_bus_rm_device(handle);
}
int i2c_master_probe(i2c_master_bus_handle_t handle, int address, int timeoutMs)
{
    return g_i2cMock->i2c_master_probe(handle, address, timeoutMs);
}
int i2c_master_transmit(i2c_master_dev_handle_t handle, const uint8_t *write_buffer, size_t write_size, int xfer_timeout_ms)
{
   return g_i2cMock->i2c_master_transmit(handle, write_buffer, write_size, xfer_timeout_ms);
}
int i2c_master_transmit_receive(i2c_master_dev_handle_t handle, const uint8_t *write_buffer, size_t write_size, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms)
{
   return g_i2cMock->i2c_master_transmit_receive(handle, write_buffer, write_size, read_buffer, read_size, xfer_timeout_ms);
}
