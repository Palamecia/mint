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

#include "mint/memory/class_tools.h"
#include "mint/compiler/class_builder.h"
#include "mint/compiler/compiler.h"
#include "mint/compiler/descriptions.h"
#include "mint/compiler/type_annotation.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/data.h"
#include "mint/program/program.h"
#include "mint/program/module.h"
#include "mint/program/symbol.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/garbage_collector.h"
#include "mint/system/error.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

using namespace mint;

Class& mint::create_enum(Program& program, Symbol name,
    std::span<const std::pair<Symbol, std::optional<std::intmax_t>>> values) {
	return create_enum(program, program.main(), std::move(name), values);
}

Class& mint::create_enum(Program& program, ModuleInfo& module, Symbol name,
    std::span<const std::pair<Symbol, std::optional<std::intmax_t>>> values) {

	std::size_t next_enum_value = 0;
	auto compiler = Compiler(program, module);
	auto& desc = module.description.create_class(std::move(name));
	const Reference::Flags flags = Reference::const_value | Reference::const_address | Reference::global;

	for (const auto& [symbol, value] : values) {
		if (value.has_value()) {
			desc.create_attribute(symbol, PrimitiveTypeKind::number, flags,
			    compiler.make_constant(flags, compiler.make_data<Number>(*value)));
			next_enum_value = static_cast<std::size_t>(*value) + 1;
		}
		else {
			desc.create_attribute(symbol, PrimitiveTypeKind::number, flags,
			    compiler.make_constant(flags, compiler.make_data<Number>(next_enum_value++)));
		}
	}

	return desc.generate(ClassBuilder(program));
}

Class& mint::create_enum(Program& program, Symbol name,
    std::initializer_list<std::pair<Symbol, std::optional<std::intmax_t>>> values) {
	return create_enum(program, program.main(), std::move(name), std::span(values.begin(), values.end()));
}

Class& mint::create_enum(Program& program, ModuleInfo& module, Symbol name,
    std::initializer_list<std::pair<Symbol, std::optional<intmax_t>>> values) {
	return create_enum(program, module, std::move(name), std::span(values.begin(), values.end()));
}

Class& mint::create_class(Program& program, Symbol name, std::span<const CreateMember> members) {
	return create_class(program, program.main(), std::move(name), std::span<SymbolPath>(), members);
}

Class& mint::create_class(Program& program, ModuleInfo& module, Symbol name, std::span<const CreateMember> members) {
	return create_class(program, module, std::move(name), std::span<SymbolPath>(), members);
}

Class& mint::create_class(Program& program, Symbol name,
    std::span<const std::reference_wrapper<ClassDescription>> bases, std::span<const CreateMember> members) {
	auto bases_path = std::vector<SymbolPath>(std::from_range,
	    std::views::transform(bases, &ClassDescription::get_path));
	return create_class(program, program.main(), std::move(name), std::span(bases_path), members);
}

Class& mint::create_class(Program& program, ModuleInfo& module, Symbol name,
    std::span<const std::reference_wrapper<mint::ClassDescription>> bases, std::span<const CreateMember> members) {
	auto bases_path = std::vector<SymbolPath>(std::from_range,
	    std::views::transform(bases, &ClassDescription::get_path));
	return create_class(program, module, std::move(name), std::span(bases_path), members);
}

Class& mint::create_class(Program& program, Symbol name, std::span<const SymbolPath> bases,
    std::span<const CreateMember> members) {
	return create_class(program, program.main(), std::move(name), bases, members);
}

Class& mint::create_class(Program& program, ModuleInfo& module, Symbol name, std::span<const SymbolPath> bases,
    std::span<const CreateMember> members) {

	auto compiler = Compiler(program, module);
	auto& desc = module.description.create_class(std::move(name));

	for (const auto& base : bases) {
		desc.add_base(base);
	}

	for (const auto& member : members) {
		switch (member.value.data().format()) {
		case Data::Format::none:
		case Data::Format::null:
		case Data::Format::number:
		case Data::Format::boolean:
			break;
		case Data::Format::object:
			switch (member.value.data<Object>().metadata.metatype()) {
			case Class::Metatype::string:
			case Class::Metatype::regex:
			case Class::Metatype::library:
			case Class::Metatype::libobject:
				break;
			case Class::Metatype::object:
				error("member '{}' can not use objects", member.symbol.str());
				break;
			case Class::Metatype::array:
				if (!member.value.data<Array>().values.empty()) [[unlikely]] {
					error("member '{}' can not use objects", member.symbol.str());
				}
				break;
			case Class::Metatype::hash:
				if (!member.value.data<Hash>().values.empty()) [[unlikely]] {
					error("member '{}' can not use objects", member.symbol.str());
				}
				break;
			case Class::Metatype::async_iterator:
			case Class::Metatype::iterator:
				error("member '{}' can not use objects", member.symbol.str());
				break;
			}
			break;
		case Data::Format::package:
			error("member '{}' can not use packages", member.symbol.str());
			break;
		case Data::Format::function:
		case Data::Format::coroutine:
			if (is_stateful_function(member.value)) [[unlikely]] {
				error("member '{}' can not use stateful functions", member.symbol.str());
			}
			break;
		}
		desc.create_attribute(member.symbol, member.type, member.value.flags(),
		    compiler.make_constant(member.value.flags(), &member.value.data()));
	}

	return desc.generate(ClassBuilder(program));
}

Class& mint::create_class(Program& program, Symbol name, std::initializer_list<CreateMember> members) {
	return create_class(program, program.main(), std::move(name), std::span<SymbolPath>(),
	    std::span(members.begin(), members.end()));
}

Class& mint::create_class(Program& program, ModuleInfo& module, Symbol name,
    std::initializer_list<CreateMember> members) {
	return create_class(program, module, std::move(name), std::span<SymbolPath>(),
	    std::span(members.begin(), members.end()));
}

Class& mint::create_class(Program& program, Symbol name,
    std::initializer_list<std::reference_wrapper<mint::ClassDescription>> bases,
    std::initializer_list<CreateMember> members) {
	auto bases_path = std::vector<SymbolPath>(std::from_range,
	    std::views::transform(bases, &ClassDescription::get_path));
	return create_class(program, program.main(), std::move(name), std::span(bases_path),
	    std::span(members.begin(), members.end()));
}

Class& mint::create_class(Program& program, ModuleInfo& module, Symbol name,
    std::initializer_list<std::reference_wrapper<mint::ClassDescription>> bases,
    std::initializer_list<CreateMember> members) {
	auto bases_path = std::vector<SymbolPath>(std::from_range,
	    std::views::transform(bases, &ClassDescription::get_path));
	return create_class(program, module, std::move(name), std::span(bases_path),
	    std::span(members.begin(), members.end()));
}

Class& mint::create_class(Program& program, Symbol name, std::initializer_list<mint::SymbolPath> bases,
    std::initializer_list<CreateMember> members) {
	return create_class(program, program.main(), std::move(name), std::span(bases.begin(), bases.end()),
	    std::span(members.begin(), members.end()));
}

Class& mint::create_class(Program& program, ModuleInfo& module, Symbol name,
    std::initializer_list<mint::SymbolPath> bases, std::initializer_list<CreateMember> members) {
	return create_class(program, module, std::move(name), std::span(bases.begin(), bases.end()),
	    std::span(members.begin(), members.end()));
}
