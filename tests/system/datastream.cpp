#include <gtest/gtest.h>
#include <mint/system/datastream.h>

#include <string>

using namespace mint;

class TestStream : public DataStream {
public:
	explicit TestStream(const std::string &buffer) :
		m_buffer(buffer),
		m_pos(0) {}

	bool at_end() const override {
		return m_pos >= m_buffer.size();
	}

	bool is_valid() const override {
		return true;
	}

	std::string path() const override {
		return "test";
	}

protected:
	int read_char() override {
		if (m_pos < m_buffer.size()) {
			return m_buffer[m_pos++];
		}

		return EOF;
	}

	int next_buffered_char() override {
		return m_buffer[m_pos++];
	}

private:
	std::string m_buffer;
	size_t m_pos;
};

TEST(datastream, getChar) {

	TestStream stream("test");

	EXPECT_EQ('t', stream.get_char());
	EXPECT_EQ('e', stream.get_char());
	EXPECT_EQ('s', stream.get_char());
	EXPECT_EQ('t', stream.get_char());
}

TEST(datastream, setNewLineCallback) {

	TestStream stream(" \n \n\n\n\n\n");
	size_t lineNumber = 1;

	stream.set_new_line_callback([&lineNumber](size_t number) {
		lineNumber = number;
	});

	ASSERT_EQ(' ', stream.get_char());
	EXPECT_EQ(1, lineNumber);

	ASSERT_EQ('\n', stream.get_char());
	EXPECT_EQ(1, lineNumber);

	ASSERT_EQ(' ', stream.get_char());
	EXPECT_EQ(2, lineNumber);

	ASSERT_EQ('\n', stream.get_char());
	EXPECT_EQ(2, lineNumber);

	ASSERT_EQ('\n', stream.get_char());
	EXPECT_EQ(3, lineNumber);

	ASSERT_EQ('\n', stream.get_char());
	EXPECT_EQ(4, lineNumber);

	ASSERT_EQ('\n', stream.get_char());
	EXPECT_EQ(5, lineNumber);

	ASSERT_EQ('\n', stream.get_char());
	EXPECT_EQ(6, lineNumber);
}

TEST(datastream, lineNumber) {

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

TEST(datastream, lineError) {

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
