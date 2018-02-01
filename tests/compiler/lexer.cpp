#include <gtest/gtest.h>
#include <compiler/lexer.h>

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

TEST(lexer, nextToken) {

	TestStream stream("test test2+test3 + loadtest4 4.5 6..7 'with white space'");
	Lexer lexer(&stream);

	EXPECT_EQ("test", lexer.nextToken());
	EXPECT_EQ("test2", lexer.nextToken());
	EXPECT_EQ("+", lexer.nextToken());
	EXPECT_EQ("test3", lexer.nextToken());
	EXPECT_EQ("+", lexer.nextToken());
	EXPECT_EQ("loadtest4", lexer.nextToken());
	EXPECT_EQ("4.5", lexer.nextToken());
	EXPECT_EQ("6", lexer.nextToken());
	EXPECT_EQ("..", lexer.nextToken());
	EXPECT_EQ("7", lexer.nextToken());
	EXPECT_EQ("'with white space'", lexer.nextToken());
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
