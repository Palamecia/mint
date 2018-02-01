#include <gtest/gtest.h>
#include <system/utf8iterator.h>

#include <iostream>

using namespace std;
using namespace mint;

TEST(utf8iterator, utf8char_valid) {

	for (byte b = 0x00; b <= 0x7F; ++b) {
		EXPECT_TRUE(utf8char_valid(b)) << hex << static_cast<int>(b);
	}

	/// \todo Check others bytes
}

TEST(utf8iterator, utf8char_length) {

	for (byte b = 0x00; b <= 0x7F; ++b) {
		EXPECT_EQ(1, utf8char_length(b)) << hex << static_cast<int>(b);
	}

	/// \todo Check others bytes
}

TEST(utf8iterator, utf8length) {

	EXPECT_EQ(4, utf8length("test"));
	EXPECT_EQ(4, utf8length("tëst"));
}
