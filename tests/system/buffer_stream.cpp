#include <cstdio>
#include <gtest/gtest.h>
#include <string>
#include "mint/system/buffer_stream.h"

TEST(buffer_stream, get_char) {

	const auto buffer = std::string("test1\ntest2");
	mint::BufferStream stream(buffer);

	for (const char c : buffer) {
		EXPECT_EQ(c, stream.get_char());
	}

	EXPECT_EQ('\n', stream.get_char());
	EXPECT_EQ(EOF, stream.get_char());
}

TEST(buffer_stream, at_end) {

	const auto buffer = std::string("test1\ntest2\n");
	mint::BufferStream stream(buffer);

	for (const char c : buffer) {
		EXPECT_FALSE(stream.at_end());
		EXPECT_EQ(c, stream.get_char());
	}

	EXPECT_FALSE(stream.at_end());
	EXPECT_EQ('\n', stream.get_char());

	EXPECT_FALSE(stream.at_end());
	EXPECT_EQ(EOF, stream.get_char());

	EXPECT_TRUE(stream.at_end());
}

TEST(buffer_stream, is_valid) {

	const auto stream = mint::BufferStream("");
	EXPECT_TRUE(stream.is_valid());
}

TEST(buffer_stream, path) {

	const auto stream = mint::BufferStream("");
	EXPECT_EQ("buffer", stream.path());
}
