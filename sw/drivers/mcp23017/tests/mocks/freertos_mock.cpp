#include "freertos/FreeRTOS.h"

static freeRtosMock* g_rtosMock;
SemaphoreObject g_semaphore;

freeRtosMock* rtos_get_mock()
{
   return g_rtosMock;
}
void rtos_mock_init()
{
   g_rtosMock = new freeRtosMock();
}
void rtos_mock_deinit()
{
   delete g_rtosMock;
}

/*
 * Mocks for FreeRTOS task.h functions
 */
void vTaskDelay(const TickType_t delay)
{
   return g_rtosMock->vTaskDelay(delay);
}

/*
 * Mocks for FreeRTOS queue.h functions
 */
void vTaskDelete(TaskHandle_t handle)
{
   g_rtosMock->vTaskDelete(handle);
}
int xTaskCreate(TaskFunction_t func,
                const char * const name,
                const configSTACK_DEPTH_TYPE stackDepth,
                void * const params,
                unsigned int priority,
                TaskHandle_t * const handle)
{
   return g_rtosMock->xTaskCreate(func, name, stackDepth, params, priority, handle);
}
void vTaskNotifyGiveFromISR(TaskHandle_t handle,
                            int* higherPriorityTaskWoken)
{
   return g_rtosMock->vTaskNotifyGiveFromISR(handle, higherPriorityTaskWoken);
}
uint32_t ulTaskNotifyTake(unsigned int clearCountOnExit,
                          TickType_t ticksToWait)
{
   return g_rtosMock->ulTaskNotifyTake(clearCountOnExit, ticksToWait);
}

/*
 * Mocks for FreeRTOS queue.h functions
 */
QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength,
                           const UBaseType_t uxItemSize)
{
   return g_rtosMock->xQueueCreate(uxQueueLength, uxItemSize);
}
void vQueueDelete(QueueHandle_t handle)
{
   return g_rtosMock->vQueueDelete(handle);
}
BaseType_t xQueueSendFromISR( QueueHandle_t xQueue,
                              const void * const pvItemToQueue,
                              BaseType_t * const pxHigherPriorityTaskWokeni)
{
   return g_rtosMock->xQueueSendFromISR(xQueue, pvItemToQueue, pxHigherPriorityTaskWokeni);
}

/*
 * Mocks for FreeRTOS semphr.h functions
 */
SemaphoreHandle_t xSemaphoreCreateMutex()
{
   return &g_semaphore;
}
BaseType_t xSemaphoreTake(SemaphoreHandle_t, int)
{
   return 0;
}
BaseType_t xSemaphoreGive(SemaphoreHandle_t)
{
   return 0;
}
void vSemaphoreDelete(SemaphoreHandle_t)
{
}
