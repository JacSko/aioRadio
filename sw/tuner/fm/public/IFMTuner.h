#pragma once
#include <cstdint>
#include <functional>
#include "TunerTypes.h"
#include "IDeviceDriver.h"

namespace tuner::fm
{

enum class FMTunerEvent
{
   SCAN_STARTED,
   SCAN_STEP,
   SCAN_COMPLETED,
   AUDIO_PLAYBACK_STARTED,
   AUDIO_PLAYBACK_STOPPED,
   PROCESSING_REQUIRED,
};

struct CurrentStationInfo
{
   uint32_t frequencyKHz;
   uint8_t snr;
   int8_t rssi;
   bool valid;
   uint32_t piCode;
};

class FMTunerEventListener
{
public:
   virtual void onEvent(FMTunerEvent event) = 0;
   virtual ~FMTunerEventListener() = default;
};

class IFMTuner
{
public:
   static std::unique_ptr<IFMTuner> create(devicedriver::IDeviceDriver& deviceDriver);
   static const char* getEventName(FMTunerEvent);
   virtual ~IFMTuner() = default;

   virtual bool initialize() = 0;
   virtual void finalize() = 0;

   virtual void addEventListener(FMTunerEventListener* listener) = 0;
   virtual void removeEventListener(FMTunerEventListener* listener) = 0;

   virtual bool tuneFrequency(uint32_t frequencyKHz) = 0;
   virtual void scan() = 0;
   virtual int getScanProgress() = 0;
   virtual void process() = 0;

   virtual CurrentStationInfo getCurrentStationInfo() = 0;

};

}
