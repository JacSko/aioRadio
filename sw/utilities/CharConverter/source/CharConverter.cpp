#include <map>

#include "Log.h"
#include "CharConverter.h"
#include "TableCharsetConverter.hpp"

#include "EBULatinTable.h"

namespace utilities::charconverter
{

TableCharsetConverter g_ebuLatinTableConverter(EBULatinTable);

const std::map<Charset, ICharsetConverter&> g_converters =
{
   {Charset::EBU_LATIN, g_ebuLatinTableConverter}
};

std::string convertUtf16toUtf8(char16_t c)
{
   std::string result;

   if (0x80 > c)
   {
      result.resize(1);
      result[0] = static_cast<char>(c);
   }
   else if (0x800 > c)
   {
      result.resize(2);
      result[1] = static_cast<char>(0x80 | (c & 0x3F));
      c = (c >> 6);
      result[0] = static_cast<char>(0xC0 | c);
   }
   else
   {
      result.resize(3);
      result[2] = static_cast<char>(0x80 | (c & 0x3F));
      c = (c >> 6);
      result[1] = static_cast<char>(0x80 | (c & 0x3F));
      c = (c >> 6);
      result[0] = static_cast<char>(0xE0 | c);
   }
   return result;
}
std::u16string CharConverter::toUtf16(const RawString& input)
{
   const auto& converter = g_converters.find(input.charset);
   if (converter == g_converters.end())
   {
      AIO_LOGE("CharConverter", "Unknown charset=%d", static_cast<int>(input.charset));
      return std::u16string();
   }
   return converter->second.toUtf16(input);

}
std::string CharConverter::toUtf8(const std::u16string& input)
{
   std::string result;
   result.reserve(input.size());
   for(auto c : input)
   {
      if (0x0000u == c)
      {
         break;
      }
      result.append(convertUtf16toUtf8(c));
   }
   return result;
}

}
