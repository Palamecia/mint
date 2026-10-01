#include <cstddef>
#include <cstdio>
#include <gtest/gtest.h>
#include "mint/compiler/lexical_handler.h"
#include "mint/compiler/token.h"

#include <utility>
#include <string>
#include <string_view>
#include <vector>

namespace {

class SymbolCaptureHandler : public mint::LexicalHandler {
public:
	[[nodiscard]] const std::vector<std::pair<std::vector<std::string>, std::string>>& symbols() const {
		return _symbols;
	}

protected:
	bool on_module_path_token(const std::vector<std::string>& context, const std::string& token,
	    [[maybe_unused]] std::string::size_type offset) override {
		_symbols.emplace_back(context, token);
		return true;
	}

	bool on_symbol_token(const std::vector<std::string>& context, const std::string& token,
	    [[maybe_unused]] std::string::size_type offset) override {
		_symbols.emplace_back(context, token);
		return true;
	}

private:
	std::vector<std::pair<std::vector<std::string>, std::string>> _symbols;
};

class CommentCaptureHandler : public mint::LexicalHandler {
public:
	struct Comment {
		std::string token;
		std::vector<std::string> parts;
		std::string::size_type begin = 0;
		std::string::size_type end = 0;
	};

	[[nodiscard]] const std::vector<Comment>& comments() const {
		return _comments;
	}

protected:
	bool on_comment_begin(std::string::size_type offset) override {
		_comments.emplace_back(Comment {
		    .begin = offset,
		    .end = offset,
		});
		return true;
	}

	bool on_comment(const std::string& token, std::string::size_type /*offset*/) override {
		if (_comments.empty()) {
			return false;
		}
		_comments.back().token += token;
		_comments.back().parts.push_back(token);
		return true;
	}

	bool on_comment_end(std::string::size_type offset) override {
		if (_comments.empty()) {
			return false;
		}
		_comments.back().end = offset;
		return true;
	}

	bool on_token(mint::Token type, const std::string& token, std::string::size_type offset) override {
		return type != mint::Token::comment_token
		       || (!_comments.empty() && _comments.back().token == token && _comments.back().begin == offset);
	}

private:
	std::vector<Comment> _comments;
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

}

TEST(lexical_handler, module_path_symbols) {

	auto handler = SymbolCaptureHandler();
	auto stream = LexicalHandlerStream("load test.module.path");

	ASSERT_TRUE(handler.parse(stream));
	ASSERT_EQ(handler.symbols().size(), 5u);

	EXPECT_EQ(handler.symbols().at(0), std::make_pair(std::vector<std::string> {}, std::string {"test"}));
	EXPECT_EQ(handler.symbols().at(1), std::make_pair(std::vector<std::string> {"test"}, std::string {"."}));
	EXPECT_EQ(handler.symbols().at(2), std::make_pair(std::vector<std::string> {"test", "."}, std::string {"module"}));
	EXPECT_EQ(handler.symbols().at(3),
	    std::make_pair(std::vector<std::string> {"test", ".", "module"}, std::string {"."}));
	EXPECT_EQ(handler.symbols().at(4),
	    std::make_pair(std::vector<std::string> {"test", ".", "module", "."}, std::string {"path"}));
}

TEST(lexical_handler, enum_member_symbols) {

	auto handler = SymbolCaptureHandler();
	auto stream = LexicalHandlerStream(R"(
        enum Test {
            A
            B
            C
        }
    )");

	ASSERT_TRUE(handler.parse(stream));
	ASSERT_EQ(handler.symbols().size(), 4u);

	EXPECT_EQ(handler.symbols().at(0), std::make_pair(std::vector<std::string> {}, std::string {"Test"}));
	EXPECT_EQ(handler.symbols().at(1), std::make_pair(std::vector<std::string> {}, std::string {"A"}));
	EXPECT_EQ(handler.symbols().at(2), std::make_pair(std::vector<std::string> {}, std::string {"B"}));
	EXPECT_EQ(handler.symbols().at(3), std::make_pair(std::vector<std::string> {}, std::string {"C"}));
}

TEST(lexical_handler, comments) {

	constexpr std::string_view script = "// line comment\n#! shebang comment\n/* block comment */";
	auto handler = CommentCaptureHandler {};
	auto stream = LexicalHandlerStream(std::string {script});

	ASSERT_TRUE(handler.parse(stream));
	ASSERT_EQ(handler.comments().size(), 3u);

	EXPECT_EQ(handler.comments().at(0).token, "// line comment");
	EXPECT_EQ(handler.comments().at(0).begin, 0u);
	EXPECT_EQ(handler.comments().at(0).end, std::string {"// line comment"}.size());

	EXPECT_EQ(handler.comments().at(1).token, "#! shebang comment");
	EXPECT_EQ(handler.comments().at(1).begin, std::string {"// line comment\n"}.size());
	EXPECT_EQ(handler.comments().at(1).end, std::string {"// line comment\n#! shebang comment"}.size());

	EXPECT_EQ(handler.comments().at(2).token, "/* block comment */");
	EXPECT_EQ(handler.comments().at(2).begin, std::string {"// line comment\n#! shebang comment\n"}.size());
	EXPECT_EQ(handler.comments().at(2).end, script.size());
}

TEST(lexical_handler, comment_lines) {

	constexpr std::string_view script = "/* comment\non multiple\nlines */";
	auto handler = CommentCaptureHandler {};
	auto stream = LexicalHandlerStream(std::string {script});

	ASSERT_TRUE(handler.parse(stream));
	ASSERT_EQ(handler.comments().size(), 1u);
	ASSERT_EQ(handler.comments().at(0).parts.size(), 3u);

	EXPECT_EQ(handler.comments().at(0).parts.at(0), "/* comment\n");
	EXPECT_EQ(handler.comments().at(0).parts.at(1), "on multiple\n");
	EXPECT_EQ(handler.comments().at(0).parts.at(2), "lines */");
	EXPECT_EQ(handler.comments().at(0).begin, 0u);
	EXPECT_EQ(handler.comments().at(0).end, script.size());
}
