#pragma once

#include "IMCP23017.h"

namespace drivers::encoder
{

class IEncoderListener
{
public:
   struct Event
   {
      enum class Type
      {
         ROTATE,
         BUTTON_PRESSED,
         BUTTON_RELEASED
      } type;
      int8_t diff;
   };

   virtual ~IEncoderListener() = default;
   virtual void onEvent(const Event&) = 0;
   static const char* eventName(const Event::Type& type)
   {
       switch (type)
       {
           case Event::Type::ROTATE:
               return "ROTATE";
           case Event::Type::BUTTON_PRESSED:
               return "BUTTON_PRESSED";
           case Event::Type::BUTTON_RELEASED:
               return "BUTTON_RELEASED";
           default:
               return "UNKNOWN";
       }
   }
};

class IEncoder
{
public:
    virtual ~IEncoder() = default;
    static std::unique_ptr<IEncoder> create(drivers::gpio::mcp23017::IMCP23017&, IEncoderListener&);
};

} // namespace drivers
