#include "driver/gpio.h"

static gpioMock* g_gpioMock;

gpioMock* gpio_get_mock()
{
    return g_gpioMock;
}
void gpio_mock_init()
{
    g_gpioMock = new gpioMock();
}
void gpio_mock_deinit()
{
    delete g_gpioMock;
}
int gpio_config(const gpio_config_t* config)
{
    return g_gpioMock->gpio_config(config);
}
int gpio_isr_handler_add(gpio_num_t num, void (*handler)(void*), void* ctx)
{
    return g_gpioMock->gpio_isr_handler_add(num, handler, ctx);
}
int gpio_isr_handler_remove(gpio_num_t num)
{
    return g_gpioMock->gpio_isr_handler_remove(num);
}
int gpio_reset_pin(gpio_num_t num)
{
    return g_gpioMock->gpio_reset_pin(num);
}
int gpio_set_level(gpio_num_t num, bool state)
{
    return g_gpioMock->gpio_set_level(num, state);
}
