#pragma once

#include <stdint.h>
#include <memory>

#include "esp_common.h"
#include "gmock/gmock.h"

#define I2C_CLK_SRC_DEFAULT 0
#define I2C_ADDR_BIT_LEN_7 7

typedef int i2c_port_num_t;
struct i2c_master_bus_handle
{};
struct i2c_master_dev_handle
{};
typedef i2c_master_bus_handle* i2c_master_bus_handle_t;
typedef i2c_master_dev_handle* i2c_master_dev_handle_t;

typedef struct {
   i2c_port_num_t i2c_port;
   int sda_io_num;
   int scl_io_num;
   int clk_source;
   int glitch_ignore_cnt;
   int intr_priority;
} i2c_master_bus_config_t;

typedef struct {
   int dev_addr_length;
   int device_address;
   uint32_t scl_speed_hz;
   int scl_wait_us;
   struct {
      int disable_ack_check;
   } flags;
} i2c_device_config_t;

struct i2cMock
{
   MOCK_METHOD(int, i2c_new_master_bus, (const i2c_master_bus_config_t*, i2c_master_bus_handle_t*), ());
   MOCK_METHOD(int, i2c_del_master_bus, (i2c_master_bus_handle_t), ());
   MOCK_METHOD(int, i2c_master_bus_add_device, (i2c_master_bus_handle_t, const i2c_device_config_t*, i2c_master_dev_handle_t*), ());
   MOCK_METHOD(int, i2c_master_bus_rm_device, (i2c_master_dev_handle_t), ());
   MOCK_METHOD(int, i2c_master_probe, (i2c_master_bus_handle_t, int, int), ());
   MOCK_METHOD(int, i2c_master_transmit, (i2c_master_dev_handle_t, const uint8_t *, size_t write_size, int xfer_timeout_ms),());
   MOCK_METHOD(int, i2c_master_transmit_receive, (i2c_master_dev_handle_t, const uint8_t *, size_t write_size, uint8_t*, size_t, int xfer_timeout_ms),());
};

i2cMock* i2c_get_mock();
void i2c_mock_init();
void i2c_mock_deinit();
int i2c_new_master_bus(const i2c_master_bus_config_t* busConfig, i2c_master_bus_handle_t* busHandle);
int i2c_del_master_bus(i2c_master_bus_handle_t handle);
int i2c_master_bus_add_device(i2c_master_bus_handle_t bus, const i2c_device_config_t* config, i2c_master_dev_handle_t* handle);
int i2c_master_bus_rm_device(i2c_master_dev_handle_t handle);
int i2c_master_probe(i2c_master_bus_handle_t handle, int address, int timeoutMs);
int i2c_master_transmit(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, int xfer_timeout_ms);
int i2c_master_transmit_receive(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms);
