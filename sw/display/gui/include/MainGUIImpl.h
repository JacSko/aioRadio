#pragma once
#include <memory>
#include "lvgl.h"

#include "IMainGUI.h"
#include "views/FMViewImpl.hpp"
#include "views/DABViewImpl.hpp"
#include "SettingsMenu.h"
#include "popups/ScanPopup.hpp"
#include "popups/VolumePopup.hpp"

namespace display::gui
{

class MainGUIImpl : public IMainGUI
{
public:
    MainGUIImpl();
    ~MainGUIImpl();

    MainGUIImpl(const MainGUIImpl&) = delete;
    MainGUIImpl(MainGUIImpl&&) = delete;
    MainGUIImpl operator=(const MainGUIImpl&) = delete;
    MainGUIImpl operator=(const MainGUIImpl&&) = delete;

private:

    struct TopRibbon
    {
       lv_style_t style;
       lv_obj_t* ribbon;
       lv_obj_t* fmLabel;
       lv_obj_t* dabLabel;
       lv_obj_t* onlineLabel;
       lv_obj_t* audioIcon;
       lv_obj_t* wifiIcon;
    };

   void initialize(ViewType) override;
   void finalize() override;
   void showMainView(ViewType) override;
   void setAudioStatus(bool) override;
   void setWifiConnectionStatus(bool) override;

   IFMView& getFMView() override;
   IDABView& getDABView() override;
   ISettingsMenu& getSettingsMenu() override;
   IScanPopup& getScanPopup() override;
   IVolumePopup& getVolumePopup() override;
   void controlMoved(int16_t delta) override;
   void controlButtonPressed() override;
   void controlButtonReleased() override;

   void initializeStyles();
   void renderTopRibbon();
   void renderMainWindow();
   void renderBottomRibbon();
   void markAllBandsInactive();
   void markBandActive(lv_obj_t* label, bool active);
   void setupSettingsMenu();

   lv_obj_t* m_screen;
   lv_style_t m_mainWindowStyle;
   lv_style_t m_bottomRibbonStyle;
   TopRibbon m_topRibbon;
   lv_obj_t* m_mainWindow;
   lv_obj_t* m_bottomRibbon;
   FMViewImpl m_fmView;
   DABViewImpl m_dabView;
   SettingsMenu m_settingsMenu;
   popups::ScanPopup m_scanPopup;
   popups::VolumePopup m_volumePopup;
};
}
