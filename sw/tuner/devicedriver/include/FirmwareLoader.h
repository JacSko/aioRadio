#pragma once
#include <array>

#include "SPIDriver.h"
#include "TunerTypes.h"

namespace tuner::devicedriver
{

struct FirmwareData
{
   const uint8_t* data;
   unsigned int size;
};

struct FirmwareImages
{
   FirmwareData patch;
   FirmwareData firmware;
};

class FirmwareLoader
{
public:
   FirmwareLoader(drivers::spi::SPIDriver&);
   ~FirmwareLoader();

   bool boot(TunerType type);
private:
   bool powerUp();
   bool loadInit();
   bool sendImage(const uint8_t* image, size_t bytesToSend);
   bool bootDevice();
   bool waitCTS();
   drivers::spi::SPIDriver& m_driver;
   std::array<uint8_t, 4096> m_buffer;
};


}
