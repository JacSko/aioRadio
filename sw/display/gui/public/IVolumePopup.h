#pragma once
#include <cstdint>
#include <map>

#include "GUITypes.h"

namespace display::gui
{

class IVolumePopup
{
public:
    virtual ~IVolumePopup() = default;
    virtual void setVolume(int volume) = 0;
};

}

