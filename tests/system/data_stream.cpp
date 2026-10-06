#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include "mint/system/data_stream.h"
#include "mint/system/buffer_stream.h"

#include <string>
#include <utility>

namespace {

class TestStream : public mint::DataStream {
public:
	explicit TestStream(std::string buffer) :
	    _buffer(std::move(buffer)) {}

	[[nodiscard]] bool at_end() const override {
		return _pos >= _buffer.size();
	}

	[[nodiscard]] std::string get_line(std::size_t line_number) override {
		if (line_number == 0) {
			return {};
		}
		auto start = std::size_t {0};
		for (auto line = std::size_t {1}; line < line_number; ++line) {
			const auto newline = _buffer.find('\n', start);
			if (newline == std::string::npos) {
				return {};
			}
			start = newline + 1;
		}
		const auto newline = _buffer.find('\n', start);
		return _buffer.substr(start, newline == std::string::npos ? newline : newline - start);
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

private:
	std::string _buffer;
	std::size_t _pos = 0;
};

}

TEST(data_stream, get_char) {

	TestStream stream("test");

	EXPECT_EQ('t', stream.get_char());
	EXPECT_EQ('e', stream.get_char());
	EXPECT_EQ('s', stream.get_char());
	EXPECT_EQ('t', stream.get_char());
}

TEST(data_stream, set_new_line_callback) {

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

TEST(data_stream, line_number) {

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

/*TEST(data_stream, line_error) {

	TestStream stream1("line error test\n");
	stream1.get_char();
	EXPECT_EQ("line error test\n^", stream1.line_error());
	EXPECT_EQ(1, stream1.line_number());
	EXPECT_EQ('i', stream1.get_char());

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

TEST(data_stream, line_error_uses_saved_position) {
	TestStream stream("first line\nsecond line\n");
	for (int i = 0; i < 4; ++i) {
		stream.get_char();
	}
	const auto position = stream.position();
	while (stream.get_char() != EOF) {
	}

	EXPECT_EQ(1, position.line);
	EXPECT_EQ("first line\n  ^", stream.line_error(position));
}*/

TEST(data_stream, get_line) {
	TestStream stream("first line\nsecond line\n");

	EXPECT_EQ("first line", stream.get_line(1));
	EXPECT_EQ("second line", stream.get_line(2));
	EXPECT_EQ("", stream.get_line(3));
}

TEST(data_stream, buffer_stream_get_line) {
	mint::BufferStream stream("first line\nsecond line");

	EXPECT_EQ("first line", stream.get_line(1));
	EXPECT_EQ("second line", stream.get_line(2));
	EXPECT_EQ("", stream.get_line(3));
}
