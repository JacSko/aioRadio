#pragma once
#include <string>
#include <vector>

#include "GUITypes.h"

namespace display::gui
{

enum class SettingsMenuEvent
{
   MENU_OPENED,
   MENU_CLOSED,
};

struct StationListItem
{
   std::string type;
   std::string name;
   std::string frequency;
   void* token;
};
typedef std::vector<StationListItem> StationListContent;

struct SettingsMenuListener
{
    virtual ~SettingsMenuListener() = default;
    virtual void onSettingsMenuEvent(SettingsMenuEvent event) = 0;
    virtual void onStationSelected(void* stationToken) = 0;
    virtual void onScanRequested(TunerType type) = 0;
};

class ISettingsMenu
{
public:
    virtual ~ISettingsMenu() = default;

    virtual void initialize() = 0;
    virtual void showMenu(bool show) = 0;
    virtual bool isMenuVisible() = 0;
    virtual void setStationList(const StationListContent&) = 0;
    virtual void addListener(SettingsMenuListener* listener) = 0;
    virtual void removeListener(SettingsMenuListener* listener) = 0;
};

}

