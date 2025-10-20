#include "Log.h"
#include "DABTunerImpl.h"
#include "FrequencyTable.h"

namespace tuner::dab
{

static const char* TAG = "DABTuner";

std::unique_ptr<IDABTuner> IDABTuner::create(devicedriver::IDeviceDriver& deviceDriver)
{
   return std::make_unique<DABTunerImpl>(deviceDriver);
}

const char* IDABTuner::getEventName(DABTunerEvent event)
{
   switch(event)
   {
      case DABTunerEvent::SERVICE_LIST_CHANGED:
         return "SERVICE_LIST_CHANGED";
      case DABTunerEvent::SCAN_STARTED:
         return "SCAN_STARTED";
      case DABTunerEvent::SCAN_STEP:
         return "SCAN_STEP";
      case DABTunerEvent::SCAN_COMPLETED:
         return "SCAN_COMPLETED";
      case DABTunerEvent::AUDIO_PLAYBACK_STARTED:
         return "AUDIO_PLAYBACK_STARTED";
      case DABTunerEvent::AUDIO_PLAYBACK_STOPPED:
         return "AUDIO_PLAYBACK_STOPPED";
      default:
         return "UNKNOWN_EVENT";
   }
}

DABTunerImpl::DABTunerImpl(devicedriver::IDeviceDriver& deviceDriver) :
m_deviceDriver(deviceDriver),
m_tunerApi(devicedriver::ITunerApi::create(deviceDriver)),
m_task(nullptr),
m_csi({}),
m_scanProgress(0)
{
   AIO_LOGI(TAG, "Creating DABTuner");
}
DABTunerImpl::~DABTunerImpl()
{
}
bool DABTunerImpl::tuneToFrequency(uint32_t frequencyKHz)
{
   if (frequencyKHz == m_csi.frequencyKHz)
   {
      AIO_LOGD(TAG, "Already tuned to %u kHz", frequencyKHz);
      return true;
   }

   m_csi = {};
   m_ensembleInfo = {};
   m_serviceList = {};

   bool result = m_tunerApi->tuneFrequencyDAB(frequencyKHz);
   if (!result)
   {
      AIO_LOGE(TAG, "Tune to %u kHz failed", frequencyKHz);
      return false;
   }

   AIO_LOGD(TAG, "Tune to %u kHz, checking signal condition", frequencyKHz);
   DigradStatus digradStatus = {};
   result = m_tunerApi->getDigradStatus(digradStatus);
   if (!result)
   {
      AIO_LOGE(TAG, "Error reading DigradStatus on frequency %u kHz", frequencyKHz);
      return false;
   }
   AIO_LOGD(TAG, "Digrad - valid %u, acq %u, fic_quality %u", digradStatus.valid, digradStatus.acq, digradStatus.ficQuality);

   if (digradStatus.valid)
   {
      m_csi.frequencyKHz = frequencyKHz;
      AIO_LOGD(TAG, "Waiting for service list update event");
      int eventsReady = 0;
      {
         std::lock_guard lock(m_taskMutex);
         m_task = xTaskGetCurrentTaskHandle();
         eventsReady = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000));
         m_task = nullptr;
      }

      if (!eventsReady)
      {
         AIO_LOGE(TAG, "Timeout waiting for service list on %u kHz", frequencyKHz);
         return false;
      }

