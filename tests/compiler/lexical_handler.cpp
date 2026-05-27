#include <cstddef>
#include <cstdio>
#include <gtest/gtest.h>
#include "mint/compiler/lexical_handler.h"

#include <utility>
#include <string>
#include <vector>

class SymbolCaptureHandler : public mint::LexicalHandler {
public:
	SymbolCaptureHandler(std::vector<std::pair<std::vector<std::string>, std::string>>* capture) :
	    _capture(capture) {}

	bool on_module_path_token(const std::vector<std::string>& context, const std::string& token,
	    [[maybe_unused]] std::string::size_type offset) override {
		_capture->emplace_back(context, token);
		return true;
	}

	bool on_symbol_token(const std::vector<std::string>& context, const std::string& token,
	    [[maybe_unused]] std::string::size_type offset) override {
		_capture->emplace_back(context, token);
		return true;
	}

private:
	std::vector<std::pair<std::vector<std::string>, std::string>>* _capture;
};

class LexicalHandlerStream : public mint::AbstractLexicalHandlerStream {
public:
	explicit LexicalHandlerStream(std::string buffer) :
	    _buffer(std::move(buffer)) {}

	[[nodiscard]] bool at_end() const override {
		return !_good;
	}

	[[nodiscard]] bool is_valid() const override {
		return _good;
	}

protected:
	int get() override {
		if (_pos < _buffer.size()) {
			return _buffer[_pos++];
		}
		_good = false;
		return EOF;
	}

private:
	std::string _buffer;
	bool _good = true;
	std::size_t _pos = 0;
};

TEST(lexical_handler, module_path_symbols) {

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

TEST(lexical_handler, enum_member_symbols) {

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
