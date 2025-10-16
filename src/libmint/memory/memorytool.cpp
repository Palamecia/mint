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

#include "mint/memory/memorytool.h"
#include "mint/ast/printer.h"
#include "mint/ast/symbol.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/reference.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/objectprinter.h"
#include "mint/memory/casttool.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/system/error.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/fileprinter.h"
#include "mint/ast/cursor.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>

using namespace mint;

namespace {

bool ensure_not_defined(const Symbol& symbol, SymbolTable& symbols) {

	auto it = symbols.find(symbol);
	if (it != symbols.end()) {
		if (it->second.data().format() != Data::none_format) [[unlikely]] {
			return false;
		}
		symbols.erase(it);
	}

	return true;
}

Cursor::Call& setup_member_call(Cursor& cursor, Reference& reference) {

	assert(reference.data().format() == Data::object_format);
	auto* object = &reference.data<Object>();
	Cursor::Call::Flags flags = Cursor::Call::member_call;
	Class* metadata = &object->metadata;

	if (mint::is_class(*object)) {

		if (metadata->metatype() == Class::object) {
			auto instance = WeakReference(copy_from, reference.flags() | Reference::temporary, reference.data());
			object = &instance.data<Object>();
			object->construct();
			reference = std::move(instance);
		}
		else {
			/*
			 * Builtin classes can not be aliased, there is no need to clone the prototype.
			 */
			object->construct();
		}

		if (Class::MemberInfo* info = metadata->find_operator(Class::new_operator)) {

			switch (info->value.flags() & Reference::visibility_mask) {
			case Reference::protected_visibility:
				if (!is_protected_accessible(cursor, info->owner)) [[unlikely]] {
					error("could not access protected member 'new' of class '{}'", metadata->full_name());
				}
				break;
			case Reference::private_visibility:
				if (!is_private_accessible(cursor, info->owner)) [[unlikely]] {
					error("could not access private member 'new' of class '{}'", metadata->full_name());
				}
				break;
			case Reference::package_visibility:
				if (!is_package_accessible(cursor, info->owner)) [[unlikely]] {
					error("could not access package member 'new' of class '{}'", metadata->full_name());
				}
				break;
			default:
				break;
			}

			cursor.waiting_calls().emplace(Class::MemberInfo::get(*info, *object));
			metadata = &info->owner.get();
		}
		else {
			cursor.waiting_calls().emplace(create_none());
		}
	}
	else if (Class::MemberInfo* info = metadata->find_operator(Class::call_operator)) {
		cursor.waiting_calls().emplace(Class::MemberInfo::get(*info, *object));
		flags |= Cursor::Call::operator_call;
		metadata = &info->owner.get();
	}
	else {
		cursor.waiting_calls().emplace(reference);
	}

	Cursor::Call& call = cursor.waiting_calls().top();
	call.set_metadata(metadata);
	call.set_flags(flags);
	return call;
}

}

std::string mint::type_name(const Reference& reference) {
	switch (reference.data().format()) {
	case Data::none_format:
		return "none";
	case Data::null_format:
		return "null";
	case Data::number_format:
		return "number";
	case Data::boolean_format:
		return "boolean";
	case Data::object_format:
		return reference.data<Object>().metadata.full_name();
	case Data::package_format:
		return "package";
	case Data::function_format:
		return "function";
	}
	return {};
}

bool mint::is_instance_of(const Reference& reference, const std::string& type_name) {
	switch (reference.data().format()) {
	case Data::object_format:
		if (reference.data<Object>().metadata.metatype() == Class::object) {
			return type_name == reference.data<Object>().metadata.full_name();
		}
		break;
	case Data::package_format:
		return type_name == reference.data<Package>().data.full_name();
	default:
		break;
	}
	return false;
}

std::unique_ptr<Printer> mint::create_printer(Cursor& cursor) {

	const auto ref = std::move(cursor.stack().back());
	cursor.stack().pop_back();

	switch (ref.data().format()) {
	case Data::number_format:
		return std::make_unique<FilePrinter>(to_integer<int>(ref.data<Number>().value));
	case Data::object_format:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::string:
			return std::make_unique<FilePrinter>(ref.data<String>().str);
		case Class::object:
			return std::make_unique<ObjectPrinter>(cursor, ref.flags(), ref.data<Object>());
		default:
			break;
		}
		[[fallthrough]];
	default:
		error("cannot open printer from '{}'", type_name(ref));
	}
}

