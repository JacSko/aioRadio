#pragma once
#include "gmock/gmock.h"

#define portTICK_PERIOD_MS 1

typedef unsigned int TickType_t;
typedef void (* TaskFunction_t)( void * );
typedef int configSTACK_DEPTH_TYPE;

struct tskTaskControlBlock;
typedef tskTaskControlBlock* TaskHandle_t;

void vTaskDelete(TaskHandle_t);
int xTaskCreate(TaskFunction_t,
                const char * const,
                const configSTACK_DEPTH_TYPE,
                void * const,
                unsigned int,
                TaskHandle_t * const);
void vTaskNotifyGiveFromISR(TaskHandle_t,
                            int*);
uint32_t ulTaskNotifyTake(unsigned int,
                          TickType_t);
void vTaskDelay(const TickType_t xTicksToDelay);
