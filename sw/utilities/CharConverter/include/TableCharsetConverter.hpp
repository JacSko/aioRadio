#pragma once
#include <array>

#include "ICharsetConverter.h"

namespace utilities::charconverter
{

class TableCharsetConverter : public ICharsetConverter
{
public:
   virtual ~TableCharsetConverter() = default;
   TableCharsetConverter(const std::array<uint16_t, 256>& table):
   m_table(table)
   {

   }
   std::u16string toUtf16(const RawString& rawString) override
   {
      if (rawString.data.empty())
      {
         return std::u16string();
      }

      std::u16string result;
      result.reserve(rawString.data.size());
      for(auto i = 0u; i < rawString.data.size(); i++)
      {
         uint16_t c = m_table[rawString.data[i]];
         if (c == '\0')
         {
            break;
         }
         result += static_cast<char16_t>(c);
      }
      return result;
   }
private:
   const std::array<uint16_t, 256>& m_table;
};

}
