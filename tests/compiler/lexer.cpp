#include <gtest/gtest.h>
#include <mint/compiler/lexer.h>
#include <mint/system/bufferstream.h>

using namespace mint;

TEST(lexer, nextToken) {

	BufferStream stream("test test2+test3 + loadtest4 4.5 6..7 'with white space'");
	Lexer lexer(&stream);

	EXPECT_EQ("test", lexer.next_token());
	EXPECT_EQ("test2", lexer.next_token());
	EXPECT_EQ("+", lexer.next_token());
	EXPECT_EQ("test3", lexer.next_token());
	EXPECT_EQ("+", lexer.next_token());
	EXPECT_EQ("loadtest4", lexer.next_token());
	EXPECT_EQ("4.5", lexer.next_token());
	EXPECT_EQ("6", lexer.next_token());
	EXPECT_EQ("..", lexer.next_token());
	EXPECT_EQ("7", lexer.next_token());
	EXPECT_EQ("'with white space'", lexer.next_token());
}

TEST(lexer, tokenType) {

	/// \todo
}

TEST(lexer, formatError) {

	/// \todo
}

TEST(lexer, atEnd) {

	/// \todo
}

TEST(datastream, setNewLineCallback) {

	BufferStream stream(R"""(/* comment */

load module

if defined symbol {
	func()
}
)""");

	size_t lineNumber = 1;
	stream.set_new_line_callback([&lineNumber](size_t number) {
		lineNumber = number;
	});

	Lexer lexer(&stream);

	ASSERT_EQ("\n", lexer.next_token());
	EXPECT_EQ(2, lineNumber);

	ASSERT_EQ("\n", lexer.next_token());
	EXPECT_EQ(3, lineNumber);

	ASSERT_EQ("load", lexer.next_token());
	EXPECT_EQ(3, lineNumber);

	ASSERT_EQ("module", lexer.next_token());
	EXPECT_EQ(3, lineNumber);

	ASSERT_EQ("\n", lexer.next_token());
	EXPECT_EQ(4, lineNumber);

	ASSERT_EQ("\n", lexer.next_token());
	EXPECT_EQ(5, lineNumber);

	ASSERT_EQ("if", lexer.next_token());
	EXPECT_EQ(5, lineNumber);

	ASSERT_EQ("defined", lexer.next_token());
	EXPECT_EQ(5, lineNumber);

	ASSERT_EQ("symbol", lexer.next_token());
	EXPECT_EQ(5, lineNumber);

	ASSERT_EQ("{", lexer.next_token());
	EXPECT_EQ(5, lineNumber);

	ASSERT_EQ("\n", lexer.next_token());
	EXPECT_EQ(6, lineNumber);

	ASSERT_EQ("func", lexer.next_token());
	EXPECT_EQ(6, lineNumber);

	ASSERT_EQ("(", lexer.next_token());
	EXPECT_EQ(6, lineNumber);

	ASSERT_EQ(")", lexer.next_token());
	EXPECT_EQ(6, lineNumber);

	ASSERT_EQ("\n", lexer.next_token());
	EXPECT_EQ(7, lineNumber);

	ASSERT_EQ("}", lexer.next_token());
	EXPECT_EQ(7, lineNumber);

	ASSERT_EQ("\n", lexer.next_token());
	EXPECT_EQ(8, lineNumber);
}
