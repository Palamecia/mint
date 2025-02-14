%{
/**
 * Copyright (c) 2025 Gauvain CHERY.
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

#ifndef MINTDOC_DOCPARSER_HPP
#define MINTDOC_DOCPARSER_HPP

#include "docparser.h"
#include "doclexer.h"

#include <sstream>
#include <memory>

#define YYSTYPE std::string
#define yylex self->next_token

%}

%parse-param {DocParser *self}

%token SHARP_TOKEN

%token WORD_TOKEN BLANK_TOKEN
%token LINE_BREAK_TOKEN FILE_END_TOKEN

%%

document_rule:
    stmt_list_rule FILE_END_TOKEN {
		fflush(stdout);
		YYACCEPT;
	}
	| FILE_END_TOKEN {
		fflush(stdout);
		YYACCEPT;
	};

stmt_list_rule:
    stmt_list_rule stmt_rule
	| stmt_rule;

stmt_rule:
    heading_rule
	| paragraph_rule;

heading_rule:
	heading_begin_rule text_rule LINE_BREAK_TOKEN {
		self->pop_node();
	};

heading_begin_rule:
	SHARP_TOKEN BLANK_TOKEN {
		self->push_heading(1);
	}
	| SHARP_TOKEN SHARP_TOKEN BLANK_TOKEN {
		self->push_heading(2);
	}
	| SHARP_TOKEN SHARP_TOKEN SHARP_TOKEN BLANK_TOKEN {
		self->push_heading(3);
	}
	| SHARP_TOKEN SHARP_TOKEN SHARP_TOKEN SHARP_TOKEN BLANK_TOKEN {
		self->push_heading(4);
	}
	| SHARP_TOKEN SHARP_TOKEN SHARP_TOKEN SHARP_TOKEN SHARP_TOKEN BLANK_TOKEN {
		self->push_heading(5);
	}
	| SHARP_TOKEN SHARP_TOKEN SHARP_TOKEN SHARP_TOKEN SHARP_TOKEN SHARP_TOKEN BLANK_TOKEN {
		self->push_heading(6);
	};

paragraph_rule:
	paragraph_begin_rule text_rule blank_lines_rule {
		self->pop_node();
	};

paragraph_begin_rule:
	WORD_TOKEN {
		self->push_paragraph();
		self->insert_node(DocParser::create_text($1));
	};

text_rule:
	text_rule BLANK_TOKEN {
		self->insert_node(DocParser::create_text($2));
	}
	text_rule WORD_TOKEN {
		self->insert_node(DocParser::create_text($2));
	}
	| BLANK_TOKEN {
		self->insert_node(DocParser::create_text($1));
	}
	| WORD_TOKEN {
		self->insert_node(DocParser::create_text($1));
	};

blank_lines_rule:
	LINE_BREAK_TOKEN LINE_BREAK_TOKEN
	| blank_lines_rule LINE_BREAK_TOKEN

%%

void yy::parser::error(const std::string &msg) {
	self->parse_error(msg.c_str());
}

int DocParser::next_token(std::string *token) {

	if (m_lexer->at_end()) {
	    return yy::parser::token::FILE_END_TOKEN;
	}

	*token = m_lexer->next_token();
	return DocLexer::token_type(*token);
}

std::unique_ptr<DocNode> DocParser::parse(std::stringstream &stream) {
    
	m_root.reset(new DocNodeBlock);
	m_root->type = DocNode::NODE_DOCUMENT;
	m_current = {m_root.get()};

	m_lexer = std::make_unique<DocLexer>(stream);
	yy::parser parser(this);

	if (!parser.parse()) {
        return {};
    }

    return std::move(m_root);
}

#endif // MINTDOC_DOCPARSER_HPP
