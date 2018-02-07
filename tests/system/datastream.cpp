#include <gtest/gtest.h>
#include <system/datastream.h>

#include <string>

using namespace std;
using namespace mint;

class TestStream : public DataStream {
public:
	TestStream(const string &buffer) :
		m_buffer(buffer),
		m_pos(0) {

	}

	bool atEnd() const override {
		return m_pos >= m_buffer.size();
	}

	bool isValid() const override {
		return true;
	}

	string path() const override {
		return "test";
	}

protected:
	int readChar() override {
		if (m_pos < m_buffer.size()) {
			return m_buffer[m_pos++];
		}

		return EOF;
	}

	int nextBufferedChar() override {
		return m_buffer[m_pos++];
	}

private:
	string m_buffer;
	size_t m_pos;
};

TEST(datastream, getChar) {

	TestStream stream("test");

	EXPECT_EQ('t', stream.getChar());
	EXPECT_EQ('e', stream.getChar());
	EXPECT_EQ('s', stream.getChar());
	EXPECT_EQ('t', stream.getChar());
}

TEST(datastream, setLineEndCallback) {

	TestStream stream(" \n \n\n\n\n\n");
	size_t lineNumber = 0;

	stream.setLineEndCallback([&lineNumber](size_t number) {
		lineNumber = number;
	});

	stream.getChar();
	stream.getChar();
	EXPECT_EQ(1, lineNumber);

	stream.getChar();
	stream.getChar();
	EXPECT_EQ(2, lineNumber);

	stream.getChar();
	stream.getChar();
	EXPECT_EQ(4, lineNumber);

	stream.getChar();
	stream.getChar();
	EXPECT_EQ(6, lineNumber);
}

TEST(datastream, lineNumber) {
	/// \todo
}

TEST(datastream, lineError) {
	/// \todo
}
