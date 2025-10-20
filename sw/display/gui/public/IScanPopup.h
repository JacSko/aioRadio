#pragma once
#include <cstdint>
#include <map>

#include "GUITypes.h"

namespace display::gui
{

typedef std::map<TunerType, uint16_t> ScanResults;

class IScanPopup
{
public:
    virtual ~IScanPopup() = default;

    virtual void initialize() = 0;
    virtual void show() = 0;
    virtual void setScanType(TunerType) = 0;
    virtual void showSummaryAndClose(const ScanResults&) = 0;
    virtual void close() = 0;
    virtual void notifyProgress(int percentage) = 0;
    virtual void setStationsFound(int count) = 0;


};

}

