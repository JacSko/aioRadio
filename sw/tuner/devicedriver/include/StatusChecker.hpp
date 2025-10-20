#pragma once
#include "TunerTypes.h"
#include "Log.h"

namespace tuner::devicedriver
{

struct StatusChecker
{
   enum class Status
   {
      CTS_OK,
      NO_CTS_YET,
      ERROR,
   };
   static Status check(const uint8_t* status)
   {
      uint32_t statusWord = status[4];
      statusWord |= (status[3] << 8);
      statusWord |= (status[2] << 16);
      statusWord |= (status[1] << 24);
      if (statusWord & static_cast<uint32_t>(EventType::ERRCMD) ||
          statusWord & static_cast<uint32_t>(EventType::RFFE_ERR) ||
          statusWord & static_cast<uint32_t>(EventType::DSP_ERR) ||
          statusWord & static_cast<uint32_t>(EventType::REPOFERR) ||
          statusWord & static_cast<uint32_t>(EventType::CMDOFERR) ||
          statusWord & static_cast<uint32_t>(EventType::ARBERR) ||
          statusWord & static_cast<uint32_t>(EventType::ERRNR))
      {
         return Status::ERROR;
      }
      return statusWord & static_cast<uint32_t>(EventType::CTS) ? Status::CTS_OK : Status::NO_CTS_YET;
   }
   static uint32_t toStatusWord(const uint8_t* status)
   {
      uint32_t statusWord = status[4];
      statusWord |= (status[3] << 8);
      statusWord |= (status[2] << 16);
      statusWord |= (status[1] << 24);
      return statusWord;
   }
};

}
