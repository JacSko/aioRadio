#include <algorithm>

#include "TunerApiImpl.h"
#include "TunerTypes.h"
#include "Commands.h"
#include "FrequencyTable.h"

namespace tuner::devicedriver
{
std::unique_ptr<ITunerApi> ITunerApi::create(IDeviceDriver& driver)
{
   return std::make_unique<TunerApiImpl>(driver);
}

TunerApiImpl::TunerApiImpl(IDeviceDriver& driver):
m_driver(driver)
{
}
TunerApiImpl::~TunerApiImpl()
{
}
FrameTransmit TunerApiImpl::createFrame(CommandBuffer& command, EventType eventType, uint32_t timeoutMs)
{
   FrameTransmit frame = {};
   frame.txBuffer = command.m_txBuffer.data();
   frame.txSize = command.m_txBuffer.size();
   frame.rxBuffer = command.m_rxBuffer.data();
   frame.rxSize = command.m_rxBuffer.size();
   frame.eventType = eventType;
   frame.eventTimeout = timeoutMs;
   return frame;
}
bool TunerApiImpl::setProperty(PropertyId id, uint16_t value)
{
   AIO_LOGI(TAG, "Setting property 0x%.4X to value 0x%.4X", static_cast<uint16_t>(id), value);
   SetProperty property(id, value);
   FrameTransmit frame = createFrame(property, EventType::CTS, 500);
   bool result = m_driver.transmit(frame);
   result &= property.process();
   return result;
}
bool TunerApiImpl::getProperty(PropertyId id, uint16_t& value)
{
   GetProperty property(id);
   FrameTransmit frame = createFrame(property, EventType::CTS, 500);
   bool result = m_driver.transmit(frame);
   result &= property.process();
   value = property.propertyValue;
   return result;
}
bool TunerApiImpl::tuneFrequencyFM(uint32_t frequencyKHz)
{
   FMTuneFrequency command(frequencyKHz, false, 0x00, 0x00);
   FrameTransmit frame = createFrame(command, EventType::STCINT, 2000);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   return result;
}
bool TunerApiImpl::getRSQStatus(RSQStatus& status)
{
   GetRSQStatus command(true, false, false, true);
   FrameTransmit frame = createFrame(command, EventType::CTS, 500);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   status = command.status;
   return result;
}
bool TunerApiImpl::getRDSStatus(RDSStatus& status)
{
   GetRDSStatus command(false, false, true);
   FrameTransmit frame = createFrame(command, EventType::CTS, 500);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   status = command.status;
   return result;
}
bool TunerApiImpl::tuneFrequencyDAB(uint32_t frequencyKHz)
{
   auto frequencyTable = getDABFrequencyTable();
   auto it = std::find_if(frequencyTable.begin(), frequencyTable.end(),
                          [frequencyKHz](const std::pair<uint32_t, const char*>& entry)
                          {
                             return entry.first == frequencyKHz;
                          });
   if (it == frequencyTable.end())
   {
      AIO_LOGE(TAG, "DAB frequency %u kHz not found in frequency table!", frequencyKHz);
      return false;
   }
   DABTuneFrequency command(std::distance(frequencyTable.begin(), it), 0x00, 0x00);
   FrameTransmit frame = createFrame(command, EventType::STCINT, 2000);
   AIO_LOGE(TAG, "Starting tune to DAB frequency %u kHz", frequencyKHz);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   AIO_LOGE(TAG, "DAB tune to frequency %u kHz %s", frequencyKHz, result ? "succeeded" : "failed");
   return result;
}
bool TunerApiImpl::setDABFrequencyList(const std::vector<uint32_t>& frequencyList)
{
   DABSetFrequencyList command(frequencyList);
   FrameTransmit frame = createFrame(command, EventType::CTS, 2000);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   return result;
}
bool TunerApiImpl::getEnsembleInfo(EnsembleInfo& info)
{
   GetEnsembleInfo command;
   FrameTransmit frame = createFrame(command, EventType::CTS, 500);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   info = command.ensembleInfo;
   return result;
}
bool TunerApiImpl::getComponentInfo(uint32_t sid, uint32_t cmpid, DABComponent& info)
{
   GetComponentInfo command(sid, cmpid);
   FrameTransmit frame = createFrame(command, EventType::CTS, 500);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   info = command.componentInfo;
   return result;
}
bool TunerApiImpl::getDigradStatus(DigradStatus& info)
{
   GetDABDigradStatus command(true, false, false, true);
   FrameTransmit frame = createFrame(command, EventType::CTS, 500);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   info = command.digradStatus;
   return result;
}
bool TunerApiImpl::getDABEventStatus(DABEventStatus& eventStatus)
{
   GetDABEventStatus command(true);
   FrameTransmit frame = createFrame(command, EventType::CTS, 500);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   eventStatus = command.eventStatus;
   return result;
}
bool TunerApiImpl::getDigitalServiceList(DigitalServiceList& serviceList)
{
   GetDigitalServiceList command(static_cast<uint8_t>(ServiceType::DATA_SERVICE));
   FrameTransmit frame = createFrame(command, EventType::CTS, 2000);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   serviceList = command.serviceList;
   return result;
}
bool TunerApiImpl::startDigitalService(ServiceType type, uint32_t serviceId, uint32_t componentId)
{
   StartDigitalService command(static_cast<uint8_t>(type), serviceId, componentId);
   FrameTransmit frame = createFrame(command, EventType::CTS, 2000);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   return result;
}
bool TunerApiImpl::stopDigitalService(ServiceType type, uint32_t serviceId, uint32_t componentId)
{
   StopDigitalService command(static_cast<uint8_t>(type), serviceId, componentId);
   FrameTransmit frame = createFrame(command, EventType::CTS, 2000);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   return result;
}
bool TunerApiImpl::getDigitalServiceData(DigitalServiceData& data)
{
   GetDigitalServiceData command(true, false);
   FrameTransmit frame = createFrame(command, EventType::CTS, 2000);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   data = command.digitalServiceData;
   return result;
}
bool TunerApiImpl::getAudioInfo(AudioInfo& info)
{
   DABGetAudioInfo command;
   FrameTransmit frame = createFrame(command, EventType::CTS, 500);
   bool result = m_driver.transmit(frame);
   result &= command.process();
   info = command.audioInfo;
   return result;
}

}  // namespace tuner
