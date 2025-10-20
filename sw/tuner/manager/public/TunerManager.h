#pragma once
#include <queue>
#include <list>

#include "IMainGUI.h"
#include "GPIOProvider.h"
#include "IFMTuner.h"
#include "IDABTuner.h"
#include "StationList.hpp"
#include "IAudioControl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace tuner::manager
{

struct FMTuneData
{
   uint32_t frequencyKHz;
};

struct DABTuneData
{
   uint32_t frequencyKHz;
   uint32_t serviceID;
   uint32_t componentID;
};

typedef std::string OnlineTuneData;
typedef std::variant<std::monostate, FMTuneData, DABTuneData, OnlineTuneData> AnyTuneData;

struct TuneRequest
{
   ActionType actionType;
   AnyTuneData tuneData;
};


class TunerManager : public dab::DABTunerEventListener,
                     public fm::FMTunerEventListener,
                     public display::gui::SettingsMenuListener
{
public:
   TunerManager(display::gui::IMainGUI&, dab::IDABTuner&, fm::IFMTuner&, audio::IAudioControl&);
   ~TunerManager();

   void task();
private:
   void onEvent(dab::DABTunerEvent event) override;
   void onEvent(fm::FMTunerEvent event) override;

   void onSettingsMenuEvent(display::gui::SettingsMenuEvent event) override;
   void onStationSelected(void* stationToken) override;
   void onScanRequested(display::gui::TunerType type) override;
   void runScan();

   void createTask();
   void destroyTask();
   void processUserRequest(const TuneRequest& request);
   void prepareScanQueue(const AnyTuneData& tuneData);
   void closeScanPopup();
   bool switchBand(TunerType type);
   TunerType getBand() const;
   void finalizeCurrentTuner();

   display::gui::IMainGUI& m_gui;
   dab::IDABTuner& m_dabTuner;
   fm::IFMTuner& m_fmTuner;
   audio::IAudioControl& m_audioControl;
   StationList m_stationList;
   std::list<std::pair<TunerType, uint16_t>> m_scanList;
   std::list<std::pair<TunerType, uint16_t>>::iterator m_scanListIterator;
   std::mutex m_tuneQueueMutex;
   std::queue<TuneRequest> m_tuneQueue;
   TaskHandle_t m_task;
   TunerType m_band;

};

} // namespace display
