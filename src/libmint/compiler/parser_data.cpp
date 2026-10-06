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
#include "mint/compiler/symbol_scope.h"
#include "mint/compiler/type_annotation.h"
#include "mint/config.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/global_data.h"
#include "mint/memory/object.h"
#include "mint/program/symbol.h"
#include <format>
#include <functional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

mint::ParserData::ParserData() :
    variant(TemporaryData()) {}

mint::ParserData::ParserData(PrimitiveTypeKind kind) :
    variant(TemporaryData {
        .type = kind,
    }) {}

mint::ParserData::ParserData(BuiltinTypeKind kind) :
    variant(TemporaryData {
        .type = kind,
    }) {}

mint::ParserData::ParserData(SymbolPath path) :
    variant(TemporaryData {
        .type = std::move(path),
    }) {}

mint::ParserData::ParserData(std::vector<TypeAnnotation> elements) :
    variant(TemporaryData {
        .type = std::move(elements),
    }) {}

mint::ParserData::ParserData(OverriddenVariable variable) :
    variant(std::move(variable)) {}

mint::ParserData::ParserData(TemporaryData temporary) :
    variant(std::move(temporary)) {}

mint::ParserData::ParserData(const TypeAnnotation& type) :
    variant(TemporaryData {
        .type = type,
    }) {}

mint::ParserData::ParserData(const SymbolDefinition& definition) :
    ParserData(std::visit(Overloaded {
                              [](std::monostate) -> ParserData {
	                              return {};
                              },
                              [](const PackageDescription& description) -> ParserData {
	                              return description;
                              },
                              [](const ClassDescription& description) -> ParserData {
	                              return description;
                              },
                              [](FunctionDefinition description) -> ParserData {
	                              return description;
                              },
                              [](const VariableDescription& description) -> ParserData {
	                              return description;
                              },
                              [](const AliasDescription& description) -> ParserData {
	                              return description;
                              },
                          },
        definition)) {}

mint::ParserData::ParserData(const PackageDescription& description) :
    variant(description) {}

mint::ParserData::ParserData(const ClassDescription& description) :
    variant(description) {}

mint::ParserData::ParserData(const FunctionDescription& description) :
    variant(std::vector {std::cref(description)}) {}

mint::ParserData::ParserData(FunctionDefinition description) :
    variant(std::move(description)) {}

mint::ParserData::ParserData(const VariableDescription& description) :
    variant(description) {}

mint::ParserData::ParserData(const AliasDescription& description) :
    variant(description) {}

mint::ParserData::ParserData(DeferredCheck check) :
    variant(std::move(check)) {}

mint::TypeAnnotation mint::ParserData::get_type() const {
	return std::visit(Overloaded {
	                      [](const PackageDescription& description) -> mint::TypeAnnotation {
		                      return description.get_path();
	                      },
	                      [](const ClassDescription& description) -> mint::TypeAnnotation {
		                      return description.get_path();
	                      },
	                      [](const FunctionDefinition&) -> mint::TypeAnnotation {
		                      return PrimitiveTypeKind::function;
	                      },
	                      [](const VariableDescription& description) -> mint::TypeAnnotation {
		                      return description.type;
	                      },
	                      [](const AliasDescription& description) -> mint::TypeAnnotation {
		                      return description.get_type();
	                      },
	                      [](const OverriddenVariable& type_override) -> mint::TypeAnnotation {
		                      return type_override.type;
	                      },
	                      [](const TemporaryData& temporary) -> mint::TypeAnnotation {
		                      return temporary.type;
	                      },
	                      [](const DeferredCheck&) -> mint::TypeAnnotation {
		                      std::unreachable();
	                      },
	                  },
	    *this);
}

