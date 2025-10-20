#pragma once
#include <mutex>
#include <array>
#include <lvgl.h>
#include "esp_lvgl_port.h"

#include "ISettingsMenu.h"
#include "GUIConfig.h"
#include "pages/StationListPage.h"
#include "pages/StationScanPage.h"

namespace display::gui
{

class SettingsMenu : public ISettingsMenu,
                     public StationListPage,
                     public StationScanPage
{
public:
   SettingsMenu():
   StationListPage([this](void* token){this->showMenu(false);
                                       this->notifyStationSelectedEvent(token);}),
   StationScanPage([this](const TunerType& type){this->showMenu(false);
                                       this->notifyScanRequestedEvent(type);})
   {
   }
   void initialize() override
   {
      lvgl_port_lock(0);

      initializeStyles();
      createMenuWidget();
      createMainPage();
      setupBackButton();

      addStationListEntry();
      addStationScanEntry();

      lv_menu_set_page(m_menu, m_mainPage);
      lv_group_focus_obj(lv_menu_get_main_header_back_button(m_menu));
      lvgl_port_unlock();
   }
   void showMenu(bool show) override
   {
      lvgl_port_lock(0);
      if (show)
      {
         lv_menu_set_page(m_menu, m_mainPage);
         lv_group_focus_obj(lv_menu_get_main_header_back_button(m_menu));
         lv_obj_clear_flag(m_menu, LV_OBJ_FLAG_HIDDEN);
         notifyMenuEvent(SettingsMenuEvent::MENU_OPENED);
      }
      else
      {
         lv_obj_add_flag(m_menu, LV_OBJ_FLAG_HIDDEN);
         lv_menu_set_page(m_menu, NULL);
         notifyMenuEvent(SettingsMenuEvent::MENU_CLOSED);
      }
      lvgl_port_unlock();
   }
   bool isMenuVisible() override
   {
      lvgl_port_lock(0);
      bool result = lv_obj_has_flag(m_menu, LV_OBJ_FLAG_HIDDEN);
      lvgl_port_unlock();
      return !result;
   }
   void setStationList(const StationListContent& content) override
   {
      lvgl_port_lock(0);
      this->setStationListContent(content);
      lvgl_port_unlock();
   }
   void addListener(SettingsMenuListener* listener) override
   {
      std::lock_guard<std::mutex> lock(m_listenersMutex);
      auto it = std::find_if(m_listeners.begin(), m_listeners.end(), [&](SettingsMenuListener* l){return l == nullptr || l == listener;});
      if (it == m_listeners.end())
      {
         //TODO add error log
         return;
      }
      if (*it == listener)
      {
         //TODO add error log
         return;
      }
      *it = listener;
      return;
   }
   void removeListener(SettingsMenuListener* listener) override
   {
      std::lock_guard<std::mutex> lock(m_listenersMutex);
      auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
      if (it == m_listeners.end())
      {
         //TODO add error log
         return;
      }
      *it = nullptr;
      return;
   }
   bool isRootBackButton(lv_obj_t* obj)
   {
       return lv_menu_back_button_is_root(m_menu, obj);
   }
private:
   void notifyMenuEvent(SettingsMenuEvent event)
   {
      std::mutex m_listenersMutex;
      for (auto& listener : m_listeners)
      {
          if (listener)
          {
              listener->onSettingsMenuEvent(event);
          }
      }
   }
   void notifyStationSelectedEvent(void* stationToken)
   {
      std::mutex m_listenersMutex;
      for (auto& listener : m_listeners)
      {
          if (listener)
          {
              listener->onStationSelected(stationToken);
          }
      }
   }
   void notifyScanRequestedEvent(TunerType type)
   {
      std::mutex m_listenersMutex;
      for (auto& listener : m_listeners)
      {
          if (listener)
          {
              listener->onScanRequested(type);
          }
      }
   }
   void initializeStyles()
   {
      lv_style_init(&m_menuStyle);
      lv_style_set_bg_color(&m_menuStyle, lv_color_hex(SETTINGS_MENU_BACKGROUND_COLOR));
      lv_style_set_text_color(&m_menuStyle, lv_color_hex(SETTINGS_MENU_TEXT_COLOR));
      lv_style_set_outline_color(&m_menuStyle, lv_color_hex(SETTINGS_MENU_OUTLINE_COLOR));
      lv_style_set_outline_width(&m_menuStyle, 1);

      lv_style_init(&m_pageStyle);
      lv_style_set_bg_color(&m_pageStyle, lv_color_hex(SETTINGS_MENU_BACKGROUND_COLOR));
      lv_style_set_text_color(&m_pageStyle, lv_color_hex(SETTINGS_MENU_TEXT_COLOR));
      lv_style_set_outline_width(&m_pageStyle, 0);

      lv_style_init(&m_focusedStyle);
      lv_style_set_bg_color(&m_focusedStyle, lv_color_hex(SETTINGS_MENU_FOCUSED_BACKGROUND_COLOR));
      lv_style_set_text_color(&m_focusedStyle, lv_color_hex(SETTINGS_MENU_FOCUSED_TEXT_COLOR));
      lv_style_set_bg_opa(&m_focusedStyle, SETTINGS_MENU_FOCUSED_BG_OPA);
      lv_style_set_border_opa(&m_focusedStyle, SETTINGS_MENU_FOCUSED_BORDER_OPA);
      lv_style_set_radius(&m_focusedStyle, lv_dpx(SETTINGS_MENU_FOCUSED_RADIUS));
      lv_style_set_border_color(&m_focusedStyle, lv_color_hex(SETTINGS_MENU_FOCUSED_BORDER_COLOR));
      lv_style_set_border_width(&m_focusedStyle, SETTINGS_MENU_FOCUSED_BORDER_WIDTH);
      lv_style_set_border_side(&m_focusedStyle, LV_BORDER_SIDE_FULL);
      lv_style_set_outline_width(&m_focusedStyle, 0);
   }
   void createMenuWidget()
   {
      m_menu = lv_menu_create(lv_scr_act());
      lv_obj_add_flag(m_menu, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_size(m_menu, SETTINGS_MENU_WIDTH, SETTINGS_MENU_HEIGHT);
      lv_obj_center(m_menu);
      lv_obj_add_style(m_menu, &m_menuStyle, 0);
   }
   void createMainPage()
   {
      m_mainPage = lv_menu_page_create(m_menu, NULL);
      lv_obj_add_style(m_mainPage, &m_pageStyle, 0);
      lv_group_add_obj(lv_group_get_default(), lv_menu_get_main_header_back_button(m_menu));
   }
   void setupBackButton()
   {
      lv_obj_t* back_btn = lv_menu_get_main_header_back_button(m_menu);
      lv_obj_t* header = lv_menu_get_main_header(m_menu);
      lv_obj_add_style(back_btn, &m_pageStyle, 0);
      lv_obj_add_style(back_btn, &m_focusedStyle, LV_STATE_FOCUS_KEY);
      lv_obj_t* label = lv_label_create(back_btn);
      lv_label_set_text(label, " EXIT");
      lv_menu_set_mode_root_back_button(m_menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
      lv_obj_add_event_cb(back_btn, [](lv_event_t* e){
         lv_event_code_t code = lv_event_get_code(e);
         lv_obj_t* button = lv_event_get_target_obj(e);
         if(code == LV_EVENT_CLICKED)
         {
            SettingsMenu* menu = static_cast<SettingsMenu*>(lv_event_get_user_data(e));
            if (menu->isMenuVisible() && menu->isRootBackButton(button))
            {
               menu->showMenu(false);
            }
         }
      }, LV_EVENT_CLICKED, this);
   }
   void addStationListEntry()
   {
      lv_obj_t* page = StationListPage::initialize(m_menu, &m_focusedStyle);
      lv_obj_t* cont = addEntry("Station List");
      lv_menu_set_load_page_event(m_menu, cont, page);
   }
   void addStationScanEntry()
   {
      lv_obj_t* page = StationScanPage::initialize(m_menu, &m_focusedStyle);
      lv_obj_t* cont = addEntry("Scan");
      lv_menu_set_load_page_event(m_menu, cont, page);
   }
   lv_obj_t* addEntry(const std::string& name)
   {
      lv_obj_t* cont = lv_menu_cont_create(m_mainPage);
      lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
      lv_obj_t* label = lv_label_create(cont);
      lv_label_set_text(label, name.c_str());
      lv_group_add_obj(lv_group_get_default(), cont);
      lv_obj_add_style(cont, &m_focusedStyle, LV_STATE_FOCUS_KEY);
      return cont;
   }

   lv_obj_t* m_menu;
   lv_style_t m_menuStyle;
   lv_style_t m_pageStyle;
   lv_style_t m_focusedStyle;
   lv_obj_t * m_mainPage;
   std::mutex m_listenersMutex;
   std::array<SettingsMenuListener*, 5> m_listeners = {nullptr};
};

}
