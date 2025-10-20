#pragma once
#include <variant>
#include <string>
#include "TunerTypes.h"


namespace tuner
{

struct DABUri
{
   ActionType action;
   uint32_t frequencyKHz;
   uint32_t ensembleID;
   uint32_t serviceID;
   uint32_t componentID;
};

struct FMUri
{
   ActionType action;
   uint32_t frequencyKHz;
   uint32_t piCode;
};

struct OnlineUri
{
   ActionType action;
   std::string streamUrl;
};

typedef std::string Uri;
typedef std::variant<std::monostate, DABUri, FMUri, OnlineUri> AnyUri;


class UriParser
{
   static inline Uri toUri(const DABUri& dabUri)
   {
      std::string uri = "dab://";
      if (dabUri.action == ActionType::TUNE_SELECT)
      {
         uri += std::to_string(dabUri.frequencyKHz) + ",";
         uri += std::to_string(dabUri.ensembleID) + ",";
         uri += std::to_string(dabUri.serviceID) + ",";
         uri += std::to_string(dabUri.componentID);
      }
      else if (dabUri.action == ActionType::SCAN)
      {
         uri += "scan";
      }
      return uri;
   }
   static inline Uri toUri(const FMUri& fmUri)
   {
      std::string uri = "fm://";
      if (fmUri.action == ActionType::TUNE_SELECT)
      {
         uri += std::to_string(fmUri.frequencyKHz) + ",";
         uri += std::to_string(fmUri.piCode);
      }
      else if (fmUri.action == ActionType::SCAN)
      {
         uri += "scan";
      }
      return uri;

   }
   static inline Uri toUri(const OnlineUri& onlineUri)
   {
      std::string uri = "online://";
      uri += onlineUri.action == ActionType::TUNE_SELECT? onlineUri.streamUrl : "scan";
      return uri;
   }

   AnyUri fromUri(const Uri& uri)
   {
      std::size_t pos = uri.find("://");
      if (pos != std::string::npos)
      {
         std::string type = uri.substr(0, pos);
         std::string params = uri.substr(pos + 3);
         if (type == "dab")
         {
            return parseDABUri(params);
         }
         else if (type == "fm")
         {
            return parseFMUri(params);
         }
         else if (type == "online")
         {
            return parseOnlineUri(params);
         }
      }
      return std::monostate{};
   }

private:
   DABUri parseDABUri(const std::string& path)
   {
      DABUri dabUri = {};

      if (path == "scan")
      {
         dabUri.action = ActionType::SCAN;
      }
      else
      {
         std::size_t firstComma = path.find(",");
         std::size_t secondComma = path.find(",", firstComma + 1);
         std::size_t thirdComma = path.find(",", secondComma + 1);

         if (firstComma != std::string::npos)
         {
            dabUri.frequencyKHz = std::stoul(path.substr(0, firstComma));
         }
         if (secondComma != std::string::npos)
         {
            dabUri.ensembleID = std::stoul(path.substr(firstComma + 1, secondComma - firstComma - 1));
         }
         if (thirdComma != std::string::npos)
         {
            dabUri.serviceID = std::stoul(path.substr(secondComma + 1, thirdComma - secondComma - 1));
            dabUri.componentID = std::stoul(path.substr(thirdComma + 1));
         }
      }
      return dabUri;
   }
   FMUri parseFMUri(const std::string& path)
   {
      FMUri fmUri = {};

      if (path == "scan")
      {
         fmUri.action = ActionType::SCAN;
      }
      else
      {
         std::size_t commaPos = path.find(",");

         if (commaPos != std::string::npos)
         {
            fmUri.frequencyKHz = std::stoul(path.substr(0, commaPos));
            fmUri.piCode = std::stoul(path.substr(commaPos + 1));
         }
      }
      return fmUri;

   }
   OnlineUri parseOnlineUri(const std::string& path)
   {
      OnlineUri onlineUri = {};

      if (path == "scan")
      {
         onlineUri.action = ActionType::SCAN;
      }
      else
      {
         onlineUri.action = ActionType::TUNE_SELECT;
         onlineUri.streamUrl = path;
      }
      return onlineUri;
   }
};



}