std::string mint::ParserData::get_name() const {
	return std::visit(Overloaded {
	                      [](const PackageDescription& description) -> std::string {
		                      return std::format("package '{}'", description.get_path().to_string());
	                      },
	                      [](const ClassDescription& description) -> std::string {
		                      return std::format("class '{}'", description.get_path().to_string());
	                      },
	                      [](const FunctionDefinition& description) -> std::string {
		                      return std::format("function '{}'", description.front().get().get_name().str());
	                      },
	                      [](const VariableDescription& description) -> std::string {
		                      return std::format("variable '{}' of type '{}'", description.name.str(),
		                          description.type.to_string());
	                      },
	                      [](const AliasDescription& description) -> std::string {
		                      return std::format("alias '{}' for {}", description.name.str(),
		                          unwrap_alias(description).get_name());
	                      },
	                      [](const OverriddenVariable& type_override) -> std::string {
		                      return std::format("variable '{}' of type '{}'", type_override.description.name.str(),
		                          type_override.type.to_string());
	                      },
	                      [](const TemporaryData& temporary) -> std::string {
		                      return std::format("'{}' value", temporary.type.to_string());
	                      },
	                      [](const DeferredCheck&) -> std::string {
		                      std::unreachable();
	                      },
	                  },
	    *this);
}

mint::ParserData mint::to_parser_data(const Data* data) {
	switch (data->format()) {
	case Data::Format::none:
		return TemporaryData {
		    .type = PrimitiveTypeKind::none,
		    .data = data,
		};
	case Data::Format::null:
		return TemporaryData {
		    .type = PrimitiveTypeKind::null,
		    .data = data,
		};
	case Data::Format::number:
		return TemporaryData {
		    .type = PrimitiveTypeKind::number,
		    .data = data,
		};
	case Data::Format::boolean:
		return TemporaryData {
		    .type = PrimitiveTypeKind::boolean,
		    .data = data,
		};
	case Data::Format::object:
		switch (const auto* object = static_cast<const Object*>(data); object->metadata.metatype()) {
		case Class::Metatype::object:
			return TemporaryData {
			    .type = object->metadata.get_description().get_path(),
			    .data = data,
			};
		case Class::Metatype::string:
			return TemporaryData {
			    .type = BuiltinTypeKind::string,
			    .data = data,
			};
		case Class::Metatype::regex:
			return TemporaryData {
			    .type = BuiltinTypeKind::regex,
			    .data = data,
			};
		case Class::Metatype::array:
			return TemporaryData {
			    .type = BuiltinTypeKind::array,
			    .data = data,
			};
		case Class::Metatype::hash:
			return TemporaryData {
			    .type = BuiltinTypeKind::hash,
			    .data = data,
			};
		case Class::Metatype::iterator:
			return TemporaryData {
			    .type = BuiltinTypeKind::iterator,
			    .data = data,
			};
		case Class::Metatype::async_iterator:
			return TemporaryData {
			    .type = BuiltinTypeKind::async_iterator,
			    .data = data,
			};
		case Class::Metatype::library:
			return TemporaryData {
			    .type = BuiltinTypeKind::library,
			    .data = data,
			};
		case Class::Metatype::libobject:
			return TemporaryData {
			    .type = BuiltinTypeKind::libobject,
			    .data = data,
			};
		}
	case Data::Format::package:
		return static_cast<const Package*>(data)->data.get_description();
	case Data::Format::function:
		return TemporaryData {
		    .type = PrimitiveTypeKind::function,
		    .data = data,
		};
	case Data::Format::coroutine:
		return TemporaryData {
		    .type = PrimitiveTypeKind::coroutine,
		    .data = data,
		};
	}
	return {};
}

mint::ParserData mint::unwrap_alias(const AliasDescription& alias) {
	return std::visit(Overloaded {
	                      [](const PackageDescription& description) -> ParserData {
		                      return description;
	                      },
	                      [](const ClassDescription& description) -> ParserData {
		                      return description;
	                      },
	                      [](const FunctionDefinition& description) -> ParserData {
		                      return description;
	                      },
	                  },
	    alias.target);
}
