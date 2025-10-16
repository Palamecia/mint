/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#include "mint/memory/classtool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/classregister.h"
#include "mint/ast/symbol.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/garbagecollector.h"
#include "mint/system/error.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace mint;

Class& mint::create_enum(AbstractSyntaxTree& ast, const std::string& name,
    std::span<const std::pair<Symbol, std::optional<std::intmax_t>>> values) {
	return create_enum(ast.global_data(), name, values);
}

Class& mint::create_enum(PackageData& package, const std::string& name,
    std::span<const std::pair<Symbol, std::optional<std::intmax_t>>> values) {

	std::size_t next_enum_value = 0;
	auto desc = std::make_unique<ClassDescription>(package, Reference::default_flags, name);
	const Reference::Flags flags = Reference::const_value | Reference::const_address | Reference::global;

	for (const auto& [symbol, value] : values) {
		if (value.has_value()) {
			if (!desc->create_member(symbol, make_weak_reference<Number>(flags, *value))) {
				error("{}: member was already defined for enum '{}'", symbol.str(), name);
			}
			next_enum_value = static_cast<std::size_t>(*value) + 1;
		}
		else {
			if (!desc->create_member(symbol, make_weak_reference<Number>(flags, next_enum_value++))) {
				error("{}: member was already defined for enum '{}'", symbol.str(), name);
			}
		}
	}

	auto& desc_ref = *desc;
	package.register_class(package.create_class(std::move(desc)));
	return desc_ref.generate();
}

Class& mint::create_enum(AbstractSyntaxTree& ast, const std::string& name,
    std::initializer_list<std::pair<Symbol, std::optional<std::intmax_t>>> values) {
	return create_enum(ast.global_data(), name, std::span(values.begin(), values.end()));
}

Class& mint::create_enum(PackageData& package, const std::string& name,
    std::initializer_list<std::pair<Symbol, std::optional<std::intmax_t>>> values) {
	return create_enum(package, name, std::span(values.begin(), values.end()));
}

Class& mint::create_class(AbstractSyntaxTree& ast, const std::string& name,
    std::span<const std::pair<Symbol, WeakReference>> members) {
	return create_class(ast.global_data(), name, std::span<ClassRegister::Path>(), members);
}

Class& mint::create_class(PackageData& package, const std::string& name,
    std::span<const std::pair<Symbol, WeakReference>> members) {
	return create_class(package, name, std::span<ClassRegister::Path>(), members);
}

Class& mint::create_class(AbstractSyntaxTree& ast, const std::string& name,
    std::span<const std::reference_wrapper<ClassDescription>> bases,
    std::span<const std::pair<Symbol, WeakReference>> members) {
	auto bases_path = std::vector<ClassRegister::Path>(std::from_range,
	    std::views::transform(bases, &ClassDescription::get_path));
	return create_class(ast.global_data(), name, std::span(bases_path), members);
}

Class& mint::create_class(PackageData& package, const std::string& name,
    std::span<const std::reference_wrapper<ClassDescription>> bases,
    std::span<const std::pair<Symbol, WeakReference>> members) {
	auto bases_path = std::vector<ClassRegister::Path>(std::from_range,
	    std::views::transform(bases, &ClassDescription::get_path));
	return create_class(package, name, std::span(bases_path), members);
}

Class& mint::create_class(AbstractSyntaxTree& ast, const std::string& name, std::span<const ClassRegister::Path> bases,
    std::span<const std::pair<Symbol, WeakReference>> members) {
	return create_class(ast.global_data(), name, bases, members);
}

Class& mint::create_class(PackageData& package, const std::string& name, std::span<const ClassRegister::Path> bases,
    std::span<const std::pair<Symbol, WeakReference>> members) {

	auto desc = std::make_unique<ClassDescription>(package, Reference::default_flags, name);

	for (const auto& base : bases) {
		desc->add_base(base);
	}

	for (const auto& [symbol, member] : members) {
		if (!desc->create_member(symbol, member)) {
			error("{}: member was already defined for class '{}'", symbol.str(), name);
		}
	}

	auto& desc_ref = *desc;
	package.register_class(package.create_class(std::move(desc)));
	return desc_ref.generate();
}

Class& mint::create_class(AbstractSyntaxTree& ast, const std::string& name,
    std::initializer_list<std::pair<Symbol, WeakReference>> members) {
	return create_class(ast.global_data(), name, std::span<ClassRegister::Path>(),
	    std::span(members.begin(), members.end()));
}

Class& mint::create_class(PackageData& package, const std::string& name,
    std::initializer_list<std::pair<Symbol, WeakReference>> members) {
	return create_class(package, name, std::span<ClassRegister::Path>(), std::span(members.begin(), members.end()));
}

Class& mint::create_class(AbstractSyntaxTree& ast, const std::string& name,
    std::initializer_list<std::reference_wrapper<mint::ClassDescription>> bases,
    std::initializer_list<std::pair<Symbol, WeakReference>> members) {
	auto bases_path = std::vector<ClassRegister::Path>(std::from_range,
	    std::views::transform(bases, &ClassDescription::get_path));
	return create_class(ast.global_data(), name, std::span(bases_path), std::span(members.begin(), members.end()));
}

Class& mint::create_class(PackageData& package, const std::string& name,
    std::initializer_list<std::reference_wrapper<mint::ClassDescription>> bases,
    std::initializer_list<std::pair<Symbol, WeakReference>> members) {
	auto bases_path = std::vector<ClassRegister::Path>(std::from_range,
	    std::views::transform(bases, &ClassDescription::get_path));
	return create_class(package, name, std::span(bases_path), std::span(members.begin(), members.end()));
}

Class& mint::create_class(AbstractSyntaxTree& ast, const std::string& name,
    std::initializer_list<mint::ClassRegister::Path> bases,
    std::initializer_list<std::pair<Symbol, WeakReference>> members) {
	return create_class(ast.global_data(), name, std::span(bases.begin(), bases.end()),
	    std::span(members.begin(), members.end()));
}

Class& mint::create_class(PackageData& package, const std::string& name,
    std::initializer_list<mint::ClassRegister::Path> bases,
    std::initializer_list<std::pair<Symbol, WeakReference>> members) {
	return create_class(package, name, std::span(bases.begin(), bases.end()), std::span(members.begin(), members.end()));
}
