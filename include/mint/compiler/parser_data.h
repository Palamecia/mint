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
#include "mint/compiler/type_annotation.h"
#include "mint/config.h"
#include "mint/memory/data.h"
#include "mint/program/symbol.h"
#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace mint {

class MINT_EXPORT ParserData :
    public std::variant<std::reference_wrapper<const PackageDescription>,
        std::reference_wrapper<const ClassDescription>, std::reference_wrapper<const FunctionDescription>,
        std::reference_wrapper<const VariableDescription>, TypeAnnotation> {
public:
	ParserData();
	explicit(false) ParserData(PrimitiveTypeKind kind);
	explicit(false) ParserData(SymbolPath path);
	explicit(false) ParserData(BuiltinTypeKind kind);
	explicit(false) ParserData(std::vector<TypeAnnotation> elements);
	explicit(false) ParserData(const TypeAnnotation& type);
	explicit(false) ParserData(const PackageDescription& description);
	explicit(false) ParserData(const ClassDescription& description);
	explicit(false) ParserData(const FunctionDescription& description);
	explicit(false) ParserData(const VariableDescription& description);

	[[nodiscard]] TypeAnnotation get_type() const;
	[[nodiscard]] std::string get_name() const;
};

MINT_EXPORT ParserData to_parser_data(const Data* data);

}

#endif // MINT_COMPILER_PARSER_DATA_H
