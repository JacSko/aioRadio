#include <vector>
#include <optional>

namespace utilities
{

struct TableConverter
{
   template<typename FromType, typename ToType>
   static std::optional<ToType>
   convert(const std::vector<std::pair<FromType, ToType>>& table, FromType fromValue)
   {
      for (const auto& entry : table)
      {
         if (entry.first == fromValue)
         {
            return entry.second;
         }
      }
      return std::nullopt;
   }
};

struct TableConverterReversed
{
   template<typename ToType, typename FromType>
   static std::optional<ToType>
   convert(const std::vector<std::pair<ToType, FromType>>& table, FromType fromValue)
   {
      for (const auto& entry : table)
      {
         if (entry.second == fromValue)
         {
            return entry.first;
         }
      }
      return std::nullopt;
   }
};

} // namespace utilities
