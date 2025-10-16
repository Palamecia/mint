#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include "mint/system/datastream.h"

#include <string>
#include <utility>

class TestStream : public mint::DataStream {
public:
	explicit TestStream(std::string buffer) :
	    _buffer(std::move(buffer)) {}

	[[nodiscard]] bool at_end() const override {
		return _pos >= _buffer.size();
	}

	[[nodiscard]] bool is_valid() const override {
		return true;
	}

	[[nodiscard]] std::filesystem::path path() const override {
		return "test";
	}

protected:
	int read_char() override {
		if (_pos < _buffer.size()) {
			return _buffer[_pos++];
		}

		return EOF;
	}

	int next_buffered_char() override {
		return _buffer[_pos++];
	}

private:
	std::string _buffer;
	std::size_t _pos = 0;
};

TEST(datastream, get_char) {

	TestStream stream("test");

	EXPECT_EQ('t', stream.get_char());
	EXPECT_EQ('e', stream.get_char());
	EXPECT_EQ('s', stream.get_char());
	EXPECT_EQ('t', stream.get_char());
}

TEST(datastream, set_new_line_callback) {

	TestStream stream(" \n \n\n\n\n\n");
	std::size_t line_number = 1;

	stream.set_new_line_callback([&line_number](std::size_t number) {
		line_number = number;
	});

	ASSERT_EQ(' ', stream.get_char());
	EXPECT_EQ(1, line_number);

	ASSERT_EQ('\n', stream.get_char());
	EXPECT_EQ(1, line_number);

	ASSERT_EQ(' ', stream.get_char());
	EXPECT_EQ(2, line_number);

	ASSERT_EQ('\n', stream.get_char());
	EXPECT_EQ(2, line_number);

	ASSERT_EQ('\n', stream.get_char());
	EXPECT_EQ(3, line_number);

	ASSERT_EQ('\n', stream.get_char());
	EXPECT_EQ(4, line_number);

	ASSERT_EQ('\n', stream.get_char());
	EXPECT_EQ(5, line_number);

	ASSERT_EQ('\n', stream.get_char());
	EXPECT_EQ(6, line_number);
}

TEST(datastream, line_number) {

	TestStream stream(" \n \n\n\n\n\n");

	EXPECT_EQ(1, stream.line_number());

	stream.get_char();
	stream.get_char();
	EXPECT_EQ(2, stream.line_number());

	stream.get_char();
	stream.get_char();
	EXPECT_EQ(3, stream.line_number());

	stream.get_char();
	stream.get_char();
	EXPECT_EQ(5, stream.line_number());

	stream.get_char();
	stream.get_char();
	EXPECT_EQ(7, stream.line_number());
}

TEST(datastream, line_error) {

	TestStream stream1("line error test\n");
	stream1.get_char();
	EXPECT_EQ("line error test\n^", stream1.line_error());

	TestStream stream2("line error test\n");
	stream2.get_char();
	stream2.get_char();
	stream2.get_char();
	stream2.get_char();
	stream2.get_char();
	EXPECT_EQ("line error test\n   ^", stream2.line_error());

	TestStream stream3("\t\t  line error test\n");
	stream3.get_char();
	stream3.get_char();
	stream3.get_char();
	stream3.get_char();
	stream3.get_char();
	stream3.get_char();
	EXPECT_EQ("\t\t  line error test\n\t\t  ^", stream3.line_error());
}
