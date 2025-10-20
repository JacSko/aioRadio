#pragma once
#include <string>
#include <cstdint>

#include "lvgl.h"
#include "esp_lvgl_port.h"

#include "IVolumePopup.h"
#include "GUIConfig.h"

namespace display::gui::popups
{

static inline void volumePopupTimer(lv_timer_t * timer);

class VolumePopup : public IVolumePopup
{
public:
   VolumePopup()
   {
   }
   ~VolumePopup()
   {
   }

   void initialize()
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

   void setVolume(int volume) override
   {
      lvgl_port_lock(0);
      if (!m_popup)
      {
         show();
      }
      lv_bar_set_value(m_bar, volume, LV_ANIM_ON);
      lv_label_set_text_fmt(m_summaryLabel, "Volume: %d/100", volume);
      lv_timer_reset(m_timer);
      lvgl_port_unlock();
   }

   void close()
   {
      lv_msgbox_close(m_popup);
      m_popup = nullptr;
   }
private:
   void show()
   {
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

      m_titleLabel = lv_label_create(titleObject);
      lv_label_set_text(m_titleLabel, "Volume");
      lv_obj_set_style_text_align(m_titleLabel, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_align(m_titleLabel, LV_ALIGN_CENTER, 0, 0);
      lv_obj_set_style_text_font(m_titleLabel, AIO_RADIO_BIG_FONT, 0);

      m_bar = lv_bar_create(progressBarObject);
      lv_obj_center(m_bar);
      lv_obj_set_size(m_bar, SCAN_VOL_POPUP_PROGRESS_BAR_WIDTH, SCAN_VOL_POPUP_PROGRESS_BAR_HEIGHT);

      m_summaryLabel = lv_label_create(stationsFoundObject);
      lv_obj_set_style_text_align(m_summaryLabel, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_align(m_summaryLabel, LV_ALIGN_CENTER, 0, 0);
      lv_obj_set_style_text_font(m_summaryLabel, AIO_RADIO_SMALL_FONT, 0);

      m_timer = lv_timer_create(volumePopupTimer, VOL_POPUP_DISPLAY_TIME_MS, this);
   }

   lv_style_t m_style;
   lv_obj_t* m_popup = nullptr;
   lv_obj_t* m_titleLabel = nullptr;
   lv_obj_t* m_summaryLabel = nullptr;
   lv_obj_t* m_bar = nullptr;
   lv_timer_t* m_timer = nullptr;
};

static inline void volumePopupTimer(lv_timer_t * timer)
{
   VolumePopup* volumePopup = static_cast<VolumePopup*>(lv_timer_get_user_data(timer));;
   volumePopup->close();
   lv_timer_delete(timer);
}

} // namespace display::gui::popups

