#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "IMainGUI.h"
#include "IEncoder.h"
#include "IAudioControl.h"
#include "IMCP23017.h"

namespace control
{

class UserControl : public drivers::encoder::IEncoderListener
{
public:
   UserControl(drivers::gpio::mcp23017::IMCP23017&, display::gui::IMainGUI&, audio::IAudioControl&);
   ~UserControl();

   UserControl(UserControl&) = delete;
   UserControl(UserControl&&) = delete;
   UserControl operator=(UserControl&) = delete;
   UserControl operator=(UserControl&&) = delete;

   void task();
private:
   void onEvent(const Event&) override;
   void createQueue();
   void createTask();
   void destroyQueue();
   void destroyTask();

   display::gui::IMainGUI& m_gui;
   std::unique_ptr<drivers::encoder::IEncoder> m_encoder;
   audio::IAudioControl& m_audioControl;

   TaskHandle_t m_task;
   QueueHandle_t m_queue;
};


}
