/**
 * Copyright (c) 2026 Gauvain CHÉRY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_COMPILER_ABSTRACT_SYNTAX_TREE_H
#define MINT_COMPILER_ABSTRACT_SYNTAX_TREE_H

#include "mint/config.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mint {

struct AbstractSyntaxTreeNode {
	virtual ~AbstractSyntaxTreeNode() = default;
};

struct Expression : AbstractSyntaxTreeNode {
	~Expression() override = default;
};

struct Identifier final : Expression {
	explicit Identifier(std::string name) :
	    name(std::move(name)) {}

	std::string name;
};

struct VariableExpression final : Expression {
	explicit VariableExpression(std::unique_ptr<Expression> expression) :
	    expression(std::move(expression)) {}

	std::unique_ptr<Expression> expression;
};

struct Literal final : Expression {
	enum class Kind {
		number,
		string,
		regex,
		constant,
	};

	Literal(Kind kind, std::string value) :
	    kind(kind),
	    value(std::move(value)) {}

	Kind kind;
	std::string value;
};

struct BinaryExpression final : Expression {
	BinaryExpression(std::string op, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right) :
	    op(std::move(op)),
	    left(std::move(left)),
	    right(std::move(right)) {}

	std::string op;
	std::unique_ptr<Expression> left;
	std::unique_ptr<Expression> right;
};

struct UnaryExpression final : Expression {
	UnaryExpression(std::string op, std::unique_ptr<Expression> operand) :
	    op(std::move(op)),
	    operand(std::move(operand)) {}

	std::string op;
	std::unique_ptr<Expression> operand;
};

struct ConditionalExpression final : Expression {
	ConditionalExpression(std::unique_ptr<Expression> condition, std::unique_ptr<Expression> then_expression,
	    std::unique_ptr<Expression> else_expression) :
	    condition(std::move(condition)),
	    then_expression(std::move(then_expression)),
	    else_expression(std::move(else_expression)) {}

	std::unique_ptr<Expression> condition;
	std::unique_ptr<Expression> then_expression;
	std::unique_ptr<Expression> else_expression;
};

struct MemberExpression final : Expression {
	MemberExpression(std::unique_ptr<Expression> object, std::string member) :
	    object(std::move(object)),
	    member(std::move(member)) {}

	std::unique_ptr<Expression> object;
	std::string member;
};

struct SubscriptExpression final : Expression {
	SubscriptExpression(std::unique_ptr<Expression> object, std::unique_ptr<Expression> index) :
	    object(std::move(object)),
	    index(std::move(index)) {}

	std::unique_ptr<Expression> object;
	std::unique_ptr<Expression> index;
};

struct CallExpression final : Expression {
	explicit CallExpression(std::unique_ptr<Expression> callee) :
	    callee(std::move(callee)) {}

	std::unique_ptr<Expression> callee;
	std::vector<std::unique_ptr<Expression>> arguments;
};

struct Statement : AbstractSyntaxTreeNode {
	~Statement() override = default;
};

struct ExpressionStatement final : Statement {
	explicit ExpressionStatement(std::unique_ptr<Expression> expression) :
	    expression(std::move(expression)) {}

	std::unique_ptr<Expression> expression;
};

struct ControlStatement final : Statement {
	enum class Kind {
		load,
		try_statement,
		if_statement,
		switch_statement,
		while_statement,
		for_statement,
		break_statement,
		continue_statement,
		print_statement,
		yield_statement,
		return_statement,
		raise_statement,
		exit_statement,
		assignment_statement,
		definition,
	};

	explicit ControlStatement(Kind kind) :
	    kind(kind) {}

	Kind kind;
	std::vector<std::unique_ptr<Expression>> expressions;
};

struct ModuleNode final : AbstractSyntaxTreeNode {
	std::vector<std::unique_ptr<Statement>> statements;
};

class MINT_EXPORT AbstractSyntaxTree {
public:
	[[nodiscard]] const std::unique_ptr<ModuleNode>& root() const;

private:
	friend class AbstractSyntaxTreeBuildContext;

	void set_root(std::unique_ptr<ModuleNode> root);

	std::unique_ptr<ModuleNode> _root;
};

}

#endif // MINT_COMPILER_ABSTRACT_SYNTAX_TREE_H
