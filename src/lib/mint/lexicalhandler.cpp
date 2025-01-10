/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/scheduler/scheduler.h"
#include "mint/compiler/lexicalhandler.h"

using namespace mint;

namespace symbols {
static const Symbol LexicalHandler("LexicalHandler");
static const Symbol Token("Token");
static const Symbol onScriptBegin("onScriptBegin");
static const Symbol onScriptEnd("onScriptEnd");
static const Symbol onCommentBegin("onCommentBegin");
static const Symbol onCommentEnd("onCommentEnd");
static const Symbol onModulePathToken("onModulePathToken");
static const Symbol onSymbolToken("onSymbolToken");
static const Symbol onToken("onToken");
static const Symbol onWhiteSpace("onWhiteSpace");
static const Symbol onComment("onComment");
static const Symbol onNewLine("onNewLine");
static const Symbol readChar("readChar");
}

class MintLexicalHandler : public LexicalHandler {
public:
	explicit MintLexicalHandler(Reference &self) :
		m_lexicalHandlerClass(get_member_ignore_visibility(GlobalData::instance(), symbols::LexicalHandler)),
		m_self(std::move(self)) {

	}

protected:
	bool on_script_begin() override {
		return to_boolean(Scheduler::instance()->current_process()->cursor(), Scheduler::instance()->invoke(m_self, symbols::onScriptBegin));
	}

	bool on_script_end() override {
		return to_boolean(Scheduler::instance()->current_process()->cursor(), Scheduler::instance()->invoke(m_self, symbols::onScriptEnd));
	}

	bool on_comment_begin(std::string::size_type offset) override {
		return to_boolean(Scheduler::instance()->current_process()->cursor(), Scheduler::instance()->invoke(m_self, symbols::onCommentBegin, create_number(offset)));
	}

	bool on_comment_end(std::string::size_type offset) override {
		return to_boolean(Scheduler::instance()->current_process()->cursor(), Scheduler::instance()->invoke(m_self, symbols::onCommentEnd, create_number(offset)));
	}

	bool on_module_path_token(const std::vector<std::string> &context, const std::string &token, std::string::size_type offset) override {
		WeakReference context_values = create_array();
		std::for_each(context.begin(), context.end(), [&context_values](const std::string &context_symbol) {
			array_append(context_values.data<Array>(), create_string(context_symbol));
		});
		return to_boolean(Scheduler::instance()->current_process()->cursor(), Scheduler::instance()->invoke(m_self, symbols::onModulePathToken, std::move(context_values), create_string(token), create_number(offset)));
	}

	bool on_symbol_token(const std::vector<std::string> &context, const std::string &token, std::string::size_type offset) override {
		WeakReference context_values = create_array();
		std::for_each(context.begin(), context.end(), [&context_values](const std::string &context_symbol) {
			array_append(context_values.data<Array>(), create_string(context_symbol));
		});
		return to_boolean(Scheduler::instance()->current_process()->cursor(), Scheduler::instance()->invoke(m_self, symbols::onSymbolToken, std::move(context_values), create_string(token), create_number(offset)));
	}

	bool on_symbol_token(const std::vector<std::string> &context, std::string::size_type offset) override {
		WeakReference context_values = create_array();
		std::for_each(context.begin(), context.end(), [&context_values](const std::string &context_symbol) {
			array_append(context_values.data<Array>(), create_string(context_symbol));
		});
		return to_boolean(Scheduler::instance()->current_process()->cursor(), Scheduler::instance()->invoke(m_self, symbols::onSymbolToken, std::move(context_values), create_number(offset)));
	}

	bool on_token(token::Type type, const std::string &token, std::string::size_type offset) override {
		WeakReference Token = get_global_ignore_visibility(m_lexicalHandlerClass.data<Object>(), symbols::Token);
		return to_boolean(Scheduler::instance()->current_process()->cursor(), Scheduler::instance()->invoke(m_self, symbols::onToken, find_enum_value(Token.data<Object>(), type), create_string(token), create_number(offset)));
	}

	bool on_white_space(const std::string &token, std::string::size_type offset) override {
		return to_boolean(Scheduler::instance()->current_process()->cursor(), Scheduler::instance()->invoke(m_self, symbols::onWhiteSpace, create_string(token), create_number(offset)));
	}

	bool on_comment(const std::string &token, std::string::size_type offset) override {
		return to_boolean(Scheduler::instance()->current_process()->cursor(), Scheduler::instance()->invoke(m_self, symbols::onComment, create_string(token), create_number(offset)));
	}

	bool on_new_line(size_t line_number, std::string::size_type offset) override {
		return to_boolean(Scheduler::instance()->current_process()->cursor(), Scheduler::instance()->invoke(m_self, symbols::onNewLine, create_number(line_number), create_number(offset)));
	}

private:
	WeakReference m_lexicalHandlerClass;
	WeakReference m_self;
};

class LexicalHandlerStream : public AbstractLexicalHandlerStream {
public:
	explicit LexicalHandlerStream(Reference &self) :
		m_self(std::move(self)) {

	}

	bool at_end() const override {
		return !m_good;
	}

	bool is_valid() const override {
		return m_good;
	}

protected:
	int get() override {
		if (m_buffer.empty()) {
			WeakReference result = Scheduler::instance()->invoke(m_self, symbols::readChar);
			if (is_instance_of(result, Data::FMT_NONE)) {
				m_good = false;
				return EOF;
			}
			std::string buffer = to_string(result);
			for (auto it = buffer.rbegin(); it != buffer.rend(); ++it) {
				m_buffer.push_back(static_cast<int>(*it));
			}
		}
		int c = m_buffer.back();
		m_buffer.pop_back();
		return c;
	}

private:
	WeakReference m_self;
	std::vector<int> m_buffer;
	bool m_good = true;
};

MINT_FUNCTION(mint_lexical_handler_new, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	Reference &self = helper.pop_parameter();
	helper.return_value(create_object(new MintLexicalHandler(self)));
}

MINT_FUNCTION(mint_lexical_handler_delete, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	delete self.data<LibObject<MintLexicalHandler>>()->impl;
}

MINT_FUNCTION(mint_lexical_handler_parse, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &stream = helper.pop_parameter();
	Reference &self = helper.pop_parameter();

	LexicalHandlerStream handler_stream(stream);
	helper.return_value(create_boolean(self.data<LibObject<MintLexicalHandler>>()->impl->parse(handler_stream)));
}
