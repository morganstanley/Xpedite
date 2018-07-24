///////////////////////////////////////////////////////////////////////////////////////////////
//
// Test conversion of hex ascii strings to numbers
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/util/Util.H>
#include <gtest/gtest.h>
#include <limits>
#include <stdlib.h>

namespace xpedite { namespace util { namespace test {

  struct AtoiHexTest : ::testing::Test
  {
  };

  inline bool isValidValue(uint8_t value_) {
    return (value_ >= '0' && value_ <= '9')
        || (value_ >= 'A' && value_ <= 'F')
        || (value_ >= 'a' && value_ <= 'f');
  }

  inline std::tuple<uint8_t, bool> buildExpectedValue(const char* valueStr_) {
    if(isValidValue(valueStr_[0]) && isValidValue(valueStr_[1])) {
      errno = {};
      uint8_t expectedValue {static_cast<uint8_t>(strtol(valueStr_, nullptr, 16))};
      bool expectedValidity {errno != EINVAL};
      return std::make_tuple(expectedValue, expectedValidity);
    }
    return {};
  }

  TEST_F(AtoiHexTest, validate) {

    for(uint8_t i=0; i<std::numeric_limits<uint8_t>::max(); ++i) {
      for(uint8_t j=0; j<std::numeric_limits<uint8_t>::max(); ++j) {
        const char valueStr [] {static_cast<char>(i), static_cast<char>(j), '\0'};

        uint8_t expectedValue; bool expectedValidity;
        std::tie(expectedValue, expectedValidity) = buildExpectedValue(valueStr);

        uint8_t value; bool isValid;
        std::tie(value, isValid) = atoiHex(valueStr);

        ASSERT_EQ(isValid, expectedValidity) << "failed to detect validity of value - " << valueStr;
        ASSERT_EQ(expectedValue, value) << "failed to decode value - " << valueStr;
      }
    }
  }

}}}
