#include <gtest/gtest.h>
#include <mint/system/bufferstream.h>

using namespace mint;

TEST(bufferstream, get_char) {

	std::string buffer = "test1\ntest2";
	BufferStream stream(buffer);

	for (char c : buffer) {
		EXPECT_EQ(c, stream.get_char());
	}

	EXPECT_EQ('\n', stream.get_char());
	EXPECT_EQ(EOF, stream.get_char());
}

TEST(bufferstream, at_end) {

	std::string buffer = "test1\ntest2\n";
	BufferStream stream(buffer);

	for (char c : buffer) {
		EXPECT_FALSE(stream.at_end());
		EXPECT_EQ(c, stream.get_char());
	}

	EXPECT_FALSE(stream.at_end());
	EXPECT_EQ('\n', stream.get_char());

	EXPECT_FALSE(stream.at_end());
	EXPECT_EQ(EOF, stream.get_char());

	EXPECT_TRUE(stream.at_end());
}

TEST(bufferstream, is_valid) {

	BufferStream stream("");
	EXPECT_TRUE(stream.is_valid());
}

TEST(bufferstream, path) {

	BufferStream stream("");
	EXPECT_EQ("buffer", stream.path());
}
