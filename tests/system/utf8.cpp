#include <cstdint>
#include <gtest/gtest.h>
#include "mint/system/utf8.h"

#include <iostream>

TEST(utf8iterator, utf8_begin_code_point) {

	for (std::uint8_t b = 0x00; b <= 0x7F; ++b) {
		EXPECT_TRUE(mint::utf8_begin_code_point(b)) << std::hex << static_cast<int>(b);
	}

	/// \todo Check others bytes
}

TEST(utf8iterator, utf8_code_point_length) {

	for (std::uint8_t b = 0x00; b <= 0x7F; ++b) {
		EXPECT_EQ(1, mint::utf8_code_point_length(b)) << std::hex << static_cast<int>(b);
	}

	/// \todo Check others bytes
}

TEST(utf8iterator, utf8_code_point_count) {

	EXPECT_EQ(4, mint::utf8_code_point_count("test"));
	EXPECT_EQ(4, mint::utf8_code_point_count("tëst"));
}
