#pragma once
#include <cstdint>
#include "IDeviceDriver.h"

namespace audio
{

enum class AudioSource
{
   NONE,
   FM_RADIO,
   DAB_RADIO,
   ONLINE_RADIO,
};

class IAudioControl
{
public:
   static std::unique_ptr<IAudioControl> create(tuner::devicedriver::IDeviceDriver& deviceDriver);

   virtual ~IAudioControl() = default;
   virtual void volumeUp() = 0;
   virtual void volumeDown() = 0;
   virtual void setVolume(uint8_t volume) = 0;
   virtual uint8_t getVolume() const = 0;
   virtual void switchSource(AudioSource) = 0;
};


}
