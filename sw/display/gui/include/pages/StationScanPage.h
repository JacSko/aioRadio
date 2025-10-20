#pragma once
#include <string>
#include <vector>
#include <functional>
#include "lvgl.h"

#include "ISettingsMenu.h"

namespace display::gui
{


class StationScanPage
{
public:
   StationScanPage(std::function<void(const TunerType&)> stationScanRequestCallback):
   m_stationScanRequestCallback(stationScanRequestCallback),
   m_scanTypes({TunerType::ALL,
                TunerType::DAB,
                TunerType::FM,
                TunerType::ONLINE})
   {

   }
   lv_obj_t* initialize(lv_obj_t* main_page, lv_style_t* focusedStyle)
   {
      m_page = lv_menu_page_create(main_page, NULL);
      lv_obj_add_flag(m_page, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
      m_focusedStyle = focusedStyle;
      for (auto it = m_scanTypes.begin(); it < m_scanTypes.end(); it++)
      {
         addItem(*it);
      }
      return m_page;
   }
   void onStationScanRequested(TunerType type)
   {
      if (m_stationScanRequestCallback)
      {
         m_stationScanRequestCallback(type);
      }
   }

private:
   lv_obj_t* addItem(TunerType& type)
   {
      lv_obj_t* cont = lv_menu_cont_create(m_page);
      lv_obj_add_style(cont, m_focusedStyle, LV_STATE_FOCUS_KEY);
      lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
      lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
      lv_group_add_obj(lv_group_get_default(), cont);
      lv_obj_set_user_data(cont, static_cast<void*>(&type));
      lv_obj_add_event_cb(cont, [](lv_event_t* e)
            {
               lv_event_code_t code = lv_event_get_code(e);
               StationScanPage* context = static_cast<StationScanPage*>(lv_event_get_user_data(e));
               lv_obj_t* target = lv_event_get_target_obj(e);
               TunerType* scanType = static_cast<TunerType*>(lv_obj_get_user_data(target));
               if(code == LV_EVENT_CLICKED)
               {
                  context->onStationScanRequested(*scanType);
               }
            }, LV_EVENT_CLICKED, this);

      lv_obj_t* nameLabel = lv_label_create(cont);
      lv_label_set_text(nameLabel, tunerTypeToName(type));
      lv_obj_set_flex_grow(nameLabel, 8);
      lv_obj_center(nameLabel);
      return cont;
   }
   std::function<void(const TunerType&)> m_stationScanRequestCallback;
   std::vector<TunerType> m_scanTypes;
   lv_obj_t* m_page;
   lv_style_t* m_focusedStyle;
};

}

