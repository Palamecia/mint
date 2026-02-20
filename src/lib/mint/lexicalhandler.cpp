/**
 * Copyright (c) 2026 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "mint/ast/symbol.h"
#include "mint/compiler/token.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"
#include <cstddef>
#include <cstdio>
#include <functional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>
#include "mint/compiler/lexicalhandler.h"

namespace symbols {

static const mint::Symbol lexical_handler("LexicalHandler");
static const mint::Symbol token("Token");
static const mint::Symbol on_script_begin("onScriptBegin");
static const mint::Symbol on_script_end("onScriptEnd");
static const mint::Symbol on_comment_begin("onCommentBegin");
static const mint::Symbol on_comment_end("onCommentEnd");
static const mint::Symbol on_module_path_token("onModulePathToken");
static const mint::Symbol on_symbol_token("onSymbolToken");
static const mint::Symbol on_token("onToken");
static const mint::Symbol on_white_space("onWhiteSpace");
static const mint::Symbol on_comment("onComment");
static const mint::Symbol on_new_line("onNewLine");
static const mint::Symbol read_char("readChar");

}

namespace {

class MintLexicalHandler : public mint::LexicalHandler {
public:
	explicit MintLexicalHandler(mint::Scheduler& scheduler, const mint::Reference& self) :
	    _scheduler(scheduler),
	    _lexical_handler_class(
	        mint::get_member_ignore_visibility(scheduler.ast().global_data(), symbols::lexical_handler)),
	    _self(self) {}

protected:
	bool on_script_begin() override {
		return to_boolean(_scheduler.get().invoke(_self, symbols::on_script_begin));
	}

	bool on_script_end() override {
		return to_boolean(_scheduler.get().invoke(_self, symbols::on_script_end));
	}

	bool on_comment_begin(std::string::size_type offset) override {
		return to_boolean(
		    _scheduler.get().invoke(_self, symbols::on_comment_begin, mint::create_unsigned_number(offset)));
	}

	bool on_comment_end(std::string::size_type offset) override {
		return to_boolean(_scheduler.get().invoke(_self, symbols::on_comment_end, mint::create_unsigned_number(offset)));
	}

	bool on_module_path_token(const std::vector<std::string>& context, const std::string& token,
	    std::string::size_type offset) override {
		mint::WeakReference context_values = mint::create_array(_scheduler.get().ast(),
		    {std::from_range, std::views::transform(context, [this](const std::string& context_symbol) {
			     return mint::create_string(_scheduler.get().ast(), context_symbol);
		     })});
		return to_boolean(_scheduler.get().invoke(_self, symbols::on_module_path_token, std::move(context_values),
		    mint::create_string(_scheduler.get().ast(), token), mint::create_unsigned_number(offset)));
	}

	bool on_symbol_token(const std::vector<std::string>& context, const std::string& token,
	    std::string::size_type offset) override {
		mint::WeakReference context_values = mint::create_array(_scheduler.get().ast(),
		    {std::from_range, std::views::transform(context, [this](const std::string& context_symbol) {
			     return mint::create_string(_scheduler.get().ast(), context_symbol);
		     })});
		return to_boolean(_scheduler.get().invoke(_self, symbols::on_symbol_token, std::move(context_values),
		    mint::create_string(_scheduler.get().ast(), token), mint::create_unsigned_number(offset)));
	}

	bool on_symbol_token(const std::vector<std::string>& context, std::string::size_type offset) override {
		mint::WeakReference context_values = mint::create_array(_scheduler.get().ast(),
		    {std::from_range, std::views::transform(context, [this](const std::string& context_symbol) {
			     return mint::create_string(_scheduler.get().ast(), context_symbol);
		     })});
		return to_boolean(_scheduler.get().invoke(_self, symbols::on_symbol_token, std::move(context_values),
		    mint::create_unsigned_number(offset)));
	}

	bool on_token(mint::Token type, const std::string& token, std::string::size_type offset) override {
		const auto token_value = mint::get_global_ignore_visibility(_lexical_handler_class.data<mint::Object>(),
		    symbols::token);
		return to_boolean(_scheduler.get().invoke(_self, symbols::on_token,
		    find_enum_value(token_value.data<mint::Object>(), static_cast<double>(type)),
		    mint::create_string(_scheduler.get().ast(), token), mint::create_unsigned_number(offset)));
	}

	bool on_white_space(const std::string& token, std::string::size_type offset) override {
		return to_boolean(_scheduler.get().invoke(_self, symbols::on_white_space,
		    mint::create_string(_scheduler.get().ast(), token), mint::create_unsigned_number(offset)));
	}

	bool on_comment(const std::string& token, std::string::size_type offset) override {
		return to_boolean(_scheduler.get().invoke(_self, symbols::on_comment,
		    mint::create_string(_scheduler.get().ast(), token), mint::create_unsigned_number(offset)));
	}

	bool on_new_line(std::size_t line_number, std::string::size_type offset) override {
		return to_boolean(_scheduler.get().invoke(_self, symbols::on_new_line,
		    mint::create_unsigned_number(line_number), mint::create_unsigned_number(offset)));
	}

private:
	std::reference_wrapper<mint::Scheduler> _scheduler;
	mint::WeakReference _lexical_handler_class;
	mint::WeakReference _self;
};

class LexicalHandlerStream : public mint::AbstractLexicalHandlerStream {
public:
	explicit LexicalHandlerStream(mint::Scheduler& scheduler, const mint::Reference& self) :
	    _scheduler(scheduler),
	    _self(self) {}

	[[nodiscard]] bool at_end() const override {
		return !_good;
	}

	[[nodiscard]] bool is_valid() const override {
		return _good;
	}

protected:
	int get() override {
		if (_buffer.empty()) {
			const auto result = _scheduler.get().invoke(_self, symbols::read_char);
			if (is_instance_of(result, mint::Data::Format::none)) {
				_good = false;
				return EOF;
			}
			const auto buffer = to_string(result);
			for (const char ch : std::views::reverse(buffer)) {
				_buffer.push_back(static_cast<int>(ch));
			}
		}
		const int c = _buffer.back();
		_buffer.pop_back();
		return c;
	}

private:
	std::reference_wrapper<mint::Scheduler> _scheduler;
	mint::WeakReference _self;
	std::vector<int> _buffer;
	bool _good = true;
};

mint::WeakReference mint_lexical_handler_new(mint::FunctionHelper& helper, const mint::Reference& self) {
	return mint::create_c_object(helper.cursor().ast(), new MintLexicalHandler(helper.scheduler(), self));
}

mint::WeakReference mint_lexical_handler_delete(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	delete self.data<mint::LibObject<MintLexicalHandler>>().ptr;
	return {};
}

mint::WeakReference mint_lexical_handler_parse(mint::FunctionHelper& helper, const mint::Reference& self,
    mint::Reference& stream) {
	auto handler_stream = LexicalHandlerStream(helper.scheduler(), stream);
	return mint::create_boolean(self.data<mint::LibObject<MintLexicalHandler>>().ptr->parse(handler_stream));
}

}

MINT_EXPORT_FUNCTION(mint_lexical_handler_new, 1);
MINT_EXPORT_FUNCTION(mint_lexical_handler_delete, 1);
MINT_EXPORT_FUNCTION(mint_lexical_handler_parse, 2);
