#pragma once

typedef int esp_err_t;
#define IRAM_ATTR

#define ESP_OK   0
#define ESP_FAIL -1
static inline const char *esp_err_to_name(esp_err_t){return "ESP_OK";}
