#pragma once
#include <string>
#include <functional>

#include "ISettingsMenu.h"
#include "GUIConfig.h"

namespace display::gui
{


class StationListPage
{
public:
   StationListPage(std::function<void(void*)> stationSelectedCallback):
   m_stationSelectedCallback(stationSelectedCallback)
   {

   }
   lv_obj_t* initialize(lv_obj_t* main_page, lv_style_t* focusedStyle)
   {
      m_page = lv_menu_page_create(main_page, NULL);
      lv_obj_add_flag(m_page, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
      m_focusedStyle = focusedStyle;
      return m_page;
   }
   void onStationSelected(void* token)
   {
      if (m_stationSelectedCallback)
      {
         m_stationSelectedCallback(token);
      }
   }
   void setStationListContent(const StationListContent& content)
   {
      lv_obj_clean(m_page);

      for (const auto& item : content)
      {
         addStation(item.type, item.name, item.frequency, item.token);
      }
   }

private:
   lv_obj_t* addStation(const std::string& type,
                        const std::string& name,
                        const std::string& frequency,
                        void* token)
   {
      lv_obj_t* cont = lv_menu_cont_create(m_page);
      lv_obj_add_style(cont, m_focusedStyle, LV_STATE_FOCUS_KEY);
      lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
      lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
      lv_group_add_obj(lv_group_get_default(), cont);
      lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
      lv_obj_set_user_data(cont, token);
      lv_obj_add_event_cb(cont, [](lv_event_t* e)
            {
               lv_event_code_t code = lv_event_get_code(e);
               StationListPage* context = static_cast<StationListPage*>(lv_event_get_user_data(e));
               lv_obj_t* target = lv_event_get_target_obj(e);
               void* token = lv_obj_get_user_data(target);
               if(code == LV_EVENT_CLICKED)
               {
                  context->onStationSelected(token);
               }
            }, LV_EVENT_CLICKED, this);
      /* Icon */
      lv_obj_t* iconLabel = lv_label_create(cont);
      lv_label_set_text(iconLabel, LV_SYMBOL_AUDIO);
      lv_obj_set_flex_grow(iconLabel, 1);
      lv_obj_set_style_text_align(iconLabel, LV_TEXT_ALIGN_CENTER, 0);

      /* Type */
      lv_obj_t* typeLabel = lv_label_create(cont);
      lv_label_set_text(typeLabel, type.c_str());
      lv_obj_set_flex_grow(typeLabel, 3);
      lv_obj_center(typeLabel);
      lv_obj_set_style_text_align(typeLabel, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_style_text_font(typeLabel, &lv_font_montserrat_12, 0);
      lv_obj_set_style_text_color(typeLabel, lv_color_hex(0x808080), 0);

      /* Name */
      lv_obj_t* nameLabel = lv_label_create(cont);
      lv_label_set_text(nameLabel, name.c_str());
      lv_obj_set_style_text_font(nameLabel, AIO_RADIO_SMALL_FONT, 0);
      lv_obj_set_flex_grow(nameLabel, 8);
      lv_obj_center(nameLabel);

      /* frequency */
      lv_obj_t* frequencyLabel = lv_label_create(cont);
      lv_label_set_text(frequencyLabel, frequency.c_str());
      lv_obj_set_flex_grow(frequencyLabel, 2);
      lv_obj_set_style_text_font(typeLabel, &lv_font_montserrat_12, 0);
      lv_obj_set_style_text_color(typeLabel, lv_color_hex(0x707070), 0);

      return cont;
   }
   std::function<void(void*)> m_stationSelectedCallback;
   lv_obj_t* m_page;
   lv_style_t* m_focusedStyle;
};
}
