#include <gtest/gtest.h>
#include "TableConverter.hpp"

TEST(TableConverterTest, ConvertsStringToInt)
{
   std::vector<std::pair<std::string, int>> table = {
       {"apple", 10},
       {"banana", 20}
   };

   auto result = TableConverter::convert(table, std::string("banana"));
   ASSERT_TRUE(result.has_value());
   EXPECT_EQ(result.value(), 20);
}

TEST(TableConverterTest, ConvertsCharToDouble)
{
   std::vector<std::pair<char, double>> table = {
       {'a', 1.1},
       {'b', 2.2}
   };

   auto result = TableConverter::convert(table, 'a');
   ASSERT_TRUE(result.has_value());
   EXPECT_DOUBLE_EQ(result.value(), 1.1);
}

TEST(TableConverterTest, HandlesDuplicateKeys)
{
   std::vector<std::pair<int, std::string>> table = {
       {1, "first"},
       {1, "second"}
   };

   auto result = TableConverter::convert(table, 1);
   ASSERT_TRUE(result.has_value());
   EXPECT_EQ(result.value(), "first"); // Should return first match
}

TEST(TableConverterTest, ConvertsCustomType)
{
   struct Foo {
       int id;
       bool operator==(const Foo& other) const { return id == other.id; }
   };

   std::vector<std::pair<Foo, std::string>> table = {
       {Foo{1}, "one"},
       {Foo{2}, "two"}
   };

   auto result = TableConverter::convert(table, Foo{2});
   ASSERT_TRUE(result.has_value());
   EXPECT_EQ(result.value(), "two");
}

TEST(TableConverterReversedTest, ConvertsExistingValue)
{
   std::vector<std::pair<int, std::string>> table = {
       {1, "one"},
       {2, "two"},
       {3, "three"}
   };

   auto result = TableConverterReversed::convert(table, std::string("two"));
   ASSERT_TRUE(result.has_value());
   EXPECT_EQ(result.value(), 2);

   result = TableConverterReversed::convert(table, std::string("three"));
   ASSERT_TRUE(result.has_value());
   EXPECT_EQ(result.value(), 3);
}

TEST(TableConverterReversedTest, ReturnsNulloptForMissingValue)
{
   std::vector<std::pair<int, std::string>> table = {
       {1, "one"},
       {2, "two"}
   };

   auto result = TableConverterReversed::convert(table, std::string("four"));
   EXPECT_FALSE(result.has_value());
}

TEST(TableConverterReversedTest, WorksWithOtherTypes)
{
   std::vector<std::pair<char, double>> table = {
       {'a', 1.1},
       {'b', 2.2}
   };

   auto result = TableConverterReversed::convert(table, 2.2);
   ASSERT_TRUE(result.has_value());
   EXPECT_EQ(result.value(), 'b');
}
