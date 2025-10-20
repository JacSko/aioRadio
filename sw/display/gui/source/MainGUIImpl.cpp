#include <mutex>

#include "esp_lvgl_port.h"

#include "MainGUIImpl.h"
#include "GUIConfig.h"

static std::mutex s_controlMutex;
static bool s_controlButtonPressed = false;
static int16_t s_controlMoved= 0;
static const char* TAG = "MainGUI";

static void mainGUIEncoderRead(lv_indev_t * indev, lv_indev_data_t * data)
{
   std::lock_guard<std::mutex> lock(s_controlMutex);

   data->enc_diff = s_controlMoved;
   data->state = s_controlButtonPressed? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
   s_controlMoved = 0;
   // AIO_LOGE(TAG, "enc_diff %d state %u\n", data->enc_diff, data->state == LV_INDEV_STATE_PRESSED);
}

namespace display::gui
{

std::unique_ptr<IMainGUI> IMainGUI::create()
{
   return std::make_unique<MainGUIImpl>();
}

MainGUIImpl::MainGUIImpl()
{

}

MainGUIImpl::~MainGUIImpl()
{

}
void MainGUIImpl::initializeStyles()
{
   lv_style_init(&m_topRibbon.style);
   lv_style_set_bg_color(&m_topRibbon.style, lv_color_hex(DISPLAY_BACKGROUND_COLOR));
   lv_style_set_text_color(&m_topRibbon.style, lv_color_hex(TOP_RIBBON_DEFAULT_TEXT_COLOR));
   lv_style_set_text_font(&m_topRibbon.style, AIO_RADIO_MEDIUM_FONT);
   lv_style_set_border_width(&m_topRibbon.style, 0);
   lv_style_set_outline_width(&m_topRibbon.style, 0);
   lv_style_set_pad_all(&m_topRibbon.style, 0);
   lv_style_set_outline_pad(&m_topRibbon.style, 0);
   lv_style_set_border_side(&m_topRibbon.style, LV_BORDER_SIDE_NONE);

   lv_style_init(&m_mainWindowStyle);
   lv_style_set_bg_color(&m_mainWindowStyle, lv_color_hex(DISPLAY_BACKGROUND_COLOR));
   lv_style_set_border_width(&m_mainWindowStyle, 0);
   lv_style_set_outline_width(&m_mainWindowStyle, 0);
   lv_style_set_pad_all(&m_mainWindowStyle, 0);
   lv_style_set_outline_pad(&m_mainWindowStyle, 0);
   lv_style_set_border_side(&m_mainWindowStyle, LV_BORDER_SIDE_NONE);

   lv_style_init(&m_bottomRibbonStyle);
   lv_style_set_bg_color(&m_bottomRibbonStyle, lv_color_hex(DISPLAY_BACKGROUND_COLOR));
   lv_style_set_text_color(&m_bottomRibbonStyle, lv_color_hex(BOTTOM_RIBBON_DEFAULT_TEXT_COLOR));
   lv_style_set_text_font(&m_bottomRibbonStyle, AIO_RADIO_SMALL_FONT);
   lv_style_set_border_width(&m_bottomRibbonStyle, 0);
   lv_style_set_outline_width(&m_bottomRibbonStyle, 0);
   lv_style_set_pad_all(&m_bottomRibbonStyle, 0);
   lv_style_set_outline_pad(&m_bottomRibbonStyle, 0);
   lv_style_set_border_side(&m_bottomRibbonStyle, LV_BORDER_SIDE_NONE);

}
void MainGUIImpl::renderTopRibbon()
{
   m_topRibbon.ribbon = lv_obj_create(m_screen);
   lv_obj_remove_flag(m_topRibbon.ribbon, LV_OBJ_FLAG_SCROLLABLE);
   lv_obj_set_size(m_topRibbon.ribbon, SCREEN_WIDTH, TOP_RIBBON_HEIGHT);
   lv_obj_align(m_topRibbon.ribbon, LV_ALIGN_TOP_MID, 0, 0);
   lv_obj_add_style(m_topRibbon.ribbon, &m_topRibbon.style, 0);

   m_topRibbon.fmLabel= lv_label_create(m_topRibbon.ribbon);
   lv_label_set_text(m_topRibbon.fmLabel, "FM");
   lv_obj_add_style(m_topRibbon.fmLabel, &m_topRibbon.style, 0);
   lv_obj_align(m_topRibbon.fmLabel, LV_ALIGN_LEFT_MID, 20, 0);

   m_topRibbon.dabLabel= lv_label_create(m_topRibbon.ribbon);
   lv_label_set_text(m_topRibbon.dabLabel, "DAB");
   lv_obj_add_style(m_topRibbon.dabLabel, &m_topRibbon.style, 0);
   lv_obj_align_to(m_topRibbon.dabLabel, m_topRibbon.fmLabel, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
   lv_obj_set_style_text_color(m_topRibbon.dabLabel, lv_color_hex(TOP_RIBBON_DEFAULT_TEXT_COLOR), 0);

   m_topRibbon.onlineLabel= lv_label_create(m_topRibbon.ribbon);
   lv_label_set_text(m_topRibbon.onlineLabel, "ONLINE");
   lv_obj_add_style(m_topRibbon.onlineLabel, &m_topRibbon.style, 0);
   lv_obj_align_to(m_topRibbon.onlineLabel, m_topRibbon.dabLabel, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

   m_topRibbon.wifiIcon = lv_label_create(m_topRibbon.ribbon);
   lv_label_set_text(m_topRibbon.wifiIcon, LV_SYMBOL_WIFI);
   lv_obj_align(m_topRibbon.wifiIcon, LV_ALIGN_RIGHT_MID, -20, 0);
   lv_obj_set_style_text_color(m_topRibbon.wifiIcon, lv_color_hex(TOP_RIBBON_DEFAULT_WIFI_ICON_COLOR), 0);

   m_topRibbon.audioIcon = lv_label_create(m_topRibbon.ribbon);
   lv_label_set_text(m_topRibbon.audioIcon, LV_SYMBOL_AUDIO);
   lv_obj_align_to(m_topRibbon.audioIcon, m_topRibbon.wifiIcon, LV_ALIGN_OUT_LEFT_MID, -20, 0);
   lv_obj_set_style_text_color(m_topRibbon.audioIcon, lv_color_hex(TOP_RIBBON_DEFAULT_AUDIO_ICON_COLOR), 0);
}
void MainGUIImpl::renderMainWindow()
{
   m_mainWindow = lv_obj_create(m_screen);
   lv_obj_remove_flag(m_mainWindow, LV_OBJ_FLAG_SCROLLABLE);
   lv_obj_set_size(m_mainWindow, SCREEN_WIDTH, MAIN_VIEW_HEIGHT);
   lv_obj_align_to(m_mainWindow, m_topRibbon.ribbon, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
   lv_obj_add_style(m_mainWindow, &m_mainWindowStyle, 0);
}
void MainGUIImpl::renderBottomRibbon()
{
   m_bottomRibbon = lv_obj_create(m_screen);
   lv_obj_remove_flag(m_bottomRibbon, LV_OBJ_FLAG_SCROLLABLE);
   lv_obj_set_size(m_bottomRibbon, SCREEN_WIDTH, BOTTOM_RIBBON_HEIGHT);
   lv_obj_align_to(m_bottomRibbon, m_mainWindow, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
   lv_obj_add_style(m_bottomRibbon, &m_bottomRibbonStyle, 0);
}
void MainGUIImpl::initialize(ViewType type)
{
   lvgl_port_lock(0);
   m_screen = lv_obj_create(nullptr);
   lv_obj_set_style_bg_color(m_screen, lv_color_hex(DISPLAY_BACKGROUND_COLOR), 0);
   initializeStyles();
   renderTopRibbon();
   renderMainWindow();
   renderBottomRibbon();
   lv_screen_load(m_screen);
   showMainView(type);
   setupSettingsMenu();
   m_scanPopup.initialize();
   m_volumePopup.initialize();
   lvgl_port_unlock();
}
void MainGUIImpl::finalize()
{
   lvgl_port_lock(0);
   lv_obj_del(m_screen);
   lvgl_port_unlock();
}
void MainGUIImpl::showMainView(ViewType type)
{
   lvgl_port_lock(0);
   markAllBandsInactive();
   lv_obj_clean(m_mainWindow);
   lv_obj_clean(m_bottomRibbon);
   switch(type)
   {
   case ViewType::FM:
      m_fmView.draw(m_mainWindow, m_bottomRibbon);
      markBandActive(m_topRibbon.fmLabel, true);
      break;
   case ViewType::DAB:
      m_dabView.draw(m_mainWindow, m_bottomRibbon);
      markBandActive(m_topRibbon.dabLabel, true);
      break;
   case ViewType::ONLINE:
      break;
   }
   lvgl_port_unlock();
}
void MainGUIImpl::setupSettingsMenu()
{
   lvgl_port_lock(0);
   lv_indev_t * indev = lv_indev_create();
   lv_group_t* group = lv_group_create();

   lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
   lv_indev_set_read_cb(indev, mainGUIEncoderRead);
   lv_indev_set_group(indev, group);
   lv_group_set_default(group);
   m_settingsMenu.initialize();
   lvgl_port_unlock();
}
void MainGUIImpl::setAudioStatus(bool audioPlaying)
{
   lvgl_port_lock(0);
   lv_obj_set_style_text_color(m_topRibbon.audioIcon, lv_color_hex(audioPlaying? TOP_RIBBON_ACTIVE_AUDIO_ICON_COLOR : TOP_RIBBON_DEFAULT_AUDIO_ICON_COLOR), 0);
   lvgl_port_unlock();
}
void MainGUIImpl::setWifiConnectionStatus(bool wifiConnected)
{
   lvgl_port_lock(0);
   lv_obj_set_style_text_color(m_topRibbon.wifiIcon, lv_color_hex(wifiConnected? TOP_RIBBON_ACTIVE_WIFI_ICON_COLOR : TOP_RIBBON_DEFAULT_WIFI_ICON_COLOR), 0);
   lvgl_port_unlock();
}
void MainGUIImpl::markAllBandsInactive()
{
   markBandActive(m_topRibbon.fmLabel, false);
   markBandActive(m_topRibbon.dabLabel, false);
   markBandActive(m_topRibbon.onlineLabel, false);
}
void MainGUIImpl::markBandActive(lv_obj_t* label, bool active)
{
   lv_obj_set_style_text_color(label, lv_color_hex(active? TOP_RIBBON_ACTIVE_TEXT_COLOR : TOP_RIBBON_DEFAULT_TEXT_COLOR), 0);
}
IFMView& MainGUIImpl::getFMView()
{
   return m_fmView;
}
IDABView& MainGUIImpl::getDABView()
{
   return m_dabView;
}
ISettingsMenu& MainGUIImpl::getSettingsMenu()
{
   return m_settingsMenu;
}
IScanPopup& MainGUIImpl::getScanPopup()
{
   return m_scanPopup;
}
IVolumePopup& MainGUIImpl::getVolumePopup()
{
   return m_volumePopup;
}
void MainGUIImpl::controlMoved(int16_t delta)
{
   std::lock_guard<std::mutex> lock(s_controlMutex);
   s_controlMoved = delta;
}
void MainGUIImpl::controlButtonPressed()
{
   std::lock_guard<std::mutex> lock(s_controlMutex);
   s_controlButtonPressed = true;
}
void MainGUIImpl::controlButtonReleased()
{
   std::lock_guard<std::mutex> lock(s_controlMutex);
   s_controlButtonPressed = false;
}

}
