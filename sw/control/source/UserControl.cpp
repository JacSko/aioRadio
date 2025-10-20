#include "UserControl.h"

#include "Log.h"

namespace control
{

static const char* TAG = "UserControl";
static const int TASK_STACK_SIZE = 4096;
static const int TASK_NOTIFY_WAIT_MS = 10000;
static const int TASK_PRIORITY = 2;

static void taskFunction(void* arg)
{
   UserControl* gpio = static_cast<UserControl*>(arg);
   gpio->task();
}

UserControl::UserControl(drivers::gpio::mcp23017::IMCP23017& mcpDriver,
                         display::gui::IMainGUI& gui,
                         audio::IAudioControl& audioControl) :
m_gui(gui),
m_encoder(drivers::encoder::IEncoder::create(mcpDriver, *this)),
m_audioControl(audioControl),
m_task(nullptr),
m_queue(nullptr)
{
   createQueue();
   createTask();
}
UserControl::~UserControl()
{
   destroyTask();
   destroyQueue();
}
void UserControl::createQueue()
{
   m_queue = xQueueCreate(5, sizeof(drivers::encoder::IEncoderListener::Event));
   AIO_LOGE_IF(!m_queue, TAG, "Failed to create interrupt queue");
   AIO_LOGD_IF(m_queue, TAG, "Interrupt queue created");
}
void UserControl::destroyQueue()
{
   AIO_LOGD(TAG, "Destroying interrupt queue");
   if (m_queue != nullptr)
   {
      vQueueDelete(m_queue);
      m_queue = nullptr;
   }
}
void UserControl::destroyTask()
{
   AIO_LOGD(TAG, "Destroying interrupt task");
   if (m_task != nullptr)
   {
      vTaskDelete(m_task);
      m_task = nullptr;
   }
}
void UserControl::createTask()
{
   AIO_LOGD(TAG, "Creating queue task");
   BaseType_t result = xTaskCreate(taskFunction, "UserControl", TASK_STACK_SIZE, this, TASK_PRIORITY, &m_task);
   AIO_LOGE_IF(result != pdPASS, TAG, "Failed to create queue task, error %d", result);
   AIO_LOGD_IF(result == pdPASS, TAG, "Created queue task");
}
void UserControl::task()
{
   drivers::encoder::IEncoderListener::Event event = {};
   for (;;)
   {
      if (xQueueReceive(m_queue, &event, pdMS_TO_TICKS(TASK_NOTIFY_WAIT_MS)) == pdTRUE)
      {
         AIO_LOGD(TAG, "Received event %u, diff %d", static_cast<int>(event.type), event.diff)
         switch(event.type)
         {
            case drivers::encoder::IEncoderListener::Event::Type::ROTATE:
               if (m_gui.getSettingsMenu().isMenuVisible())
               {
                  m_gui.controlMoved(event.diff);
               }
               else
               {
                  if (event.diff > 0)
                  {
                     m_audioControl.volumeUp();
                  }
                  else if (event.diff < 0)
                  {
                     m_audioControl.volumeDown();
                  }
                  m_gui.getVolumePopup().setVolume(m_audioControl.getVolume());
               }
               break;
            case drivers::encoder::IEncoderListener::Event::Type::BUTTON_RELEASED:
               m_gui.controlButtonReleased();
               break;
            case drivers::encoder::IEncoderListener::Event::Type::BUTTON_PRESSED:
               if (!m_gui.getSettingsMenu().isMenuVisible())
               {
                  m_gui.getSettingsMenu().showMenu(true);
               }
               else
               {
                  m_gui.controlButtonPressed();
               }
               break;
         }
      }
   }
}
void UserControl::onEvent(const drivers::encoder::IEncoderListener::Event& event)
{
   xQueueSend(m_queue, &event, 0);
}

}
