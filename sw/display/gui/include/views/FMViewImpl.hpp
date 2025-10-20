#include "lvgl.h"
#include "esp_lvgl_port.h"

#include "IRadioViews.h"
#include "GUIConfig.h"

namespace display::gui
{

class FMViewImpl : public IFMView
{
public:
   FMViewImpl():
   m_stationNameLabel(nullptr),
   m_ptyLabel(nullptr),
   m_artistLabel(nullptr),
   m_titleLabel(nullptr),
   m_radiotextLabel(nullptr)
   {
   }

   void draw(lv_obj_t* mainRibbon, lv_obj_t* statusRibbon)
   {
      lv_style_init(&m_stationNameStyle);
      lv_style_set_text_font(&m_stationNameStyle, AIO_RADIO_BIG_FONT);
      lv_style_set_text_color(&m_stationNameStyle, lv_color_hex(MAIN_WINDOW_DEFAULT_TEXT_COLOR));

      lv_style_init(&m_ptyStyle);
      lv_style_set_text_font(&m_ptyStyle, AIO_RADIO_SMALL_FONT);
      lv_style_set_text_color(&m_ptyStyle, lv_color_hex(MAIN_WINDOW_DEFAULT_TEXT_COLOR));

      lv_style_init(&m_radiotextTagsStyle);
      lv_style_set_text_font(&m_radiotextTagsStyle, AIO_RADIO_MEDIUM_FONT);
      lv_style_set_text_color(&m_radiotextTagsStyle, lv_color_hex(MAIN_WINDOW_DEFAULT_TEXT_COLOR));

      lv_style_init(&m_radiotextStyle);
      lv_style_set_text_font(&m_radiotextStyle, AIO_RADIO_MEDIUM_FONT);
      lv_style_set_text_color(&m_radiotextStyle, lv_color_hex(MAIN_WINDOW_DEFAULT_TEXT_COLOR));

      m_stationNameLabel = lv_label_create(mainRibbon);
      lv_label_set_text(m_stationNameLabel, "");
      lv_obj_add_style(m_stationNameLabel, &m_stationNameStyle, 0);
      lv_obj_align(m_stationNameLabel, LV_ALIGN_TOP_LEFT, 40, 30);

      m_ptyLabel= lv_label_create(mainRibbon);
      lv_label_set_text(m_ptyLabel, "");
      lv_obj_add_style(m_ptyLabel, &m_ptyStyle, 0);
      lv_obj_align_to(m_ptyLabel, m_stationNameLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

      m_artistLabel = lv_label_create(mainRibbon);
      lv_label_set_text(m_artistLabel, "");
      lv_obj_add_style(m_artistLabel, &m_radiotextTagsStyle, 0);
      lv_obj_align_to(m_artistLabel, m_ptyLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 50);
      lv_label_set_long_mode(m_artistLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
      lv_obj_set_width(m_artistLabel, MAIN_WINDOW_RT_TAGS_WIDTH);

      m_titleLabel = lv_label_create(mainRibbon);
      lv_label_set_text(m_titleLabel, "");
      lv_obj_add_style(m_titleLabel, &m_radiotextTagsStyle, 0);
      lv_obj_align_to(m_titleLabel, m_artistLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
      lv_label_set_long_mode(m_titleLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
      lv_obj_set_width(m_titleLabel, MAIN_WINDOW_RT_TAGS_WIDTH);

      m_radiotextLabel = lv_label_create(mainRibbon);
      lv_label_set_text(m_radiotextLabel, "");
      lv_obj_add_style(m_radiotextLabel, &m_radiotextStyle, 0);
      lv_obj_align_to(m_radiotextLabel, m_titleLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
      lv_label_set_long_mode(m_radiotextLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
      lv_obj_set_width(m_radiotextLabel, MAIN_WINDOW_RT_WIDTH);

      m_frequencyLabel = lv_label_create(statusRibbon);
      lv_label_set_text(m_frequencyLabel, "");
      lv_obj_align(m_frequencyLabel, LV_ALIGN_LEFT_MID, 20, 0);

      m_piLabel = lv_label_create(statusRibbon);
      lv_label_set_text(m_piLabel, "");
      lv_obj_align_to(m_piLabel, m_frequencyLabel, LV_ALIGN_OUT_RIGHT_MID, 200, 0);
   }

private:
   void setStationName(const std::string& name)
   {
      lvgl_port_lock(0);
      lv_label_set_text(m_stationNameLabel, name.c_str());
      lvgl_port_unlock();
   }
   void setPty(const std::string& pty)
   {
      lvgl_port_lock(0);
      lv_label_set_text(m_ptyLabel, pty.c_str());
      lvgl_port_unlock();
   }
   void setArtist(const std::string& artist)
   {
      lvgl_port_lock(0);
      lv_label_set_text(m_artistLabel, artist.c_str());
      lvgl_port_unlock();
   }
   void setTitle(const std::string& title)
   {
      lvgl_port_lock(0);
      lv_label_set_text(m_titleLabel, title.c_str());
      lvgl_port_unlock();
   }
   void setRadiotext(const std::string& radiotext)
   {
      lvgl_port_lock(0);
      lv_label_set_text(m_radiotextLabel, radiotext.c_str());
      lvgl_port_unlock();
   }
   void setPI(uint32_t pi)
   {
      lvgl_port_lock(0);
      lv_label_set_text_fmt(m_piLabel, "PI: %.4lX", pi);
      lvgl_port_unlock();
   }
   void setFrequency(uint32_t frequencyKHz)
   {
      lvgl_port_lock(0);
      lv_label_set_text_fmt(m_frequencyLabel, "Frequency: %lu kHz", frequencyKHz);
      lvgl_port_unlock();
   }

   lv_obj_t* m_stationNameLabel;
   lv_obj_t* m_ptyLabel;
   lv_obj_t* m_artistLabel;
   lv_obj_t* m_titleLabel;
   lv_obj_t* m_radiotextLabel;
   lv_obj_t* m_frequencyLabel;
   lv_obj_t* m_piLabel;
   lv_style_t m_stationNameStyle;
   lv_style_t m_ptyStyle;
   lv_style_t m_radiotextTagsStyle;
   lv_style_t m_radiotextStyle;
};

}
