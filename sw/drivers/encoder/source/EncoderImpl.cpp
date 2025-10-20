#include "EncoderImpl.h"
#include "Log.h"

namespace drivers::encoder
{
using namespace gpio;

static const char* TAG = "Encoder";
static const int TASK_STACK_SIZE = 4096;
static const int TASK_NOTIFY_WAIT_MS = 1000;
static const int TASK_PRIORITY = 2;
static const mcp23017::PIN PIN_A = mcp23017::PIN::GPIOA1;
static const mcp23017::PIN PIN_B = mcp23017::PIN::GPIOA0;
static const mcp23017::PIN PIN_BUTTON = mcp23017::PIN::GPIOB6;

std::unique_ptr<IEncoder> IEncoder::create(drivers::gpio::mcp23017::IMCP23017& driver, IEncoderListener& listener)
{
    return std::make_unique<EncoderImpl>(driver, listener);
}

static void mcp_gpio_listener(mcp23017::PIN pin, bool state , void* arg)
{
   EncoderImpl* enc = static_cast<EncoderImpl*>(arg);
   enc->onInterrupt(pin, state);
}

EncoderImpl::EncoderImpl(drivers::gpio::mcp23017::IMCP23017& driver, IEncoderListener& listener):
m_driver(driver),
m_listener(listener),
m_edgesMutex(xSemaphoreCreateMutex())
{
   AIO_ASSERT(m_edgesMutex != nullptr, TAG, "Failed to create listeners mutex");
   openEncoderPin(PIN_A);
   openEncoderPin(PIN_B);
   openEncoderPin(PIN_BUTTON);
}
EncoderImpl::~EncoderImpl()
{
   closeEncoderPin(PIN_A);
   closeEncoderPin(PIN_B);
   closeEncoderPin(PIN_BUTTON);
}
void EncoderImpl::openEncoderPin(mcp23017::PIN pin)
{
   m_driver.setPullup(pin, mcp23017::Pullup::ENABLED);
   m_driver.setInterruptCompare(pin, mcp23017::InterruptCompare::COMPARE_PREVIOUS);
   m_driver.setDirection(pin,mcp23017::Direction::INPUT);
   m_driver.setListener(pin, mcp_gpio_listener, this);
}
void EncoderImpl::closeEncoderPin(mcp23017::PIN pin)
{
   m_driver.removeListener(pin);
}
void EncoderImpl::onInterrupt(mcp23017::PIN pin, bool state)
{
   IEncoderListener::Event event = {};
   switch(pin)
   {
      case PIN_A:
         AIO_LOGD(TAG, "Interrupt on encoder pin A, state: %d", static_cast<int>(state));
         xSemaphoreTake(m_edgesMutex, portMAX_DELAY);
         m_lastEdges <<= 8;
         m_lastEdges |= state ? PIN_A_RISING : PIN_A_FALLING;
         checkEncoderMove();
         xSemaphoreGive(m_edgesMutex);
         break;
      case PIN_B:
         AIO_LOGD(TAG, "Interrupt on encoder pin B, state: %d", static_cast<int>(state));
         xSemaphoreTake(m_edgesMutex, portMAX_DELAY);
         m_lastEdges <<= 8;
         m_lastEdges |= state? PIN_B_RISING : PIN_B_FALLING;
         checkEncoderMove();
         xSemaphoreGive(m_edgesMutex);
         break;
      case PIN_BUTTON:
         AIO_LOGD(TAG, "Interrupt on encoder button, state: %d", static_cast<int>(state));
         event.type = state? IEncoderListener::Event::Type::BUTTON_RELEASED:
                             IEncoderListener::Event::Type::BUTTON_PRESSED;
         event.diff = 0;
         m_listener.onEvent(event);
         break;
      default:
         AIO_LOGW(TAG, "Interrupt on unknown GPIO ID: %.2x", static_cast<int>(pin));
         break;
   }
}
void EncoderImpl::checkEncoderMove()
{
   static const uint32_t CW_PATTERN = (PIN_A_FALLING << 24) | (PIN_B_FALLING << 16) | (PIN_A_RISING << 8) | (PIN_B_RISING);
   static const uint32_t CCW_PATTERN = (PIN_B_FALLING << 24) | (PIN_A_FALLING << 16) | (PIN_B_RISING << 8) | (PIN_A_RISING);

   if (m_lastEdges == CW_PATTERN)
   {
      IEncoderListener::Event event = {};
      event.type = IEncoderListener::Event::Type::ROTATE;
      event.diff = -1;
      m_listener.onEvent(event);
      m_lastEdges = 0;
   }
   else if (m_lastEdges == CCW_PATTERN)
   {
      IEncoderListener::Event event = {};
      event.type = IEncoderListener::Event::Type::ROTATE;
      event.diff = 1;
      m_listener.onEvent(event);
      m_lastEdges = 0;
   }
}

} // namespace drivers
