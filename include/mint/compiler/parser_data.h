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

#ifndef MINT_COMPILER_PARSER_DATA_H
#define MINT_COMPILER_PARSER_DATA_H

#include "mint/compiler/descriptions.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/compiler/type_annotation.h"
#include "mint/config.h"
#include "mint/memory/data.h"
#include "mint/program/symbol.h"
#include "mint/system/data_stream.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace mint {

class ParserData;

enum class UnaryOperation {
	inc_operator,
	dec_operator,
	not_operator,
	compl_operator,
	pos_operator,
	neg_operator,
	typeof_operator,
	membersof_operator,
	defined_operator,
	await_operator,
};

enum class BinaryOperation {
	move_operator,
	copy_operator,
	add_operator,
	sub_operator,
	mul_operator,
	div_operator,
	pow_operator,
	mod_operator,
	is_operator,
	as_operator,
	in_operator,
	eq_operator,
	ne_operator,
	lt_operator,
	gt_operator,
	le_operator,
	ge_operator,
	and_operator,
	or_operator,
	band_operator,
	bor_operator,
	xor_operator,
	shift_left_operator,
	shift_right_operator,
	inclusive_range_operator,
	exclusive_range_operator,
	subscript_operator,
	regex_match_operator,
	regex_unmatch_operator,
	strict_eq_operator,
	strict_ne_operator,
};

enum class TernaryOperation {
	conditional_operator,
	subscript_move_operator,
};

struct LookupContext {
	std::reference_wrapper<PackageDescription> package;
	FunctionDescription* function = nullptr;
};

struct DeferredInitSymbolCheck {
	std::shared_ptr<ParserData> target;
	std::shared_ptr<ParserData> value;
	LookupContext lookup_context;
};

struct DeferredUnaryOperationCheck {
	UnaryOperation operation;
	std::shared_ptr<ParserData> operand;
};

struct DeferredBinaryOperationCheck {
	BinaryOperation operation;
	std::shared_ptr<ParserData> lhs;
	std::shared_ptr<ParserData> rhs;
};

struct DeferredTernaryOperationCheck {
	TernaryOperation operation;
	std::shared_ptr<ParserData> first;
	std::shared_ptr<ParserData> second;
	std::shared_ptr<ParserData> third;
};

struct DeferredMemberCheck {
	std::shared_ptr<ParserData> object;
	Symbol member;
};

struct DeferredSymbolCheck {
	SymbolPath symbol;
	const SymbolScope* scope = nullptr;
	const SymbolScope* fallback_scope = nullptr;
};

struct DeferredFunctionCallCheck {
	std::shared_ptr<ParserData> function;
	std::vector<ParserData> parameters;
	std::size_t with_unpack = 0;
};

struct DeferredMethodCallCheck {
	std::shared_ptr<ParserData> object;
	Symbol method;
	std::vector<ParserData> parameters;
	std::size_t with_unpack = 0;
};

struct DeferredReturnCheck {
	TypeAnnotation expected;
	std::shared_ptr<ParserData> actual;
};

class DeferredCheck {
public:
	using Definition = std::variant<DeferredInitSymbolCheck, DeferredUnaryOperationCheck, DeferredBinaryOperationCheck,
	    DeferredTernaryOperationCheck, DeferredMemberCheck, DeferredSymbolCheck, DeferredFunctionCallCheck,
	    DeferredMethodCallCheck, DeferredReturnCheck>;

	std::shared_ptr<Definition> definition;
	mutable std::shared_ptr<ParserData> result;
	DataStream::Position position;
};

struct OverriddenVariable {
	VariableDescription description;
	TypeAnnotation type;
	std::size_t index;
};

struct TemporaryData {
	TypeAnnotation type;
	const Data* data = nullptr;
	const VariableDescription* typename_for = nullptr;
};

class MINT_EXPORT ParserData :
    public std::variant<PackageDefinition, ClassDefinition, FunctionDefinition, VariableDefinition, AliasDefinition,
        OverriddenVariable, TemporaryData, DeferredCheck> {
public:
	ParserData();
	explicit(false) ParserData(PrimitiveTypeKind kind);
	explicit(false) ParserData(SymbolPath path);
	explicit(false) ParserData(BuiltinTypeKind kind);
	explicit(false) ParserData(std::vector<TypeAnnotation> elements);
	explicit(false) ParserData(OverriddenVariable variable);
	explicit(false) ParserData(TemporaryData temporary);
	explicit(false) ParserData(const TypeAnnotation& type);
	explicit(false) ParserData(const SymbolDefinition& definition);
	explicit(false) ParserData(const PackageDescription& description);
	explicit(false) ParserData(const ClassDescription& description);
	explicit(false) ParserData(const FunctionDescription& description);
	explicit(false) ParserData(FunctionDefinition description);
	explicit(false) ParserData(const VariableDescription& description);
	explicit(false) ParserData(const AliasDescription& description);
	explicit(false) ParserData(DeferredCheck check);

	[[nodiscard]] TypeAnnotation get_type() const;
	[[nodiscard]] std::string get_name() const;

	[[nodiscard]] bool is_package() const {
		return std::holds_alternative<PackageDefinition>(*this);
	}

	[[nodiscard]] bool is_class() const {
		return std::holds_alternative<ClassDefinition>(*this);
	}

	[[nodiscard]] bool is_function() const {
		return std::holds_alternative<FunctionDefinition>(*this);
	}

	[[nodiscard]] bool is_variable() const {
		return std::holds_alternative<VariableDefinition>(*this);
	}

	[[nodiscard]] bool is_temporary() const {
		return std::holds_alternative<TemporaryData>(*this);
	}

	[[nodiscard]] bool is_deferred() const {
		return std::holds_alternative<DeferredCheck>(*this);
	}
};

MINT_EXPORT ParserData to_parser_data(const Data* data);
MINT_EXPORT ParserData unwrap_alias(const AliasDescription& alias);

}

#endif // MINT_COMPILER_PARSER_DATA_H
