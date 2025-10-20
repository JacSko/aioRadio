#pragma once
#include <cstdint>
#include <string>

namespace display::gui
{

enum class ViewType
{
   FM,
   DAB,
   ONLINE,
};

class IFMView
{
public:
    virtual ~IFMView() = default;
    virtual void setStationName(const std::string&) = 0;
    virtual void setPty(const std::string&) = 0;
    virtual void setArtist(const std::string&) = 0;
    virtual void setTitle(const std::string&) = 0;
    virtual void setRadiotext(const std::string&) = 0;
    virtual void setPI(uint32_t pi) = 0;
    virtual void setFrequency(uint32_t frequencyKHz) = 0;
};

class IDABView
{
public:
    virtual ~IDABView() = default;
    virtual void setServiceName(const std::string&) = 0;
    virtual void setEnsembleName(const std::string&) = 0;
    virtual void setPty(const std::string&) = 0;
    virtual void setArtist(const std::string&) = 0;
    virtual void setTitle(const std::string&) = 0;
    virtual void setRadiotext(const std::string&) = 0;
    virtual void setServiceID(uint32_t sid) = 0;
    virtual void setEnsembleID(uint32_t eid) = 0;
    virtual void setFrequency(uint32_t frequencyKHz) = 0;
};

class IOnlineView
{
public:

};
}
