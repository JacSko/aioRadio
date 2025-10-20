#pragma once
#include "ITunerApi.h"
#include "Commands.h"

namespace tuner::devicedriver
{

class TunerApiImpl : public ITunerApi
{

public:
   TunerApiImpl(IDeviceDriver& driver);
   virtual ~TunerApiImpl();
   bool setProperty(PropertyId id, uint16_t value) override;
   bool getProperty(PropertyId id, uint16_t& value) override;
   bool tuneFrequencyFM(uint32_t frequencyKHz) override;
   bool getRSQStatus(RSQStatus& status) override;
   bool getRDSStatus(RDSStatus& status) override;
   bool tuneFrequencyDAB(uint32_t frequencyKHz) override;
   bool setDABFrequencyList(const std::vector<uint32_t>& frequencyList) override;
   bool getEnsembleInfo(EnsembleInfo& info) override;
   bool getComponentInfo(uint32_t sid, uint32_t cmpid, DABComponent& info) override;
   bool getDigradStatus(DigradStatus& info) override;
   bool getDABEventStatus(DABEventStatus& eventStatus) override;
   bool getDigitalServiceList(DigitalServiceList& serviceList) override;
   bool startDigitalService(ServiceType type, uint32_t serviceId, uint32_t componentId) override;
   bool stopDigitalService(ServiceType type, uint32_t serviceId, uint32_t componentId) override;
   bool getDigitalServiceData(DigitalServiceData& data) override;
   bool getAudioInfo(AudioInfo& info) override;

private:
   FrameTransmit createFrame(CommandBuffer& command, EventType eventType, uint32_t timeoutMs);

   IDeviceDriver& m_driver;
};

}  // namespace tuner
