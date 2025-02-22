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

#ifndef EXPRESSIONEVALUATOR_H
#define EXPRESSIONEVALUATOR_H

#include <mint/compiler/lexicalhandler.h>
#include <mint/ast/cursor.h>

#include <cstdint>

class ExpressionEvaluator : public mint::LexicalHandler {
public:
	ExpressionEvaluator(mint::AbstractSyntaxTree *ast);
	ExpressionEvaluator(const ExpressionEvaluator &) = delete;
	ExpressionEvaluator(ExpressionEvaluator &&) = delete;
	~ExpressionEvaluator();

	ExpressionEvaluator &operator=(const ExpressionEvaluator &) = delete;
	ExpressionEvaluator &operator=(ExpressionEvaluator &&) = delete;

	void setup_locals(const mint::SymbolTable &symbols);

	mint::Reference &get_result();

protected:
	bool on_token(mint::token::Type type, const std::string &token, std::string::size_type offset) override;

private:
	enum State : std::uint8_t {
		READ_OPERAND,
		READ_OPERATOR,
		READ_MEMBER
	};

	enum Associativity : std::uint8_t {
		LEFT_TO_RIGHT,
		RIGHT_TO_LEFT
	};

	struct Priority {
		int level;
		std::vector<void (*)(mint::Cursor *)> unary_operations;
		std::vector<void (*)(mint::Cursor *)> binary_operations;
	};

	struct EvaluatorState {
		State state = READ_OPERAND;
		std::vector<Priority> priority;
	};

	static Associativity associativity(int level);

	void on_unary_operator(int level, void (*operation)(mint::Cursor *));
	void on_binary_operator(int level, void (*operation)(mint::Cursor *));

	[[nodiscard]] State get_state() const;
	void push_state(State state);
	void set_state(State state);
	void pop_state();

	std::unique_ptr<mint::Cursor> m_cursor;
	std::vector<EvaluatorState> m_state = {EvaluatorState {}};
};

#endif // EXPRESSIONEVALUATOR_H
