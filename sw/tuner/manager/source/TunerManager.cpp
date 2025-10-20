#include "TunerManager.h"
#include "Log.h"
#include "TableConverter.hpp"
#include "GUITypes.h"

namespace tuner::manager
{

const char* TAG = "TM";
static const int TASK_STACK_SIZE = 4096;
static const int TASK_PRIORITY = 2;

static void taskFunction(void* arg)
{
   TunerManager* tm = static_cast<TunerManager*>(arg);
   tm->task();
}

TunerManager::TunerManager(display::gui::IMainGUI& gui, dab::IDABTuner& dabTuner, fm::IFMTuner& fmTuner, audio::IAudioControl& audioControl):
m_gui(gui),
m_dabTuner(dabTuner),
m_fmTuner(fmTuner),
m_audioControl(audioControl)
{
   AIO_LOGD(TAG, "Starting TunerManager...");
   m_dabTuner.addEventListener(this);
   m_fmTuner.addEventListener(this);
   m_gui.getSettingsMenu().addListener(this);
   AIO_LOGE_IF(!switchBand(TunerType::FM), TAG, "Failed to initialize FM Tuner");
   AIO_LOGD(TAG, "TunerManager started");
   createTask();
}
TunerManager::~TunerManager()
{
   destroyTask();
   m_gui.getSettingsMenu().removeListener(this);
   m_fmTuner.removeEventListener(this);
   m_dabTuner.removeEventListener(this);
}
void TunerManager::destroyTask()
{
   AIO_LOGD(TAG, "Destroying TunerManager task");
   if (m_task != nullptr)
   {
      vTaskDelete(m_task);
      m_task = nullptr;
   }
}
void TunerManager::createTask()
{
   AIO_LOGD(TAG, "Creating TunerManager task");
   BaseType_t result = xTaskCreate(taskFunction, TAG, TASK_STACK_SIZE, this, TASK_PRIORITY, &m_task);
   AIO_LOGE_IF(result != pdPASS, TAG, "Failed to create TunerManager task, error %d", result);
}
bool TunerManager::switchBand(TunerType type)
{
   bool result = false;
   if (m_band == type)
   {
      AIO_LOGI(TAG, "Tuner band is already set to %d", static_cast<int>(type));
      return true;
   }

   finalizeCurrentTuner();

   switch(type)
   {
      case TunerType::DAB:
         AIO_LOGI(TAG, "Setting tuner band to DAB");
         m_dabTuner.initialize();
         m_audioControl.switchSource(audio::AudioSource::DAB_RADIO);
         m_band = TunerType::DAB;
         result = true;
         break;
      case TunerType::FM:
         AIO_LOGI(TAG, "Setting tuner band to FM");
         m_fmTuner.initialize();
         m_audioControl.switchSource(audio::AudioSource::FM_RADIO);
         m_band = TunerType::FM;
         result = true;
         break;
      case TunerType::ONLINE:
      default:
         AIO_LOGE(TAG, "Invalid tuner type for setBand: %d", static_cast<int>(type));
   }
   return result;
}
void TunerManager::finalizeCurrentTuner()
{
   switch(m_band)
   {
      case TunerType::DAB:
         AIO_LOGI(TAG, "Finalizing DAB tuner");
         m_dabTuner.finalize();
         break;
      case TunerType::FM:
         AIO_LOGI(TAG, "Finalizing FM tuner");
         m_fmTuner.finalize();
         break;
      case TunerType::ONLINE:
      default:
         AIO_LOGI(TAG, "No tuner to finalize for band: %d", static_cast<int>(m_band));
   }
}
TunerType TunerManager::getBand() const
{
   return m_band;
}
void TunerManager::task()
{
   AIO_LOGI(TAG, "Starting task");
   std::optional<TuneRequest> request = std::nullopt;
   while (true)
   {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      {
         std::lock_guard<std::mutex> lock(m_tuneQueueMutex);
         if (!m_tuneQueue.empty())
         {
            request = std::move(m_tuneQueue.front());
            m_tuneQueue.pop();
         }
      }

      if (request.has_value())
      {
         processUserRequest(request.value());
         request = std::nullopt;
      }
      else
      {
         switch(m_band)
         {
            case TunerType::DAB:
               // m_dabTuner.process();
               break;
            case TunerType::FM:
               m_fmTuner.process();
               break;
            case TunerType::ONLINE:
            default:
               AIO_LOGE(TAG, "No processing implemented for tuner band: %d", static_cast<int>(m_band));
         }
      }
   }
}
void TunerManager::processUserRequest(const TuneRequest& request)
{
   switch(request.actionType)
   {
      case ActionType::SCAN:
      {
         AIO_LOGI(TAG, "Processing SCAN request");
         prepareScanQueue(request.tuneData);
         m_gui.getScanPopup().show();
         runScan();
         m_gui.getSettingsMenu().setStationList(m_stationList.toStationListContent());
         closeScanPopup();
         break;
      }
      case ActionType::TUNE_SELECT:
      {
         AIO_LOGI(TAG, "Processing TUNE_SELECT request");
         if (std::holds_alternative<DABTuneData>(request.tuneData))
         {
            DABTuneData data = std::get<DABTuneData>(request.tuneData);
            if (getBand() != TunerType::DAB)
            {
               AIO_LOGE_IF(!switchBand(TunerType::DAB), TAG, "Failed to switch to DAB band for tune select" );
            }
            m_dabTuner.tuneAndSelect(data.frequencyKHz, data.serviceID, data.componentID);

            dab::CurrentStationInfo stationInfo = m_dabTuner.getCurrentStationInfo();
            m_gui.showMainView(display::gui::ViewType::DAB);
            m_gui.getDABView().setServiceName(stationInfo.serviceInfo.label);
            m_gui.getDABView().setEnsembleName(stationInfo.ensembleInfo.label);
            m_gui.getDABView().setServiceID(stationInfo.serviceInfo.sid);
            m_gui.getDABView().setEnsembleID(stationInfo.ensembleInfo.eid);
            m_gui.getDABView().setFrequency(stationInfo.frequencyKHz);
         }
         else if (std::holds_alternative<FMTuneData>(request.tuneData))
         {
            FMTuneData data = std::get<FMTuneData>(request.tuneData);
            if (getBand() != TunerType::FM)
            {
               AIO_LOGE_IF(!switchBand(TunerType::FM), TAG, "Failed to switch to FM band for tune select" );
            }
            m_fmTuner.tuneFrequency(data.frequencyKHz);

            fm::CurrentStationInfo stationInfo = m_fmTuner.getCurrentStationInfo();
            m_gui.showMainView(display::gui::ViewType::FM);
            m_gui.getFMView().setFrequency(stationInfo.frequencyKHz);
            m_gui.getFMView().setStationName(std::to_string(stationInfo.frequencyKHz) + " kHz");
            m_gui.getFMView().setPI(stationInfo.piCode);
         }
         else
         {
            AIO_LOGE(TAG, "TUNE_SELECT request has invalid tune data");
         }
         break;
      }
      default:
         AIO_LOGE(TAG, "Unknown TuneRequest action type: %d", static_cast<int>(request.actionType));
   }
}
void TunerManager::prepareScanQueue(const AnyTuneData& tuneData)
{
   m_scanList = {};
   if (std::holds_alternative<FMTuneData>(tuneData))
   {
      m_scanList.push_back({TunerType::FM, 0});
   }
   else if (std::holds_alternative<DABTuneData>(tuneData))
   {
      m_scanList.push_back({TunerType::DAB, 0});
   }
   else if (std::holds_alternative<OnlineTuneData>(tuneData))
   {
      m_scanList.push_back({TunerType::ONLINE, 0});
   }
   else
   {
      m_scanList.push_back({TunerType::FM, 0});
      m_scanList.push_back({TunerType::DAB, 0});
      m_scanList.push_back({TunerType::ONLINE, 0});
   }
}
void TunerManager::closeScanPopup()
{
   static std::vector<std::pair<TunerType, display::gui::TunerType>> tunerTypeTable = {
      {TunerType::FM, display::gui::TunerType::FM},
      {TunerType::DAB, display::gui::TunerType::DAB},
      {TunerType::ONLINE, display::gui::TunerType::ONLINE}
   };

   display::gui::ScanResults results = {};
   for (const auto& scanEntry : m_scanList)
   {
      auto guiTunerTypeOpt = utilities::TableConverter::convert(tunerTypeTable, scanEntry.first);
      if (guiTunerTypeOpt.has_value())
      {
         AIO_LOGI(TAG, "Found %u station(s) for %s", scanEntry.second, display::gui::tunerTypeToName(guiTunerTypeOpt.value()));
         results[guiTunerTypeOpt.value()] = scanEntry.second;
      }
   }
   m_gui.getScanPopup().showSummaryAndClose(results);
}
void TunerManager::onEvent(dab::DABTunerEvent event)
{
   AIO_LOGD(TAG, "Received DABTuner event: %s", dab::IDABTuner::getEventName(event));
   switch(event)
   {
      case dab::DABTunerEvent::SCAN_STARTED:
      {
         m_gui.getScanPopup().setScanType(display::gui::TunerType::DAB);
         m_gui.getScanPopup().setStationsFound(0);
         m_gui.getScanPopup().notifyProgress(0);
         break;
      }
      case dab::DABTunerEvent::SCAN_STEP:
      {
         dab::CurrentStationInfo csi = m_dabTuner.getCurrentStationInfo();
         const DigitalServiceList& services = m_dabTuner.getDigitalServiceList();
         if (csi.ensembleInfo.eid != 0 && services.services.size() > 0)
         {
            AIO_LOGI(TAG, "DAB Ensemble found: freq %u kHz, eid %.4x, label %s, services found: %u",
                     csi.frequencyKHz,
                     csi.ensembleInfo.eid,
                     csi.ensembleInfo.label.c_str(),
                     static_cast<uint32_t>(services.services.size()));
            m_stationList.addDABStation(csi.frequencyKHz, csi.ensembleInfo, services);
            m_scanListIterator->second += static_cast<uint16_t>(services.services.size());
            m_gui.getScanPopup().setStationsFound(m_scanListIterator->second);
         }
         m_gui.getScanPopup().notifyProgress(m_dabTuner.getScanProgress());
         break;
      }

      default:
         AIO_LOGE(TAG, "No handling implemented for event: %s", dab::IDABTuner::getEventName(event));
   }
}
void TunerManager::onEvent(fm::FMTunerEvent event)
{
   AIO_LOGD(TAG, "Received FMTuner event: %s", fm::IFMTuner::getEventName(event));
   switch(event)
   {
      case fm::FMTunerEvent::SCAN_STARTED:
      {
         m_gui.getScanPopup().setScanType(display::gui::TunerType::FM);
         m_gui.getScanPopup().setStationsFound(0);
         m_gui.getScanPopup().notifyProgress(0);
         break;
      }
      case fm::FMTunerEvent::SCAN_STEP:
      {
         fm::CurrentStationInfo csi = m_fmTuner.getCurrentStationInfo();
         if (csi.frequencyKHz != 0 && csi.valid)
         {
            AIO_LOGI(TAG, "FM station found: freq %u kHz, valid %u, snr %u, rssi %d",
                     csi.frequencyKHz,
                     csi.valid,
                     csi.snr,
                     csi.rssi);

            m_stationList.addFMStation(csi.frequencyKHz, std::to_string(csi.frequencyKHz) + " kHz");
            m_scanListIterator->second++;
            m_gui.getScanPopup().setStationsFound(m_scanListIterator->second);
         }
         m_gui.getScanPopup().notifyProgress(m_fmTuner.getScanProgress());
         break;
      }
      case fm::FMTunerEvent::PROCESSING_REQUIRED:
      {
         xTaskNotifyGive(m_task);
         break;
      }
      default:
         AIO_LOGE(TAG, "No handling implemented for event: %s", fm::IFMTuner::getEventName(event));
   }
}
void TunerManager::onSettingsMenuEvent(display::gui::SettingsMenuEvent event)
{
}
void TunerManager::onStationSelected(void* stationToken)
{
   AIO_LOGE_IF(!stationToken, TAG, "Station token is null, cannot select!");
   if (stationToken)
   {
      uint32_t token = *(static_cast<uint32_t*>(stationToken));
      std::optional<StationListEntry> entryOpt = m_stationList.findByToken(token);
      AIO_LOGE_IF(!entryOpt.has_value(), TAG, "No station found for token: %d", token);
      if (entryOpt.has_value())
      {
         StationListEntry entry = entryOpt.value();
         if (std::holds_alternative<DABStationListEntry>(entry))
         {
            std::lock_guard<std::mutex> lock(m_tuneQueueMutex);

            DABStationListEntry dabEntry = std::get<DABStationListEntry>(entry);
            TuneRequest request = {};
            request.actionType = ActionType::TUNE_SELECT;
            request.tuneData = DABTuneData{
                                 .frequencyKHz = dabEntry.frequencyKHz,
                                 .serviceID =  dabEntry.serviceID,
                                 .componentID = dabEntry.componentID
                               };
            m_tuneQueue.push(std::move(request));
            xTaskNotifyGive(m_task);
         }
         else if (std::holds_alternative<FMStationListEntry>(entry))
         {
            std::lock_guard<std::mutex> lock(m_tuneQueueMutex);

            FMStationListEntry fmEntry = std::get<FMStationListEntry>(entry);
            TuneRequest request = {};
            request.actionType = ActionType::TUNE_SELECT;
            request.tuneData = FMTuneData{
                                 .frequencyKHz = fmEntry.frequencyKHz,
                               };
            m_tuneQueue.push(std::move(request));
            xTaskNotifyGive(m_task);
         }
         else
         {
            AIO_LOGE(TAG, "Selected station is not a DAB station, cannot select!");
         }
      }
   }
}
void TunerManager::onScanRequested(display::gui::TunerType type)
{
   AIO_LOGI(TAG, "Scan requested for tuner type: %s", display::gui::tunerTypeToName(type));
   std::lock_guard<std::mutex> lock(m_tuneQueueMutex);
   TuneRequest request = {};
   request.actionType = ActionType::SCAN;
   request.tuneData = std::monostate{};

   switch(type)
   {
      case display::gui::TunerType::FM:
         request.tuneData = FMTuneData{};
         break;
      case display::gui::TunerType::DAB:
         request.tuneData = DABTuneData{};
         break;
      case display::gui::TunerType::ONLINE:
         request.tuneData = OnlineTuneData{};
         break;
      case display::gui::TunerType::ALL:
      default:
         // empty variant to scan all tuners
         request.tuneData = std::monostate{};
         break;
   }
   m_tuneQueue.push(std::move(request));
   xTaskNotifyGive(m_task);
}

void TunerManager::runScan()
{
   m_scanListIterator = m_scanList.begin();
   while(m_scanListIterator != m_scanList.end())
   {
      switch(m_scanListIterator->first)
      {
         case TunerType::DAB:
            m_stationList.removeStations(TunerType::DAB);
            if (getBand() != TunerType::DAB)
            {
               AIO_LOGE_IF(!switchBand(TunerType::DAB), TAG, "Failed to switch to DAB band for scan" );
            }
            m_dabTuner.scan();
            break;
         case TunerType::FM:
            m_stationList.removeStations(TunerType::FM);
            if (getBand() != TunerType::FM)
            {
               AIO_LOGE_IF(!switchBand(TunerType::FM), TAG, "Failed to switch to FM band for scan" );
            }
            m_fmTuner.scan();
            break;
         case TunerType::ONLINE:
            // TODO implement ONLINE scan
            AIO_LOGE(TAG, "ONLINE scan not implemented yet!");
            break;
         default:
            AIO_LOGE(TAG, "Unknown tuner type in scan list: %d", static_cast<int>(m_scanListIterator->first));
      }
      ++m_scanListIterator;
   }
}

} // namespace display
