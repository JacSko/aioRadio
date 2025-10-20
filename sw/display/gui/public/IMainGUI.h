#pragma once
#include <memory>

#include "IRadioViews.h"
#include "ISettingsMenu.h"
#include "IScanPopup.h"
#include "IVolumePopup.h"

namespace display::gui
{
class IMainGUI
{
public:
    virtual ~IMainGUI() = default;

    static std::unique_ptr<IMainGUI> create();

    virtual void initialize(ViewType) = 0;
    virtual void finalize() = 0;
    virtual void showMainView(ViewType) = 0;
    virtual void setAudioStatus(bool) = 0;
    virtual void setWifiConnectionStatus(bool) = 0;
    virtual IFMView& getFMView() = 0;
    virtual IDABView& getDABView() = 0;
    virtual ISettingsMenu& getSettingsMenu() = 0;
    virtual IScanPopup& getScanPopup() = 0;
    virtual IVolumePopup& getVolumePopup() = 0;

    virtual void controlMoved(int16_t delta) = 0;
    virtual void controlButtonPressed() = 0;
    virtual void controlButtonReleased() = 0;
};

}
