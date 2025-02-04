#include <cstddef>
#include <gtest/gtest.h>
#include <mint/compiler/lexicalhandler.h>

#include <utility>
#include <string>
#include <vector>

class SymbolCaptureHandler : public mint::LexicalHandler {
public:
	SymbolCaptureHandler(std::vector<std::pair<std::vector<std::string>, std::string>> *capture) :
		m_capture(capture) {}

	bool on_module_path_token(const std::vector<std::string> &context, const std::string &token,
							  [[maybe_unused]] std::string::size_type offset) override {
		m_capture->emplace_back(context, token);
		return true;
	}

	bool on_symbol_token(const std::vector<std::string> &context, const std::string &token,
						 [[maybe_unused]]std::string::size_type offset) override {
		m_capture->emplace_back(context, token);
		return true;
	}

private:
	std::vector<std::pair<std::vector<std::string>, std::string>> *m_capture;
};

class LexicalHandlerStream : public mint::AbstractLexicalHandlerStream {
public:
	explicit LexicalHandlerStream(std::string buffer) :
		m_buffer(std::move(buffer)) {}

	[[nodiscard]] bool at_end() const override {
		return !m_good;
	}

	[[nodiscard]] bool is_valid() const override {
		return m_good;
	}

protected:
	int get() override {
		if (m_pos < m_buffer.size()) {
			return m_buffer[m_pos++];
		}
		m_good = false;
		return EOF;
	}

private:
	std::string m_buffer;
	bool m_good = true;
	size_t m_pos = 0;
};

TEST(lexicalhandler, module_path_symbols) {

	std::vector<std::pair<std::vector<std::string>, std::string>> capture;
	SymbolCaptureHandler handler(&capture);
	LexicalHandlerStream stream("load test.module.path");

	ASSERT_TRUE(handler.parse(stream));
	ASSERT_EQ(5u, capture.size());

	EXPECT_EQ(std::make_pair(std::vector<std::string> {}, std::string {"test"}), capture[0]);
	EXPECT_EQ(std::make_pair(std::vector<std::string> {"test"}, std::string {"."}), capture[1]);
	EXPECT_EQ(std::make_pair(std::vector<std::string> {"test", "."}, std::string {"module"}), capture[2]);
	EXPECT_EQ(std::make_pair(std::vector<std::string> {"test", ".", "module"}, std::string {"."}), capture[3]);
	EXPECT_EQ(std::make_pair(std::vector<std::string> {"test", ".", "module", "."}, std::string {"path"}), capture[4]);
}

TEST(lexicalhandler, enum_member_symbols) {

	std::vector<std::pair<std::vector<std::string>, std::string>> capture;
	SymbolCaptureHandler handler(&capture);
	LexicalHandlerStream stream(R"(
        enum Test {
            A
            B
            C
        }
    )");

	ASSERT_TRUE(handler.parse(stream));
	ASSERT_EQ(4u, capture.size());

	EXPECT_EQ(std::make_pair(std::vector<std::string> {}, std::string {"Test"}), capture[0]);
	EXPECT_EQ(std::make_pair(std::vector<std::string> {}, std::string {"A"}), capture[1]);
	EXPECT_EQ(std::make_pair(std::vector<std::string> {}, std::string {"B"}), capture[2]);
	EXPECT_EQ(std::make_pair(std::vector<std::string> {}, std::string {"C"}), capture[3]);
}
