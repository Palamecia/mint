#include <gtest/gtest.h>
#include <system/bufferstream.h>

using namespace std;
using namespace mint;

TEST(bufferstream, getChar) {

	string buffer = "test1\ntest2";
	BufferStream stream(buffer);

	for (size_t i = 0; i < buffer.size(); ++i) {
		EXPECT_EQ(buffer[i], stream.getChar());
	}

	EXPECT_EQ('\n', stream.getChar());
	EXPECT_EQ(EOF, stream.getChar());
}

TEST(bufferstream, atEnd) {

	string buffer = "test1\ntest2\n";
	BufferStream stream(buffer);

	for (size_t i = 0; i < buffer.size(); ++i) {
		EXPECT_FALSE(stream.atEnd());
		stream.getChar();
	}

	EXPECT_FALSE(stream.atEnd());
	stream.getChar();

	EXPECT_TRUE(stream.atEnd());
}

TEST(bufferstream, isValid) {

	BufferStream stream("");
	EXPECT_TRUE(stream.isValid());
}

TEST(bufferstream, path) {

	BufferStream stream("");
	EXPECT_EQ("buffer", stream.path());
}
