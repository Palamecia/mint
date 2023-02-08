#include <gtest/gtest.h>
#include <mint/system/utf8.h>

#include <iostream>

using namespace std;
using namespace mint;

TEST(utf8iterator, utf8_begin_code_point) {

	for (byte_t b = 0x00; b <= 0x7F; ++b) {
		EXPECT_TRUE(utf8_begin_code_point(b)) << hex << static_cast<int>(b);
	}

	/// \todo Check others bytes
}

TEST(utf8iterator, utf8_code_point_length) {

	for (byte_t b = 0x00; b <= 0x7F; ++b) {
		EXPECT_EQ(1, utf8_code_point_length(b)) << hex << static_cast<int>(b);
	}

	/// \todo Check others bytes
}

TEST(utf8iterator, utf8_code_point_count) {

	EXPECT_EQ(4, utf8_code_point_count("test"));
	EXPECT_EQ(4, utf8_code_point_count("tëst"));
}
