#pragma once
#include <cstdint>
#include <string>
#include <list>
#include <variant>


#include "TunerTypes.h"
#include "ISettingsMenu.h"
#include "IDABTuner.h"

namespace tuner::manager
{

enum class ServiceType
{
   AUDIO,
   DATA,
};

struct DABStationListEntry
{
   TunerType tunerType;
   ServiceType serviceType;

   uint32_t frequencyKHz;

   uint32_t ensembleID;
   uint32_t serviceID;
   uint32_t componentID;

   std::string ensembleLabel;
   std::string serviceLabel;
   std::string componentLabel;
   uint32_t token;
};

struct FMStationListEntry
{
   TunerType tunerType;
   uint32_t frequencyKHz;
   std::string psName;
   uint32_t token;
};

typedef std::variant<FMStationListEntry, DABStationListEntry> StationListEntry;

class StationList
{
public:
   StationList():
   m_token(0)
   {
   }
   void addDABStation(uint32_t frequencyKHz, dab::CurrentEnsembleInfo& ensembleInfo, const DigitalServiceList& services)
   {
      std::lock_guard<std::mutex> lock(m_stationListMutex);
      for (const auto& service : services.services)
      {
         for (const auto& component : service.components)
         {
            DABStationListEntry entry;
            entry.tunerType = TunerType::DAB;
            entry.serviceType = service.isAudioService? ServiceType::AUDIO : ServiceType::DATA;
            entry.frequencyKHz = frequencyKHz;
            entry.ensembleID = ensembleInfo.eid;
            entry.serviceID = service.id;
            entry.componentID = component.id;
            entry.ensembleLabel = ensembleInfo.label;
            entry.serviceLabel = service.serviceLabel;
            entry.componentLabel = component.label;
            entry.token = m_token++;
            m_stations.push_back(entry);
         }
      }
   }

   void addFMStation(uint32_t frequencyKHz, const std::string& psName)
   {
      std::lock_guard<std::mutex> lock(m_stationListMutex);
      FMStationListEntry entry;
      entry.tunerType = TunerType::FM;
      entry.frequencyKHz = frequencyKHz;
      entry.psName = psName;
      entry.token = m_token++;
      m_stations.push_back(entry);
   }
   void removeStations(TunerType tunerType)
   {
      std::lock_guard<std::mutex> lock(m_stationListMutex);
      m_stations.remove_if([&](const StationListEntry& entry)
      {
         if (tunerType == TunerType::DAB && std::holds_alternative<DABStationListEntry>(entry))
         {
            return true;
         }
         if (tunerType == TunerType::FM && std::holds_alternative<FMStationListEntry>(entry))
         {
            return true;
         }
         // FMStationListEntry handling can be added here in the future
         return false;
      });
   }

   display::gui::StationListContent toStationListContent()
   {
      display::gui::StationListContent content;
      std::lock_guard<std::mutex> lock(m_stationListMutex);
      for (auto& station : m_stations)
      {
         if (std::holds_alternative<DABStationListEntry>(station))
         {
            auto& dabEntry = std::get<DABStationListEntry>(station);
            if (dabEntry.serviceType == ServiceType::AUDIO)
            {
               display::gui::StationListItem item = {};
               item.type= "DAB";
               item.name = dabEntry.serviceLabel;
               item.frequency = std::to_string(dabEntry.frequencyKHz);
               item.token = &dabEntry.token;
               content.push_back(item);
            }
         }
         if (std::holds_alternative<FMStationListEntry>(station))
         {
            auto& fmEntry = std::get<FMStationListEntry>(station);
            display::gui::StationListItem item = {};
            item.type= "FM";
            item.name = fmEntry.psName;
            item.frequency = std::to_string(fmEntry.frequencyKHz);
            item.token = &fmEntry.token;
            content.push_back(item);
         }
      }
      return content;
   };

   std::optional<StationListEntry> findByToken(uint32_t token)
   {
      std::lock_guard<std::mutex> lock(m_stationListMutex);
      for (auto& station : m_stations)
      {
         if (std::holds_alternative<DABStationListEntry>(station))
         {
            auto& dabEntry = std::get<DABStationListEntry>(station);
            if (dabEntry.token == token)
            {
               return station;
            }
         }
         if (std::holds_alternative<FMStationListEntry>(station))
         {
            auto& fmEntry = std::get<FMStationListEntry>(station);
            if (fmEntry.token == token)
            {
               return station;
            }
         }
      }
      return std::nullopt;
   }

private:
   std::mutex m_stationListMutex;
   uint32_t m_token;
   std::list<StationListEntry> m_stations;
};

} // namespace tuner::manager
