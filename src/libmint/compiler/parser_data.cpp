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

#include "mint/compiler/parser_data.h"
#include "mint/compiler/descriptions.h"
#include "mint/compiler/type_annotation.h"
#include "mint/config.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/program/symbol.h"
#include <format>
#include <functional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

mint::ParserData::ParserData() :
    variant(TypeAnnotation()) {}

mint::ParserData::ParserData(PrimitiveTypeKind kind) :
    variant(TypeAnnotation(kind)) {}

mint::ParserData::ParserData(BuiltinTypeKind kind) :
    variant(TypeAnnotation(kind)) {}

mint::ParserData::ParserData(SymbolPath path) :
    variant(TypeAnnotation(std::move(path))) {}

mint::ParserData::ParserData(std::vector<TypeAnnotation> elements) :
    variant(TypeAnnotation(std::move(elements))) {}

mint::ParserData::ParserData(const TypeAnnotation& type) :
    variant(type) {}

mint::ParserData::ParserData(const PackageDescription& description) :
    variant(description) {}

mint::ParserData::ParserData(const ClassDescription& description) :
    variant(description) {}

mint::ParserData::ParserData(const FunctionDescription& description) :
    variant(description) {}

mint::ParserData::ParserData(const VariableDescription& description) :
    variant(description) {}

mint::TypeAnnotation mint::ParserData::get_type() const {
	return std::visit(Overloaded {
	                      [](std::reference_wrapper<const PackageDescription> /*description*/) -> mint::TypeAnnotation {
		                      return PrimitiveTypeKind::package;
	                      },
	                      [](std::reference_wrapper<const ClassDescription> description) -> mint::TypeAnnotation {
		                      return description.get().get_path();
	                      },
	                      [](std::reference_wrapper<const FunctionDescription> /*description*/) -> mint::TypeAnnotation {
		                      return PrimitiveTypeKind::function;
	                      },
	                      [](std::reference_wrapper<const VariableDescription> description) -> mint::TypeAnnotation {
		                      return description.get().type;
	                      },
	                      [](const TypeAnnotation& type) -> mint::TypeAnnotation {
		                      return type;
	                      },
	                  },
	    *this);
}

std::string mint::ParserData::get_name() const {
	return std::visit(Overloaded {
	                      [](std::reference_wrapper<const PackageDescription> description) -> std::string {
		                      return std::format("package '{}'", description.get().get_path().to_string());
	                      },
	                      [](std::reference_wrapper<const ClassDescription> description) -> std::string {
		                      return std::format("class '{}'", description.get().get_path().to_string());
	                      },
	                      [](std::reference_wrapper<const FunctionDescription> description) -> std::string {
		                      return std::format("function '{}'", description.get().get_name().str());
	                      },
	                      [](std::reference_wrapper<const VariableDescription> description) -> std::string {
		                      return std::format("variable '{}' of type '{}'", description.get().name.str(),
		                          description.get().type.to_string());
	                      },
	                      [](const TypeAnnotation& type) -> std::string {
		                      return std::format("'{}' value", type.to_string());
	                      },
	                  },
	    *this);
}

mint::ParserData mint::to_parser_data(const Data* data) {
	switch (data->format()) {
	case Data::Format::none:
		return PrimitiveTypeKind::none;
	case Data::Format::null:
		return PrimitiveTypeKind::null;
	case Data::Format::number:
		return PrimitiveTypeKind::number;
	case Data::Format::boolean:
		return PrimitiveTypeKind::boolean;
	case Data::Format::object:
		switch (const auto* object = static_cast<const Object*>(data); object->metadata.metatype()) {
		case Class::Metatype::object:
			return object->metadata.get_description();
		case Class::Metatype::string:
			return BuiltinTypeKind::string;
		case Class::Metatype::regex:
			return BuiltinTypeKind::regex;
		case Class::Metatype::array:
			return BuiltinTypeKind::array;
		case Class::Metatype::hash:
			return BuiltinTypeKind::hash;
		case Class::Metatype::iterator:
			return BuiltinTypeKind::iterator;
		case Class::Metatype::async_iterator:
			return BuiltinTypeKind::async_iterator;
		case Class::Metatype::library:
			return BuiltinTypeKind::library;
		case Class::Metatype::libobject:
			return BuiltinTypeKind::libobject;
		}
	case Data::Format::package:
		return PrimitiveTypeKind::package;
	case Data::Format::function:
		return PrimitiveTypeKind::function;
	case Data::Format::coroutine:
		return PrimitiveTypeKind::coroutine;
	}
	return {};
}
