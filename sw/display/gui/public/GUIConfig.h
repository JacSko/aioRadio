#pragma once
#include <cstdint>
#include "lvgl.h"

LV_FONT_DECLARE(montserrat_ebu_latin_14)
LV_FONT_DECLARE(montserrat_ebu_latin_18)
LV_FONT_DECLARE(montserrat_ebu_latin_24)

#define AIO_RADIO_SMALL_FONT &montserrat_ebu_latin_14
#define AIO_RADIO_MEDIUM_FONT &montserrat_ebu_latin_18
#define AIO_RADIO_BIG_FONT &montserrat_ebu_latin_24

namespace display::gui
{

static constexpr uint32_t SCREEN_WIDTH = 480;
static constexpr uint32_t SCREEN_HEIGHT = 320;
static constexpr uint32_t TOP_RIBBON_HEIGHT = 30;
static constexpr uint32_t MAIN_VIEW_HEIGHT = 260;
static constexpr uint32_t BOTTOM_RIBBON_HEIGHT = 30;
static constexpr uint32_t DAB_SLS_WIDTH = 150;
static constexpr uint32_t DAB_SLS_HEIGHT = 150;

static constexpr uint32_t DISPLAY_BACKGROUND_COLOR = 0x22232A;

static constexpr uint32_t TOP_RIBBON_DEFAULT_TEXT_COLOR = 0xF0F0F0;
static constexpr uint32_t TOP_RIBBON_ACTIVE_TEXT_COLOR = 0x00FFFF;
static constexpr uint32_t TOP_RIBBON_DEFAULT_AUDIO_ICON_COLOR = 0xFF0000;
static constexpr uint32_t TOP_RIBBON_DEFAULT_WIFI_ICON_COLOR = 0xFF0000;
static constexpr uint32_t TOP_RIBBON_ACTIVE_AUDIO_ICON_COLOR = 0x00FF00;
static constexpr uint32_t TOP_RIBBON_ACTIVE_WIFI_ICON_COLOR = 0x00FF00;

static constexpr uint32_t MAIN_WINDOW_DEFAULT_TEXT_COLOR = 0xF0F0F0;
static constexpr uint32_t MAIN_WINDOW_ENSEMBLE_LABEL_TEXT_COLOR = 0x707070;
static constexpr uint32_t MAIN_WINDOW_RT_TAGS_WIDTH = 200;
static constexpr uint32_t MAIN_WINDOW_RT_WIDTH = 420;

static constexpr uint32_t BOTTOM_RIBBON_DEFAULT_TEXT_COLOR = 0x707070;

static constexpr uint16_t SETTINGS_MENU_WIDTH = 420;
static constexpr uint16_t SETTINGS_MENU_HEIGHT = 280;
static constexpr uint32_t SETTINGS_MENU_BACKGROUND_COLOR = DISPLAY_BACKGROUND_COLOR;
static constexpr uint32_t SETTINGS_MENU_TEXT_COLOR = TOP_RIBBON_DEFAULT_TEXT_COLOR;
static constexpr uint32_t SETTINGS_MENU_OUTLINE_COLOR = 0x00FFFF;
static constexpr uint32_t SETTINGS_MENU_FOCUSED_BACKGROUND_COLOR = 0x00FFFF;
static constexpr uint32_t SETTINGS_MENU_FOCUSED_BORDER_COLOR = 0x00FFFF;
static constexpr uint32_t SETTINGS_MENU_FOCUSED_TEXT_COLOR = DISPLAY_BACKGROUND_COLOR;
static constexpr uint8_t SETTINGS_MENU_FOCUSED_RADIUS = 10;
static constexpr uint8_t SETTINGS_MENU_FOCUSED_BG_OPA= 180;
static constexpr uint8_t SETTINGS_MENU_FOCUSED_BORDER_OPA = 255;
static constexpr uint8_t SETTINGS_MENU_FOCUSED_BORDER_WIDTH = 3;

static constexpr uint32_t SCAN_VOL_POPUP_WIDTH = 300;
static constexpr uint32_t SCAN_VOL_POPUP_HEIGHT = 170;
static constexpr uint32_t SCAN_VOL_POPUP_TITLE_HEIGHT = 100;
static constexpr uint32_t SCAN_VOL_POPUP_PROGRESS_BAR_HEIGHT = 10;
static constexpr uint32_t SCAN_VOL_POPUP_PROGRESS_BAR_WIDTH= 260;
static constexpr uint32_t SCAN_VOL_POPUP_SUMMARY_HEIGHT = 50;

static constexpr uint32_t SCAN_VOL_POPUP_SUMMARY_TITLE_HEIGHT = 80;
static constexpr uint32_t SCAN_VOL_POPUP_TUNER_SUMMARY_HEIGHT = 15;
static constexpr uint32_t SCAN_SUMMARY_DISPLAY_TIME_MS = 5000;
static constexpr uint32_t SCAN_VOL_DEFAULT_TEXT_COLOR = 0xF0F0F0;
static constexpr uint32_t VOL_POPUP_DISPLAY_TIME_MS = 5000;

}
