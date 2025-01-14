#include <gtest/gtest.h>
#include <mint/system/bufferstream.h>

using namespace mint;

TEST(bufferstream, getChar) {

	std::string buffer = "test1\ntest2";
	BufferStream stream(buffer);

	for (size_t i = 0; i < buffer.size(); ++i) {
		EXPECT_EQ(buffer[i], stream.get_char());
	}

	EXPECT_EQ('\n', stream.get_char());
	EXPECT_EQ(EOF, stream.get_char());
}

TEST(bufferstream, atEnd) {

	std::string buffer = "test1\ntest2\n";
	BufferStream stream(buffer);

	for (size_t i = 0; i < buffer.size(); ++i) {
		EXPECT_FALSE(stream.at_end());
		EXPECT_EQ(buffer[i], stream.get_char());
	}

	EXPECT_FALSE(stream.at_end());
	EXPECT_EQ('\n', stream.get_char());

	EXPECT_FALSE(stream.at_end());
	EXPECT_EQ(EOF, stream.get_char());

	EXPECT_TRUE(stream.at_end());
}

TEST(bufferstream, isValid) {

	BufferStream stream("");
	EXPECT_TRUE(stream.is_valid());
}

TEST(bufferstream, path) {

	BufferStream stream("");
	EXPECT_EQ("buffer", stream.path());
}
