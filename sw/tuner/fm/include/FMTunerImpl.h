#pragma once
#include <mutex>
#include <vector>
#include <queue>
#include <memory>

#include "IFMTuner.h"
#include "ITunerApi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

namespace tuner::fm
{

class FMTunerImpl : public IFMTuner,
                    public devicedriver::EventHandler
{
public:
   FMTunerImpl(devicedriver::IDeviceDriver& deviceDriver);
   ~FMTunerImpl() ;
private:
   bool initialize() override;
   void finalize() override;
   bool tuneFrequency(uint32_t frequencyKHz) override;
   void scan() override;
   int getScanProgress() override;
   void process() override;
   void addEventListener(FMTunerEventListener* listener) override;
   void removeEventListener(FMTunerEventListener* listener) override;
   CurrentStationInfo getCurrentStationInfo() override;
   void onEvent(EventType type) override;

   void notifyEvent(FMTunerEvent event);

   devicedriver::IDeviceDriver& m_deviceDriver;
   std::unique_ptr<devicedriver::ITunerApi> m_tunerApi;
   std::mutex m_taskMutex;
   TaskHandle_t m_task;
   uint32_t m_frequencyKHz;
   std::mutex m_eventListenersMutex;
   std::vector<FMTunerEventListener*> m_eventListeners;
   uint8_t m_scanProgress;
   RSQStatus m_rsqStatus;
   RDSStatus m_rdsStatus;

   std::mutex m_eventMutex;
   std::queue<EventType> m_eventQueue;
};

}
