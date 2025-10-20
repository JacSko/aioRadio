#pragma once
#include "CharConverter.h"

namespace utilities::charconverter
{

class ICharsetConverter
{
public:
    virtual ~ICharsetConverter() = default;
    virtual std::u16string toUtf16(const RawString&) = 0;
};

}
