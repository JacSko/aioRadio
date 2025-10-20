#pragma once

#define FILENAME (strrchr(__FILE__,'/')?strrchr(__FILE__,'/')+1:__FILE__)

#ifndef UNIT_TESTS
#include "esp_log.h"
#define AIO_LOGE(TAG, format, ...) ESP_LOGE(TAG, "%s:%d - " format, FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGW(TAG, format, ...) ESP_LOGW(TAG, "%s:%d - " format, FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGI(TAG, format, ...) ESP_LOGI(TAG, "%s:%d - " format, FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGD(TAG, format, ...) ESP_LOGD(TAG, "%s:%d - " format, FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGV(TAG, format, ...) ESP_LOGV(TAG, "%s:%d - " format, FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGE_IF(cond, TAG, format, ...) if(cond) ESP_LOGE(TAG, "%s:%d - " format, FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGW_IF(cond, TAG, format, ...) if(cond) ESP_LOGW(TAG, "%s:%d - " format, FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGI_IF(cond, TAG, format, ...) if(cond) ESP_LOGI(TAG, "%s:%d - " format, FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGD_IF(cond, TAG, format, ...) if(cond) ESP_LOGD(TAG, "%s:%d - " format, FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGV_IF(cond, TAG, format, ...) if(cond) ESP_LOGV(TAG, "%s:%d - " format, FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_ASSERT(cond, tag, format, ...) if(!(cond)) { ESP_LOGE(tag, "ASSERTION FAILED: %s:%d - " format, FILENAME, __LINE__, ##__VA_ARGS__); std::abort(); }

#else
#include <iostream>
#define AIO_LOGE(tag, format, ...) printf("[E] %s:%d - " format "\n", FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGW(tag, format, ...) printf("[W] %s:%d - " format "\n", FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGI(tag, format, ...) printf("[I] %s:%d - " format "\n", FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGD(tag, format, ...) printf("[D] %s:%d - " format "\n", FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGV(tag, format, ...) printf("[V] %s:%d - " format "\n", FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGE_IF(cond, tag, format, ...) if(cond) printf("[E] %s:%d - " format "\n", FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGW_IF(cond, tag, format, ...) if(cond) printf("[W] %s:%d - " format "\n", FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGI_IF(cond, tag, format, ...) if(cond) printf("[I] %s:%d - " format "\n", FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGD_IF(cond, tag, format, ...) if(cond) printf("[D] %s:%d - " format "\n", FILENAME, __LINE__, ##__VA_ARGS__);
#define AIO_LOGV_IF(cond, tag, format, ...) if(cond) printf("[V] %s:%d - " format "\n", FILENAME, __LINE__, ##__VA_ARGS__);
#endif
