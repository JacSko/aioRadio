#pragma once
#include <cstdint>
#include <functional>
#include "TunerTypes.h"
#include "IDeviceDriver.h"

namespace tuner::dab
{

enum class DABTunerEvent
{
   SERVICE_LIST_CHANGED,
   SCAN_STARTED,
   SCAN_STEP,
   SCAN_COMPLETED,
   AUDIO_PLAYBACK_STARTED,
   AUDIO_PLAYBACK_STOPPED,
   PROCESSING_REQUIRED,
};

class DABTunerEventListener
{
public:
   virtual void onEvent(DABTunerEvent event) = 0;
   virtual ~DABTunerEventListener() = default;
};

struct CurrentEnsembleInfo
{
   uint16_t eid;
   std::string label;
};
struct CurrentServiceInfo
{
   uint32_t sid;
   std::string label;
};
struct CurrentComponentInfo
{
   uint32_t cmpid;
   std::string label;
};

struct CurrentStationInfo
{
   uint32_t frequencyKHz;
   CurrentEnsembleInfo ensembleInfo;
   CurrentServiceInfo serviceInfo;
   CurrentComponentInfo componentInfo;
};

class IDABTuner
{
public:
   static std::unique_ptr<IDABTuner> create(devicedriver::IDeviceDriver& deviceDriver);
   static const char* getEventName(DABTunerEvent);
   virtual ~IDABTuner() = default;

   virtual bool initialize() = 0;
   virtual void finalize() = 0;

   virtual void addEventListener(DABTunerEventListener* listener) = 0;
   virtual void removeEventListener(DABTunerEventListener* listener) = 0;

   virtual bool tuneFrequency(uint32_t frequencyKHz) = 0;
   virtual bool tuneAndSelect(uint32_t frequencyKHz, uint32_t sid, uint32_t cmpid) = 0;
   virtual void scan() = 0;
   virtual int getScanProgress() = 0;
   virtual void process() = 0;

   virtual CurrentStationInfo getCurrentStationInfo() = 0;
   virtual const DigitalServiceList& getDigitalServiceList() = 0;

};

}
