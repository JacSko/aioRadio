#pragma once
#include "gmock/gmock.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define pdFALSE 0
#define pdTRUE  1

#define pdPASS pdTRUE
#define pdFAIL pdFALSE
#define pdMS_TO_TICKS(time) time
#define portMAX_DELAY ((TickType_t)0xffffffffUL)

struct freeRtosMock
{
   // Task mocks
   MOCK_METHOD(void, vTaskDelay, (const TickType_t), ());
   MOCK_METHOD(int, xTaskCreate, (TaskFunction_t,
                                  const char * const,
                                  const configSTACK_DEPTH_TYPE,
                                  void * const,
                                  unsigned int,
                                  TaskHandle_t * const), ());
   MOCK_METHOD(uint32_t, ulTaskNotifyTake, (unsigned int,
                                            TickType_t), ());
   MOCK_METHOD(void, vTaskNotifyGiveFromISR, (TaskHandle_t,
                                             int*), ());
   MOCK_METHOD(void, vTaskDelete, (TaskHandle_t), ());
   // Queue mocks
   MOCK_METHOD(QueueHandle_t, xQueueCreate, (const UBaseType_t,
                                             const UBaseType_t), ());
   MOCK_METHOD(void, vQueueDelete, (QueueHandle_t), ());
   MOCK_METHOD(BaseType_t, xQueueSendFromISR, (QueueHandle_t,
                                               const void * const,
                                               BaseType_t * const), ());
   MOCK_METHOD(BaseType_t, xQueueSend, (QueueHandle_t,
                                        const void * const,
                                        BaseType_t * const), ());
   MOCK_METHOD(BaseType_t, xQueueReceive, (QueueHandle_t,
                                           void *,
                                           uint32_t), ());
};

void rtos_mock_init();
void rtos_mock_deinit();
freeRtosMock* rtos_get_mock();
