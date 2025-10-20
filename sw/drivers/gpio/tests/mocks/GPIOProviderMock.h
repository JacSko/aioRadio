#pragma once
#include <gmock/gmock.h>
#include "IGPIO.h"

namespace drivers::gpio
{

class GPIOProviderMock
{
public:
    MOCK_METHOD(std::unique_ptr<IGPIO>, create, (GPIO), ());
};

std::unique_ptr<GPIOProviderMock> g_gpioProviderMock;

GPIOProviderMock* getGPIOProviderMock()
{
   return g_gpioProviderMock.get();
}

GPIOProvider::GPIOProvider(const mcp23017::i2cConfig& config){
   g_gpioProviderMock = std::make_unique<GPIOProviderMock>();
}
GPIOProvider::~GPIOProvider(){
   g_gpioProviderMock.reset();
}
std::unique_ptr<IGPIO> GPIOProvider::create(GPIO gpio)
{
   return std::move(g_gpioProviderMock->create(gpio));
}

} // namespace drivers::gpio
