#pragma once
#include <vector>
#include <string>
#include <stdint.h>


namespace utilities::charconverter
{

enum class Charset
{
    /* according to ETSI TS 101 756 clause 5.2 */
    EBU_LATIN = 0x00,
    ISO_10646_UCS2 = 0x06,
    ISO_10646_UTF_8 = 0x0F,
};

struct RawString
{
    Charset charset;
    std::vector<uint8_t> data;
};

struct CharConverter
{
    static std::u16string toUtf16(const RawString& input);
    static std::string toUtf8(const std::u16string& input);
};
}
