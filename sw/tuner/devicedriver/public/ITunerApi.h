#pragma once

#include <cstdint>

#include "IDeviceDriver.h"
#include "TunerTypes.h"

namespace tuner::devicedriver
{

class ITunerApi
{
public:
   virtual ~ITunerApi() = default;

   static std::unique_ptr<ITunerApi> create(IDeviceDriver&);

   virtual bool setProperty(PropertyId id, uint16_t value) = 0;
   virtual bool getProperty(PropertyId id, uint16_t& value) = 0;

   virtual bool tuneFrequencyFM(uint32_t frequencyKHz) = 0;
   virtual bool getRSQStatus(RSQStatus& status) = 0;
   virtual bool getRDSStatus(RDSStatus& status) = 0;
   virtual bool tuneFrequencyDAB(uint32_t frequencyKHz) = 0;
   virtual bool setDABFrequencyList(const std::vector<uint32_t>& frequencyList) = 0;
   virtual bool getEnsembleInfo(EnsembleInfo& info) = 0;
   virtual bool getComponentInfo(uint32_t sid, uint32_t cmpid, DABComponent& info) = 0;
   virtual bool getDigradStatus(DigradStatus& info) = 0;
   virtual bool getDABEventStatus(DABEventStatus& eventStatus) = 0;
   virtual bool getDigitalServiceList(DigitalServiceList& serviceList) = 0;
   virtual bool startDigitalService(ServiceType type, uint32_t serviceId, uint32_t componentId) = 0;
   virtual bool stopDigitalService(ServiceType type, uint32_t serviceId, uint32_t componentId) = 0;
   virtual bool getDigitalServiceData(DigitalServiceData& data) = 0;
   virtual bool getAudioInfo(AudioInfo& info) = 0;
};

} // namespace tuner
