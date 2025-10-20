#pragma once

namespace display::gui
{

enum class TunerType
{
   ALL,
   DAB,
   FM,
   ONLINE,
};

static inline const char* tunerTypeToName(const TunerType& type)
{
   switch(type)
   {
      case TunerType::DAB:
         return "DAB";
      case TunerType::FM:
         return "FM";
      case TunerType::ONLINE:
         return "ONLINE";
      case TunerType::ALL:
         return "ALL";
      default:
         return "UNKNOWN";
   }
}


}