      DABEventStatus status;
      m_tunerApi->getDABEventStatus(status);
      AIO_LOGE(TAG, "Event status, srvlistint %u, srvlist %u", status.srvlistInt, status.srvlist);
      if (status.srvlistInt && status.srvlist)
      {
         EnsembleInfo ensembleInfo;
         DigitalServiceList serviceList;
         bool result = readEnsembleConfiguration(ensembleInfo, serviceList);
         if (result)
         {
            m_ensembleInfo = ensembleInfo;
            m_serviceList = serviceList;
         }

         notifyEvent(DABTunerEvent::SERVICE_LIST_CHANGED);
      }
      AIO_LOGD(TAG, "Service list is ready");
      return true;
   }
   else
   {
      AIO_LOGE(TAG, "No valid signal on %u kHz found!", frequencyKHz);
      return false;
   }
}
bool DABTunerImpl::startAudioService(uint32_t sid, uint32_t cmpid)
{
   uint32_t sidToTune = 0;
   uint32_t cmpidToTune = 0;

   if (sid == SID_ANY)
   {
      DABService service = getFirstAudioService().value_or(DABService());
      DABComponent cmpid = getPrimaryComponent(service.id).value_or(DABComponent());
      AIO_LOGE_IF(service.id == 0, TAG, "Cannot find any audio service on %u kHz", m_csi.frequencyKHz);
      sidToTune = service.id;
      cmpidToTune = cmpid.id;
   }
   else if (cmpid == CMPID_ANY)
   {
      DABComponent cmpid = getPrimaryComponent(sid).value_or(DABComponent());
      sidToTune = sid;
      cmpidToTune = cmpid.id;
   }
   else
   {
      sidToTune = sid;
      cmpidToTune = cmpid;
   }
   bool result = m_tunerApi->startDigitalService(ServiceType::AUDIO_SERVICE, sidToTune, cmpidToTune);
   if (result)
   {
      AIO_LOGD(TAG, "startDigitalService %.2x:%.2x on %u success", sidToTune,
                                                                   cmpidToTune,
                                                                   m_csi.frequencyKHz);
      m_csi.sid = sidToTune;
      m_csi.cmpid = cmpidToTune;
      // todo start timer to read signal/audio metrics
   }
   else
   {
      AIO_LOGE(TAG, "startDigitalService ERROR: %.2x:%.2x on %u kHz", sidToTune,
                                                                      cmpidToTune,
                                                                      m_csi.frequencyKHz);
      m_csi.sid = 0;
      m_csi.cmpid = 0;
   }
   return result;
}
std::optional<DABService> DABTunerImpl::getFirstAudioService()
{
   auto it = std::find_if(m_serviceList.services.begin(), m_serviceList.services.end(),
                           [&](auto& item){return item.isAudioService;});
   if(it != m_serviceList.services.end())
   {
      return *it;
   }
   return std::nullopt;
}
std::optional<DABComponent> DABTunerImpl::getPrimaryComponent(uint32_t sid)
{
   auto serviceItem = std::find_if(m_serviceList.services.begin(), m_serviceList.services.end(),
                                  [&](auto& item){return item.id == sid;});
   if(serviceItem == m_serviceList.services.end())
   {
      return std::nullopt;
   }

   auto componentItem = std::find_if(serviceItem->components.begin(), serviceItem->components.end(),
                           [&](auto& item){return item.isPrimary;});
   if(componentItem == serviceItem->components.end())
   {
      return std::nullopt;
   }
   return *componentItem;
}
bool DABTunerImpl::initialize()
{
   AIO_LOGI(TAG, "Initializing DABTuner...");
   m_deviceDriver.setMode(TunerType::DAB);
   m_tunerApi->setProperty(PropertyId::PIN_CONFIG_ENABLE, PIN_CONFIG_ENABLE_DACOUTEN |
                                                          PIN_CONFIG_ENABLE_INTBOUTEN);
   m_tunerApi->setProperty(PropertyId::INT_CTL_ENABLE, INT_CTL_ENABLE_CTSIEN |
                                                       INT_CTL_ENABLE_STCIEN |
                                                       INT_CTL_ENABLE_DEVNTIEN |
                                                       INT_CTL_ENABLE_ERR_CMDIEN);
   m_tunerApi->setProperty(tuner::PropertyId::DAB_TUNE_FE_VARB,0x01E0);
   m_tunerApi->setProperty(tuner::PropertyId::DAB_TUNE_FE_VARM,0xF7A0);
   m_tunerApi->setProperty(PropertyId::DAB_EVENT_INTERRUPT_SOURCE, 0x0001);
   m_deviceDriver.setEventHandler(EventType::DEVNTINT, this);
   return true;
}
void DABTunerImpl::finalize()
{
   m_deviceDriver.removeEventHandler(EventType::DEVNTINT);
   m_tunerApi->setProperty(PropertyId::DAB_EVENT_INTERRUPT_SOURCE, 0x0000);
}
void DABTunerImpl::onEvent(EventType type)
{
   AIO_LOGE(TAG, "Got event %x", static_cast<uint32_t>(type));
   std::lock_guard lock(m_eventMutex);
   if (type == EventType::DEVNTINT && m_task != nullptr)
   {
      xTaskNotifyGive(m_task);
   }
   else
   {
      m_eventQueue.push(type);
      notifyEvent(DABTunerEvent::PROCESSING_REQUIRED);
   }

}
bool DABTunerImpl::readEnsembleConfiguration(EnsembleInfo& ensembleInfo, DigitalServiceList& serviceList)
{
   bool result = true;
   result &= m_tunerApi->getDigitalServiceList(serviceList);
   AIO_LOGE_IF(!result, TAG, "Error getting digital service list");
   AIO_LOGD_IF(result, TAG, "Got digital service list, size %u, version %u", serviceList.services.size(), serviceList.version);
   result &= m_tunerApi->getEnsembleInfo(ensembleInfo);
   AIO_LOGE_IF(!result, TAG, "Error getting ensemble info");
   AIO_LOGD_IF(result, TAG, "Ensemble info - id %.4x, label %s, ecc %.2x, charset %.2x", ensembleInfo.eid,
                                      ensembleInfo.label.c_str(),
                                      ensembleInfo.ecc,
                                      ensembleInfo.charset);
   if (result)
   {
      for(auto& service : serviceList.services)
      {
         for (auto& component : service.components)
         {
            DABComponent info = {};
            result &= m_tunerApi->getComponentInfo(service.id, component.id, info);
            component.language = info.language;
            component.charset = info.charset;
            component.label = info.label;
         }
      }
   }
   return result;
}
bool DABTunerImpl::tuneFrequency(uint32_t frequencyKHz)
{
   AIO_LOGI(TAG, "Processing TUNE request to %u kHz", frequencyKHz);
   return tuneToFrequency(frequencyKHz);
}
bool DABTunerImpl::tuneAndSelect(uint32_t frequencyKHz, uint32_t sid, uint32_t cmpid)
{
   AIO_LOGI(TAG, "Processing TUNE_SELECT request to %u kHz, sid %u, cmpid %u", frequencyKHz, sid, cmpid);
   bool result = tuneToFrequency(frequencyKHz);
   if (result)
   {
      result = startAudioService(sid, cmpid);
   }
   return result;
}
void DABTunerImpl::scan()
{
   m_scanProgress = 0;
   AIO_LOGE(TAG, "Starting band scan");
   notifyEvent(DABTunerEvent::SCAN_STARTED);
   auto frequencyTable = devicedriver::getDABFrequencyTable();
   for (auto it = frequencyTable.begin(); it != frequencyTable.end(); ++it)
   {
      tuneToFrequency(it->first);
      m_scanProgress = static_cast<uint8_t>(std::distance(frequencyTable.begin(), it) * 100 / frequencyTable.size());
      notifyEvent(DABTunerEvent::SCAN_STEP);
   }
   notifyEvent(DABTunerEvent::SCAN_COMPLETED);
   m_scanProgress = 0;
}
int DABTunerImpl::getScanProgress()
{
   return m_scanProgress;
}
void DABTunerImpl::process()
{
   std::lock_guard lock(m_eventMutex);
   while (!m_eventQueue.empty())
   {
      EventType type = m_eventQueue.front();
      m_eventQueue.pop();
      if (type == EventType::DEVNTINT)
      {
         DABEventStatus status;
         m_tunerApi->getDABEventStatus(status);
         AIO_LOGE(TAG, "Event status, srvlistint %u, srvlist %u", status.srvlistInt, status.srvlist);
         if (status.srvlistInt && status.srvlist)
         {
            EnsembleInfo ensembleInfo;
            DigitalServiceList serviceList;
            bool result = readEnsembleConfiguration(ensembleInfo, serviceList);
            if (result)
            {
               m_ensembleInfo = ensembleInfo;
               m_serviceList = serviceList;
            }
            notifyEvent(DABTunerEvent::SERVICE_LIST_CHANGED);
         }
      }
   }
}
void DABTunerImpl::addEventListener(DABTunerEventListener* listener)
{
   std::lock_guard lock(m_eventListenersMutex);
   m_eventListeners.push_back(listener);
}
void DABTunerImpl::removeEventListener(DABTunerEventListener* listener)
{
   std::lock_guard lock(m_eventListenersMutex);
   auto it = std::remove(m_eventListeners.begin(), m_eventListeners.end(), listener);
   m_eventListeners.erase(it, m_eventListeners.end());
}
void DABTunerImpl::notifyEvent(DABTunerEvent event)
{
   std::lock_guard lock(m_eventListenersMutex);
   for (auto& listener : m_eventListeners)
   {
      listener->onEvent(event);
   }
}
CurrentStationInfo DABTunerImpl::getCurrentStationInfo()
{
   CurrentStationInfo result = {};
   result.ensembleInfo.eid = m_ensembleInfo.eid;
   result.ensembleInfo.label = m_ensembleInfo.label;
   result.frequencyKHz = m_csi.frequencyKHz;
   auto serviceIt = std::find_if(m_serviceList.services.begin(), m_serviceList.services.end(),
                                 [&](auto& item){return item.id == m_csi.sid;});
   if (serviceIt != m_serviceList.services.end())
   {
      result.serviceInfo.sid = serviceIt->id;
      result.serviceInfo.label = serviceIt->serviceLabel;
      auto componentIt = std::find_if(serviceIt->components.begin(), serviceIt->components.end(),
                                    [&](auto& item){return item.id == m_csi.cmpid;});
      if (componentIt != serviceIt->components.end())
      {
         result.componentInfo.cmpid = componentIt->id;
         result.componentInfo.label = componentIt->label;
      }
   }
   return result;
}
const DigitalServiceList& DABTunerImpl::getDigitalServiceList()
{
   return m_serviceList;
}

} // namespace tuner