void mint::load_extra_arguments(Cursor& cursor) {

	auto args = create_iterator_over(cursor.ast(), cursor.stack().back());

	cursor.stack().pop_back();
	args.data<Iterator>().ctx.finalize();

	cursor.waiting_calls().top().add_extra_argument(args.data<Iterator>().ctx.size());
	std::ranges::move(args.data<Iterator>().ctx, std::back_inserter(cursor.stack()));
}

void mint::capture_symbol(Cursor& cursor, const Symbol& symbol) {

	const auto& function = cursor.stack().back();
	auto stateful_signatures = std::views::transform(function.data<Function>().mapping, [](auto& signature) {
		return signature.second.template context<Function::Stateful>();
	}) | std::views::filter([](auto* context) {
		return context != nullptr;
	}) | std::views::transform([](auto* context) -> Function::Stateful& {
		return *context;
	});

	for (auto& signature : stateful_signatures) {
		if (auto item = cursor.symbols().find(symbol); item != cursor.symbols().end()) {
			signature.capture(symbol, item->second);
		}
	}
}

void mint::capture_as_symbol(Cursor& cursor, const Symbol& symbol) {

	const auto reference = std::move(cursor.stack().back());
	cursor.stack().pop_back();

	const auto& function = cursor.stack().back();
	auto stateful_signatures = std::views::transform(function.data<Function>().mapping, [](auto& signature) {
		return signature.second.template context<Function::Stateful>();
	}) | std::views::filter([](auto* context) {
		return context != nullptr;
	}) | std::views::transform([](auto* context) -> Function::Stateful& {
		return *context;
	});

	for (auto& signature : stateful_signatures) {
		if ((reference.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
			signature.capture(symbol, WeakReference(copy_from, Reference::default_flags, reference.data()));
		}
		else {
			signature.capture(symbol, WeakReference(Reference::default_flags, reference.data()));
		}
	}
}

void mint::capture_all_symbols(Cursor& cursor) {

	const auto& function = cursor.stack().back();
	auto stateful_signatures = std::views::transform(function.data<Function>().mapping, [](auto& signature) {
		return signature.second.template context<Function::Stateful>();
	}) | std::views::filter([](auto* context) {
		return context != nullptr;
	}) | std::views::transform([](auto* context) -> Function::Stateful& {
		return *context;
	});

	for (auto& signature : stateful_signatures) {
		for (auto& item : cursor.symbols()) {
			signature.capture(item.first, item.second);
		}
	}
}

void mint::init_call(Cursor& cursor) {
	if (cursor.stack().back().data().format() != Data::object_format) {
		cursor.waiting_calls().emplace(std::move(cursor.stack().back()));
		cursor.stack().pop_back();
	}
	else {
		setup_member_call(cursor, cursor.stack().back());
	}
}

void mint::init_call(Cursor& cursor, const Reference& function) {
	if (function.data().format() != Data::object_format) {
		cursor.waiting_calls().emplace(function);
	}
	else {
		WeakReference member = function;
		setup_member_call(cursor, member);
	}
}

void mint::init_member_call(Cursor& cursor, const Symbol& member) {

	auto [function, owner] = get_member(cursor, cursor.stack().back(), member);

	if (function.flags() & Reference::global) {
		cursor.stack().pop_back();
	}

	if (function.data().format() != Data::object_format) {
		cursor.waiting_calls().emplace(std::move(function), owner);
	}
	else if (setup_member_call(cursor, function).get_flags() & Cursor::Call::operator_call) {
		cursor.stack().back() = std::move(function);
	}
	else {
		cursor.stack().emplace_back(std::move(function));
	}
}

void mint::init_operator_call(Cursor& cursor, Class::Operator op) {

	auto [function, owner] = get_operator(cursor, cursor.stack().back(), op);

	if (function.flags() & Reference::global) {
		cursor.stack().pop_back();
	}

	if (function.data().format() != Data::object_format) {
		cursor.waiting_calls().emplace(std::move(function), owner);
	}
	else if (setup_member_call(cursor, function).get_flags() & Cursor::Call::operator_call) {
		cursor.stack().back() = std::move(function);
	}
	else {
		cursor.stack().emplace_back(std::move(function));
	}
}

void mint::exit_call(Cursor& cursor) {
	cursor.exit_call();
}

void mint::init_exception(Cursor& cursor, const Symbol& symbol) {

	const auto& value = cursor.stack().back();
	SymbolTable& symbols = cursor.symbols();

	if (!ensure_not_defined(symbol, symbols)) {
		error("symbol '{}' was already defined in this context", symbol.str());
	}

	symbols.emplace(symbol, value);
	cursor.stack().pop_back();
}

void mint::reset_exception(Cursor& cursor, const Symbol& symbol) {
	SymbolTable& symbols = cursor.symbols();
	symbols.erase(symbol);
}

void mint::init_parameter(Cursor& cursor, const Symbol& symbol, mint::Reference::Flags flags, std::size_t index) {

	const auto& value = cursor.stack().back();
	SymbolTable& symbols = cursor.symbols();

	if ((flags & Reference::const_value)
	    || !((value.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value)) {
		symbols.setup_fast(symbol, index, flags).move_data(value);
	}
	else {
		symbols.setup_fast(symbol, index, flags).copy_data(value);
	}

	cursor.stack().pop_back();
}

Function::Mapping::const_iterator mint::find_function_signature(Cursor& cursor, Function::Mapping& mapping,
    int signature) {

	auto it = mapping.find(signature);

	if (it != mapping.end()) {
		return it;
	}

	it = mapping.lower_bound(~signature);

	if (it != mapping.end()) {

		auto& stack = cursor.stack();
		const int required = ~it->first;

		if (required < 0) [[unlikely]] {
			return mapping.end();
		}

		auto* va_args = GarbageCollector::instance().alloc<Iterator>(cursor.ast());
		va_args->construct();

		const auto from = std::prev(stack.end(), signature - required);
		const auto to = stack.end();
		for (auto it = from; it != to; ++it) {
			va_args->ctx.yield(std::move(*it));
		}

		stack.erase(from, to);
		stack.emplace_back(Reference::default_flags, *va_args);
	}

	return it;
}

bool mint::has_signature(Function::Mapping& mapping, int signature) {

	if (auto it = mapping.find(signature); it != mapping.end()) {
		return true;
	}

	if (auto it = mapping.lower_bound(~signature); it != mapping.end()) {
		return true;
	}

	return false;
}

bool mint::has_signature(const Reference& reference, int signature) {
	switch (reference.data().format()) {
	case Data::none_format:
	case Data::null_format:
	case Data::number_format:
	case Data::boolean_format:
		return signature == 0;
	case Data::object_format:
		if (is_object(reference.data<Object>())) {
			if (auto* op = reference.data<Object>().metadata.find_operator(Class::call_operator)) {
				return has_signature(Class::MemberInfo::get(*op, reference.data<Object>()), signature);
			}
		}
		else {
			if (auto* op = reference.data<Object>().metadata.find_operator(Class::new_operator)) {
				return has_signature(op->value, signature);
			}
		}
		return signature == 0;
	case Data::package_format:
		return false;
	case Data::function_format:
		return has_signature(reference.data<Function>().mapping, signature);
	}
	return false;
}

WeakReference mint::get_symbol(Cursor& cursor, const Symbol& symbol) {
	return get_symbol(cursor.symbols(), cursor.ast().global_data(), symbol);
}

WeakReference mint::get_symbol(SymbolTable& symbols, GlobalData& globals, const Symbol& symbol) {
	if (auto it = symbols.find(symbol); it != symbols.end()) {
		return it->second;
	}
	if (auto it = globals.symbols().find(symbol); it != globals.symbols().end()) {
		return it->second;
	}
	return symbols[symbol];
}

namespace {

Reference& get_accessible_member(Cursor& cursor, const Symbol& member, Class::MemberInfo& info, Object& object) {
	Reference& result = Class::MemberInfo::get(info, object);
	switch (result.flags() & Reference::visibility_mask) {
	case Reference::protected_visibility:
		if (!is_protected_accessible(cursor, info.owner)) [[unlikely]] {
			error("could not access protected member '{}' of class '{}'", member.str(), object.metadata.full_name());
		}
		break;
	case Reference::private_visibility:
		if (!is_private_accessible(cursor, info.owner)) [[unlikely]] {
			error("could not access private member '{}' of class '{}'", member.str(), object.metadata.full_name());
		}
		break;
	case Reference::package_visibility:
		if (!is_package_accessible(cursor, info.owner)) [[unlikely]] {
			error("could not access package member '{}' of class '{}'", member.str(), object.metadata.full_name());
		}
		break;
	default:
		break;
	}
	return result;
}

Reference& get_accessible_class_member(Cursor& cursor, const Symbol& member, Class::MemberInfo& info, Class& metadata) {
	if (cursor.is_in_builtin() || cursor.symbols().get_metadata() == nullptr) [[unlikely]] {
		error("could not access member '{}' of class '{}' without object", member.str(), metadata.full_name());
	}
	if (auto* context_metadata = cursor.symbols().get_metadata();
	    context_metadata && !metadata.is_direct_base_or_same(*context_metadata)) [[unlikely]] {
		error("class '{}' is not a direct base of '{}'", metadata.full_name(),
		    cursor.symbols().get_metadata()->full_name());
	}
	if ((info.value.flags() & Reference::private_visibility) && (&info.owner.get() != cursor.symbols().get_metadata()))
	    [[unlikely]] {
		error("could not access private member '{}' of class '{}'", member.str(), metadata.full_name());
	}
	return info.value;
}

Reference& get_accessible_global_member(Cursor& cursor, const Symbol& member, Class::MemberInfo& info, Class& metadata) {
	if (info.value.data().format() == Data::none_format) {
		return info.value;
	}
	switch (info.value.flags() & Reference::visibility_mask) {
	case Reference::protected_visibility:
		if (is_protected_accessible(cursor, info.owner)) [[likely]] {
			return info.value;
		}
		if (!is_class(info.value)) [[unlikely]] {
			error("could not access protected member '{}' of class '{}'", member.str(), metadata.full_name());
		}
		if (!is_protected_accessible(cursor, info.value.data<Object>().metadata)) [[unlikely]] {
			error("could not access protected member type '{}' of class '{}'", member.str(), metadata.full_name());
		}
		break;
	case Reference::private_visibility:
		if (is_private_accessible(cursor, info.owner)) [[likely]] {
			return info.value;
		}
		if (!is_class(info.value)) [[unlikely]] {
			error("could not access private member '{}' of class '{}'", member.str(), metadata.full_name());
		}
		if (!is_private_accessible(cursor, info.value.data<Object>().metadata)) [[unlikely]] {
			error("could not access private member type '{}' of class '{}'", member.str(), metadata.full_name());
		}
		break;
	case Reference::package_visibility:
		if (is_package_accessible(cursor, info.owner)) [[likely]] {
			return info.value;
		}
		if (!is_class(info.value)) [[unlikely]] {
			error("could not access package member '{}' of class '{}'", member.str(), metadata.full_name());
		}
		if (!is_package_accessible(cursor, info.value.data<Object>().metadata)) [[unlikely]] {
			error("could not access package member type '{}' of class '{}'", member.str(), metadata.full_name());
		}
		break;
	default:
		break;
	}
	return info.value;
}

Reference& get_accessible_member(Cursor& cursor, Class::Operator op, Class::MemberInfo& info, Object& object) {
	Reference& result = Class::MemberInfo::get(info, object);
	switch (result.flags() & Reference::visibility_mask) {
	case Reference::protected_visibility:
		if (!is_protected_accessible(cursor, info.owner)) [[unlikely]] {
			error("could not access protected member '{}' of class '{}'", get_operator_symbol(op).str(),
			    object.metadata.full_name());
		}
		break;
	case Reference::private_visibility:
		if (!is_private_accessible(cursor, info.owner)) [[unlikely]] {
			error("could not access private member '{}' of class '{}'", get_operator_symbol(op).str(),
			    object.metadata.full_name());
		}
		break;
	case Reference::package_visibility:
		if (!is_package_accessible(cursor, info.owner)) [[unlikely]] {
			error("could not access package member '{}' of class '{}'", get_operator_symbol(op).str(),
			    object.metadata.full_name());
		}
		break;
	default:
		break;
	}
	return result;
}

Reference& get_accessible_class_member(Cursor& cursor, Class::Operator op, Class::MemberInfo& info, Class& metadata) {
	if (cursor.is_in_builtin() || cursor.symbols().get_metadata() == nullptr) [[unlikely]] {
		error("could not access member '{}' of class '{}' without object", get_operator_symbol(op).str(),
		    metadata.full_name());
	}
	if (auto* context_metadata = cursor.symbols().get_metadata();
	    context_metadata && !metadata.is_direct_base_or_same(*context_metadata)) [[unlikely]] {
		error("class '{}' is not a direct base of '{}'", metadata.full_name(),
		    cursor.symbols().get_metadata()->full_name());
	}
	if ((info.value.flags() & Reference::private_visibility) && (&info.owner.get() != cursor.symbols().get_metadata()))
	    [[unlikely]] {
		error("could not access private member '{}' of class '{}'", get_operator_symbol(op).str(), metadata.full_name());
	}
	return info.value;
}

}

std::tuple<WeakReference, Class*> mint::get_member(Cursor& cursor, const Reference& reference, const Symbol& member) {

	switch (reference.data().format()) {
	case Data::package_format:
		for (PackageData* package = &reference.data<Package>().data; package != nullptr;
		    package = package->get_owner_package()) {
			if (auto it = package->symbols().find(member); it != package->symbols().end()) {
				return {it->second, nullptr};
			}
		}

		error("package '{}' has no member '{}'", reference.data<Package>().data.name().str(), member.str());
		break;

	case Data::object_format:
		{
			auto& object = reference.data<Object>();
			if (auto* info = object.metadata.find_member(member)) {
				if (is_object(object)) {
					const auto& result = get_accessible_member(cursor, member, *info, object);
					return {result, &info->owner.get()};
				}

				constexpr auto flags = Reference::const_address | Reference::const_value | Reference::global;
				const auto& result = get_accessible_class_member(cursor, member, *info, object.metadata);
				return {WeakReference(flags, result.data()), &info->owner.get()};
			}

			if (auto* info = object.metadata.find_global(member)) {
				const auto& result = get_accessible_global_member(cursor, member, *info, object.metadata);
				return {result, &info->owner.get()};
			}

			for (PackageData* package = &object.metadata.get_package(); package != nullptr;
			    package = package->get_owner_package()) {
				if (auto it = package->symbols().find(member); it != package->symbols().end()) {
					return {WeakReference(Reference::const_address | Reference::const_value, it->second.data()),
					    nullptr};
				}
			}

			if (is_object(object)) {
				error("class '{}' has no member '{}'", object.metadata.full_name(), member.str());
			}
			else {
				error("class '{}' has no global member '{}'", object.metadata.full_name(), member.str());
			}
		}
		break;

	default:
		GlobalData& externals = cursor.ast().global_data();
		if (auto it = externals.symbols().find(member); it != externals.symbols().end()) {
			return {WeakReference(Reference::const_address | Reference::const_value, it->second.data()), nullptr};
		}
		error("non class values doesn't have member '{}'", member.str());
	}

	return {};
}

std::tuple<WeakReference, Class*> mint::get_operator(Cursor& cursor, const Reference& reference, Class::Operator op) {

	switch (reference.data().format()) {
	case Data::object_format:
		if (auto& object = reference.data<Object>(); Class::MemberInfo* info = object.metadata.find_operator(op)) {

			if (is_object(object)) {
				const auto& result = get_accessible_member(cursor, op, *info, object);
				return {result, &info->owner.get()};
			}

			constexpr auto flags = Reference::const_address | Reference::const_value | Reference::global;
			const auto& result = get_accessible_class_member(cursor, op, *info, object.metadata);
			return {WeakReference(flags, result.data()), &info->owner.get()};
		}

		if (is_object(reference.data<Object>())) {
			error("class '{}' has no member '{}'", reference.data<Object>().metadata.full_name(),
			    get_operator_symbol(op).str());
		}
		else {
			error("class '{}' has no global member '{}'", reference.data<Object>().metadata.full_name(),
			    get_operator_symbol(op).str());
		}

	default:
		error("non class values doesn't have member '{}'", get_operator_symbol(op).str());
	}
}

void mint::reduce_member(Cursor& cursor, Reference&& member) {
	cursor.stack().back() = std::move(member);
}

Class::MemberInfo* mint::find_member_info(Object& object, const Reference& member) {

	const auto& data_ref = member.data();

	if (object.data) {
		for (auto [_, info] : object.metadata.members()) {
			if (&data_ref == &info.get().value.data()) {
				return &info.get();
			}
			if (&data_ref == &Class::MemberInfo::get(info.get(), object).data()) {
				return &info.get();
			}
		}
	}
	else {
		for (auto [_, info] : object.metadata.members()) {
			if (&data_ref == &info.get().value.data()) {
				return &info.get();
			}
		}
	}

	return nullptr;
}

bool mint::is_protected_accessible(const Class& owner, const Class* context) {
	return context && (owner.is_base_or_same(*context) || context->is_base_of(owner));
}

bool mint::is_protected_accessible(const Cursor& cursor, const Class& owner) {
	return !cursor.is_in_builtin() && is_protected_accessible(owner, cursor.symbols().get_metadata());
}

bool mint::is_private_accessible(const Cursor& cursor, const Class& owner) {
	return !cursor.is_in_builtin() && &owner == cursor.symbols().get_metadata();
}

bool mint::is_package_accessible(const Cursor& cursor, const Class& owner) {
	return !cursor.is_in_builtin() && &owner.get_package() == &cursor.symbols().get_package();
}

Symbol mint::var_symbol(Cursor& cursor) {
	const auto var = std::move(cursor.stack().back());
	cursor.stack().pop_back();
	return Symbol(to_string(var));
}

void mint::declare_symbol(Cursor& cursor, const Symbol& symbol, Reference::Flags flags) {

	if (flags & Reference::global) {

		PackageData& package = cursor.symbols().get_package();

		if (!ensure_not_defined(symbol, package.symbols())) [[unlikely]] {
			error("symbol '{}' was already defined in global context", symbol.str());
		}

		cursor.stack().emplace_back(package.symbols().emplace(symbol, WeakReference(flags)).first->second);
	}
	else {

		if (!ensure_not_defined(symbol, cursor.symbols())) [[unlikely]] {
			error("symbol '{}' was already defined in this context", symbol.str());
		}

		cursor.stack().emplace_back(cursor.symbols().emplace(symbol, WeakReference(flags)).first->second);
	}
}

void mint::declare_symbol(Cursor& cursor, const Symbol& symbol, std::size_t index, Reference::Flags flags) {

	if (flags & Reference::global) {

		PackageData& package = cursor.symbols().get_package();

		if (!ensure_not_defined(symbol, package.symbols())) [[unlikely]] {
			error("symbol '{}' was already defined in global context", symbol.str());
		}

		cursor.stack().emplace_back(
		    package.symbols().emplace(symbol, cursor.symbols().setup_fast(symbol, index, flags)).first->second);
	}
	else {

		if (!ensure_not_defined(symbol, cursor.symbols())) [[unlikely]] {
			error("symbol '{}' was already defined in this context", symbol.str());
		}

		cursor.stack().emplace_back(cursor.symbols().setup_fast(symbol, index, flags));
	}
}

void mint::declare_function(Cursor& cursor, const Symbol& symbol, Reference::Flags flags) {

	assert(flags & Reference::global);

	PackageData& package = cursor.symbols().get_package();

	auto it = package.symbols().find(symbol);
	if (it != package.symbols().end()) {
		switch (it->second.data().format()) {
		case Data::none_format:
			it->second = make_weak_reference<Function>(flags);
			break;
		case Data::function_format:
			if (flags != it->second.flags()) [[unlikely]] {
				error("function '{}' was already defined in global context", symbol.str());
			}
			break;
		default:
			error("symbol '{}' was already defined in this context", symbol.str());
		}
	}
	else {
		it = package.symbols().emplace(symbol, make_weak_reference<Function>(flags)).first;
	}

	cursor.stack().emplace_back(it->second);
}

void mint::function_overload_from_stack(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& signature = load_from_stack(cursor, base);
	const auto& function = load_from_stack(cursor, base - 1);

	for (const auto& item : signature.data<Function>().mapping) {
		if (!function.data<Function>().mapping.insert(item).second) [[unlikely]] {
			error("defined function already takes {}{} parameter(s)", abs(item.first), item.first < 0 ? "..." : "");
		}
	}

	cursor.stack().pop_back();
}
