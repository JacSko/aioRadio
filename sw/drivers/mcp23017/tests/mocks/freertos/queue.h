#pragma once

struct QueueDefinition
{};
typedef QueueDefinition* QueueHandle_t;
typedef int UBaseType_t;
typedef int BaseType_t;


QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength,
                                  const UBaseType_t uxItemSize);
void vQueueDelete(QueueHandle_t handle);
BaseType_t xQueueSendFromISR( QueueHandle_t xQueue,
                              const void * const pvItemToQueue,
                              BaseType_t * const pxHigherPriorityTaskWokeni);
