#pragma once

#include <stdint.h>
#include "esp_common.h"
#include "gmock/gmock.h"

typedef int gpio_int_type_t;
typedef int gpio_pull_mode_t;
typedef int gpio_mode_t;

typedef struct {
   uint64_t pin_bit_mask;
   int mode;
   int pull_up_en;
   int pull_down_en;
   int intr_type;
} gpio_config_t;

typedef enum {
   GPIO_NUM_0 = 0,
   GPIO_NUM_1 = 1,
   GPIO_NUM_2 = 2,
   GPIO_NUM_3 = 3,
   GPIO_NUM_4 = 4,
   GPIO_NUM_5 = 5,
   GPIO_NUM_6 = 6,
   GPIO_NUM_7 = 7,
   GPIO_NUM_8 = 8,
   GPIO_NUM_9 = 9,
   GPIO_NUM_10 = 10,
   GPIO_NUM_11 = 11,
   GPIO_NUM_12 = 12,
   GPIO_NUM_13 = 13,
   GPIO_NUM_14 = 14,
   GPIO_NUM_15 = 15,
   GPIO_NUM_16 = 16,
   GPIO_NUM_17 = 17,
   GPIO_NUM_18 = 18,
   GPIO_NUM_19 = 19,
   GPIO_NUM_20 = 20,
   GPIO_NUM_21 = 21,
   GPIO_NUM_22 = 22,
   GPIO_NUM_23 = 23,
   GPIO_NUM_24 = 24,
   GPIO_NUM_25 = 25,
   GPIO_NUM_26 = 26,
   GPIO_NUM_27 = 27,
   GPIO_NUM_28 = 28,
   GPIO_NUM_29 = 29,
   GPIO_NUM_30 = 30,
   GPIO_NUM_31 = 31,
   GPIO_NUM_32 = 32,
   GPIO_NUM_33 = 33,
   GPIO_NUM_34 = 34,
   GPIO_NUM_35 = 35,
   GPIO_NUM_36 = 36,
   GPIO_NUM_37 = 37,
   GPIO_NUM_38 = 38,
   GPIO_NUM_39 = 39,
   GPIO_NUM_MAX
}gpio_num_t;

#define GPIO_IS_VALID_GPIO(gpio_num) is_valid_gpio(gpio_num)
#define GPIO_MODE_INPUT 0x01
#define GPIO_MODE_OUTPUT 0x02
#define GPIO_PULLUP_ENABLE 1
#define GPIO_PULLUP_DISABLE 0
#define GPIO_PULLDOWN_ENABLE 1
#define GPIO_PULLDOWN_DISABLE 0
#define GPIO_INTR_DISABLE 0
#define GPIO_INTR_POSEDGE 1
#define GPIO_INTR_NEGEDGE 2
#define GPIO_INTR_ANYEDGE 3
#define GPIO_IS_VALID_OUTPUT_GPIO(gpio_num) is_valid_output_gpio(gpio_num)
#define GPIO_FLOATING 0
#define GPIO_PULLUP_ONLY 1
#define GPIO_PULLDOWN_ONLY 2
#define GPIO_PULLUP_PULLDOWN 3


struct gpioMock
{
   MOCK_METHOD(int, gpio_config, (const gpio_config_t*), ());
   MOCK_METHOD(int, gpio_isr_handler_add, (gpio_num_t, void (*)(void*), void*), ());
   MOCK_METHOD(int, gpio_isr_handler_remove, (gpio_num_t), ());
   MOCK_METHOD(int, gpio_reset_pin, (gpio_num_t), ());
   MOCK_METHOD(int, gpio_set_level, (gpio_num_t, bool), ());
   MOCK_METHOD(int, gpio_get_level, (gpio_num_t), ());
   MOCK_METHOD(bool, is_valid_gpio, (gpio_num_t), ());
   MOCK_METHOD(bool, is_valid_output_gpio, (gpio_num_t), ());
   MOCK_METHOD(int, gpio_set_intr_type, (gpio_num_t, gpio_int_type_t), ());
   MOCK_METHOD(int, gpio_set_pull_mode, (gpio_num_t, int), ());
   MOCK_METHOD(int, gpio_set_direction, (gpio_num_t, int), ());
};

void gpio_mock_init();
void gpio_mock_deinit();
gpioMock* gpio_get_mock();
int gpio_config(const gpio_config_t*);
int gpio_isr_handler_add(gpio_num_t, void (*)(void*), void*);
int gpio_isr_handler_remove(gpio_num_t);
int gpio_reset_pin(gpio_num_t);
int gpio_set_level(gpio_num_t, bool);
int gpio_get_level(gpio_num_t);
bool is_valid_gpio(gpio_num_t gpio_num);
bool is_valid_output_gpio(gpio_num_t gpio_num);
int gpio_set_intr_type(gpio_num_t, gpio_int_type_t);
int gpio_set_pull_mode(gpio_num_t, int);
int gpio_set_direction(gpio_num_t, int);
