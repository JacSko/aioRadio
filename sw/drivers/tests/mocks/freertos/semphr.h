#pragma once
#include "gmock/gmock.h"

struct SemaphoreObject
{};
typedef SemaphoreObject* SemaphoreHandle_t;

SemaphoreHandle_t xSemaphoreCreateMutex();
BaseType_t xSemaphoreTake(SemaphoreHandle_t, int);
BaseType_t xSemaphoreGive(SemaphoreHandle_t);
void vSemaphoreDelete(SemaphoreHandle_t);
