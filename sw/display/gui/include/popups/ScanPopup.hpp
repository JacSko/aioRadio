#pragma once
#include <string>
#include <cstdint>

#include "lvgl.h"
#include "esp_lvgl_port.h"

#include "IScanPopup.h"
#include "GUIConfig.h"

namespace display::gui::popups
{

static inline void scanPopupTimer(lv_timer_t * timer)
{
   IScanPopup* scanPopup = static_cast<IScanPopup*>(lv_timer_get_user_data(timer));;
   scanPopup->close();
   lv_timer_delete(timer);
}

class ScanPopup : public IScanPopup
{
public:
   ScanPopup()
   {
   }
   ~ScanPopup()
   {
   }

   void initialize() override
   {
      lv_style_init(&m_style);
      lv_style_set_bg_color(&m_style, lv_color_hex(DISPLAY_BACKGROUND_COLOR));
      lv_style_set_border_width(&m_style, 0);
      lv_style_set_outline_width(&m_style, 0);
      lv_style_set_pad_all(&m_style, 0);
      lv_style_set_outline_pad(&m_style, 0);
      lv_style_set_border_side(&m_style, LV_BORDER_SIDE_NONE);
      lv_style_set_text_color(&m_style, lv_color_hex(SCAN_VOL_DEFAULT_TEXT_COLOR));
   }

   void show() override
   {
      lvgl_port_lock(0);
      m_popup = lv_msgbox_create(nullptr);
      lv_obj_set_size(m_popup, SCAN_VOL_POPUP_WIDTH, SCAN_VOL_POPUP_HEIGHT);
      lv_obj_align(m_popup, LV_ALIGN_CENTER, 0, 0);
      lv_obj_add_style(m_popup, &m_style, LV_STATE_DEFAULT);

      lv_obj_t* content = lv_msgbox_get_content(m_popup);
      lv_obj_clean(content);
      lv_obj_add_style(content, &m_style, LV_STATE_DEFAULT);
      lv_obj_align(content, LV_ALIGN_CENTER, 0, 0);
      lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

      lv_obj_t* titleObject = lv_obj_create(content);
      lv_obj_add_style(titleObject, &m_style, LV_STATE_DEFAULT);
      lv_obj_set_size(titleObject, SCAN_VOL_POPUP_WIDTH, SCAN_VOL_POPUP_TITLE_HEIGHT);
      lv_obj_remove_flag(titleObject, LV_OBJ_FLAG_SCROLLABLE);

      lv_obj_t* progressBarObject = lv_obj_create(content);
      lv_obj_add_style(progressBarObject, &m_style, LV_STATE_DEFAULT);
      lv_obj_set_size(progressBarObject, SCAN_VOL_POPUP_WIDTH, SCAN_VOL_POPUP_PROGRESS_BAR_HEIGHT);
      lv_obj_remove_flag(progressBarObject, LV_OBJ_FLAG_SCROLLABLE);

      lv_obj_t* stationsFoundObject = lv_obj_create(content);
      lv_obj_add_style(stationsFoundObject, &m_style, LV_STATE_DEFAULT);
      lv_obj_set_size(stationsFoundObject, SCAN_VOL_POPUP_WIDTH, SCAN_VOL_POPUP_SUMMARY_HEIGHT);
      lv_obj_remove_flag(stationsFoundObject, LV_OBJ_FLAG_SCROLLABLE);

      m_scanLabel = lv_label_create(titleObject);
      lv_obj_set_style_text_align(m_scanLabel, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_align(m_scanLabel, LV_ALIGN_CENTER, 0, 0);
      lv_obj_set_style_text_font(m_scanLabel, AIO_RADIO_BIG_FONT, 0);

      m_bar = lv_bar_create(progressBarObject);
      lv_obj_center(m_bar);
      lv_obj_set_size(m_bar, SCAN_VOL_POPUP_PROGRESS_BAR_WIDTH, SCAN_VOL_POPUP_PROGRESS_BAR_HEIGHT);

      m_scanCountLabel = lv_label_create(stationsFoundObject);
      lv_obj_set_style_text_align(m_scanCountLabel, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_align(m_scanCountLabel, LV_ALIGN_CENTER, 0, 0);
      lv_obj_set_style_text_font(m_scanCountLabel, AIO_RADIO_SMALL_FONT, 0);
      lvgl_port_unlock();
   }
   void setScanType(TunerType type) override
   {
      lvgl_port_lock(0);
      lv_label_set_text(m_scanLabel, ("Scanning " + std::string(tunerTypeToName(type)) + "...").c_str());
      lvgl_port_unlock();
   }
   void close() override
   {
      lv_msgbox_close(m_popup);
   }
   void showSummaryAndClose(const ScanResults& results) override
   {
      lvgl_port_lock(0);
      lv_obj_t* content = lv_msgbox_get_content(m_popup);
      lv_obj_clean(content);

      lv_obj_t* titleObject = lv_obj_create(content);
      lv_obj_add_style(titleObject, &m_style, LV_STATE_DEFAULT);
      lv_obj_set_size(titleObject, SCAN_VOL_POPUP_WIDTH, SCAN_VOL_POPUP_SUMMARY_TITLE_HEIGHT);
      lv_obj_remove_flag(titleObject, LV_OBJ_FLAG_SCROLLABLE);

      lv_obj_t* title = lv_label_create(titleObject);
      lv_label_set_text(title, "SUMMARY");
      lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
      lv_obj_set_style_text_font(title, AIO_RADIO_BIG_FONT, 0);
      lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

      for (const auto& [type, count] : results)
      {
         lv_obj_t* tunerObject = lv_obj_create(content);
         lv_obj_add_style(tunerObject, &m_style, LV_STATE_DEFAULT);
         lv_obj_set_size(tunerObject, SCAN_VOL_POPUP_WIDTH, SCAN_VOL_POPUP_TUNER_SUMMARY_HEIGHT);
         lv_obj_center(tunerObject);
         lv_obj_remove_flag(tunerObject, LV_OBJ_FLAG_SCROLLABLE);

         lv_obj_t* summaryLabel = lv_label_create(tunerObject);
         lv_obj_add_style(summaryLabel, &m_style, LV_STATE_DEFAULT);
         lv_label_set_text(summaryLabel, (std::string(tunerTypeToName(type)) + ": " + std::to_string(count) + " station(s) found").c_str());
         lv_obj_align(summaryLabel, LV_ALIGN_CENTER, 0, 0);
         lv_obj_set_style_text_font(title, AIO_RADIO_MEDIUM_FONT, 0);
         lv_obj_set_style_text_align(summaryLabel, LV_TEXT_ALIGN_CENTER, 0);
      }
      lv_timer_create(scanPopupTimer, SCAN_SUMMARY_DISPLAY_TIME_MS, this);
      lvgl_port_unlock();
   }
   void notifyProgress(int percentage) override
   {
      lvgl_port_lock(0);
      lv_bar_set_value(m_bar, percentage, LV_ANIM_ON);
      lvgl_port_unlock();
   }
   void setStationsFound(int count) override
   {
      lvgl_port_lock(0);
      lv_label_set_text(m_scanCountLabel, ("Found " + std::to_string(count) + " station(s)").c_str());
      lvgl_port_unlock();
   }
private:
   lv_style_t m_style;
   lv_obj_t* m_popup = nullptr;
   lv_obj_t* m_scanLabel = nullptr;
   lv_obj_t* m_scanCountLabel = nullptr;
   lv_obj_t* m_bar = nullptr;
};

} // namespace display::gui::popups

