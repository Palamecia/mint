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

#ifndef SYMBOLEVALUATOR_H
#define SYMBOLEVALUATOR_H

#include <mint/compiler/lexicalhandler.h>
#include <mint/ast/cursor.h>
#include <optional>

class SymbolEvaluator : public mint::LexicalHandler {
public:
	SymbolEvaluator(mint::Cursor *cursor);

	const std::optional<mint::WeakReference> &get_reference() const;
	std::string get_symbol_name() const;

protected:
	enum State {
		READ_IDENT,
		READ_MEMBER,
		READ_OPERATOR
	};

	bool on_token(mint::token::Type type, const std::string &token, std::string::size_type offset) override;

private:
	std::optional<mint::WeakReference> get_symbol_reference(mint::SymbolTable *symbols, const mint::Symbol &symbol);
	std::optional<mint::WeakReference> get_member_reference(mint::Reference &reference, const mint::Symbol &member);

	mint::Cursor *m_cursor;
	State m_state = READ_IDENT;

	std::optional<mint::WeakReference> m_reference;
	std::string m_symbol_name;
};

#endif // SYMBOLEVALUATOR_H
