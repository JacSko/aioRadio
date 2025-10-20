#pragma once
#include <mutex>
#include <vector>
#include <queue>

#include "IDABTuner.h"
#include "ITunerApi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

namespace tuner::dab
{

struct CurrentInfo
{
   uint32_t frequencyKHz;
   uint32_t sid;
   uint32_t cmpid;
};

class DABTunerImpl : public IDABTuner,
                     public devicedriver::EventHandler
{
public:
   DABTunerImpl(devicedriver::IDeviceDriver& deviceDriver);
   ~DABTunerImpl() ;
private:
   bool initialize() override;
   void finalize() override;
   bool tuneFrequency(uint32_t frequencyKHz) override;
   bool tuneAndSelect(uint32_t frequencyKHz, uint32_t sid, uint32_t cmpid) override;
   void scan() override;
   int getScanProgress() override;
   void process() override;
   void addEventListener(DABTunerEventListener* listener) override;
   void removeEventListener(DABTunerEventListener* listener) override;
   CurrentStationInfo getCurrentStationInfo() override;
   const DigitalServiceList& getDigitalServiceList() override;
   void onEvent(EventType type) override;

   bool tuneToFrequency(uint32_t frequencyKHz);
   bool startAudioService(uint32_t sid, uint32_t cmpid);
   std::optional<DABService> getFirstAudioService();
   std::optional<DABComponent> getPrimaryComponent(uint32_t sid);
   void notifyEvent(DABTunerEvent event);
   bool readEnsembleConfiguration(EnsembleInfo& ensembleInfo, DigitalServiceList& serviceList);


   devicedriver::IDeviceDriver& m_deviceDriver;
   std::unique_ptr<devicedriver::ITunerApi> m_tunerApi;
   std::mutex m_taskMutex;
   TaskHandle_t m_task;
   CurrentInfo m_csi;
   DigitalServiceList m_serviceList;
   EnsembleInfo m_ensembleInfo;
   std::mutex m_eventListenersMutex;
   std::vector<DABTunerEventListener*> m_eventListeners;
   uint8_t m_scanProgress;

   std::mutex m_eventMutex;
   std::queue<EventType> m_eventQueue;
};

}
