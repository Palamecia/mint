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

#include "mint/memory/functiontool.h"
#include "mint/ast/classregister.h"
#include "mint/ast/cursor.h"
#include "mint/ast/module.h"
#include "mint/ast/symbol.h"
#include "mint/compiler/compiler.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/data.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/operatortool.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/bufferstream.h"
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <regex>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using namespace mint;

ReferenceHelper ReferenceHelper::operator[](const Symbol& symbol) const {
	return _function.get().member(_reference, symbol);
}

ReferenceHelper ReferenceHelper::member(const Symbol& symbol) const {
	return _function.get().member(_reference, symbol);
}

WeakReference ReferenceHelper::copy() const {
	return {copy_from, _reference};
}

WeakReference ReferenceHelper::share() {
	return _reference.get();
}

FunctionHelper::FunctionHelper(Cursor& cursor, std::size_t argc) :
    _cursor(cursor),
    _argc(argc),
    _top(cursor.stack().size() - _argc) {}

Cursor& FunctionHelper::cursor() {
	return _cursor.get();
}

std::span<WeakReference> FunctionHelper::parameters() {
	return {std::next(_cursor.get().stack().data(), static_cast<ssize_t>(_top)), _argc};
}

void FunctionHelper::return_value(Reference&& value) {
	_cursor.get().stack().resize(_top);
	_cursor.get().stack().emplace_back(std::move(value));
}

WeakReference mint::create_function() {
	return make_weak_reference<Function>(create_flags);
}

WeakReference mint::create_function(Function::Mapping mapping) {
	return make_weak_reference<Function>(create_flags, std::move(mapping));
}

WeakReference mint::create_function(int signature, Function::Signature&& handle) {
	return make_weak_reference<Function>(create_flags, signature, std::move(handle));
}

WeakReference mint::create_function(const std::pair<int, Function::Signature>& mapping) {
	return make_weak_reference<Function>(create_flags, mapping);
}

WeakReference mint::create_function(AbstractSyntaxTree& ast, Module::Info& module, int signature,
    const std::string& function) {

	const std::size_t offset = module.module->end() + 3;

	auto compiler = Compiler(ast);
	auto stream = BufferStream(function);
	if (!compiler.build(stream, module)) {
		return {};
	}

	return make_weak_reference<Function>(create_flags, signature,
	    std::make_unique<Function::Stateless>(*module.module->find_handle(offset)));
}

WeakReference mint::create_none() {
	return make_weak_reference<None>(create_flags);
}

WeakReference mint::create_null() {
	return make_weak_reference<Null>(create_flags);
}

WeakReference mint::create_number(double value) {
	return make_weak_reference<Number>(create_flags, value);
}

WeakReference mint::create_signed_number(std::intmax_t value) {
	return make_weak_reference<Number>(create_flags, value);
}

WeakReference mint::create_unsigned_number(std::uintmax_t value) {
	return make_weak_reference<Number>(create_flags, value);
}

WeakReference mint::create_boolean(bool value) {
	return make_weak_reference<Boolean>(create_flags, value);
}

WeakReference mint::create_alias(Class& type) {
	return make_weak_reference<Object>(create_flags, type);
}

WeakReference mint::create_object(Class& type) {
	WeakReference ref = make_weak_reference<Object>(create_flags, type);
	ref.data<String>().construct();
	return ref;
}

WeakReference mint::create_string(AbstractSyntaxTree& ast) {
	WeakReference ref = make_weak_reference<String>(create_flags, ast);
	ref.data<String>().construct();
	return ref;
}

WeakReference mint::create_string(AbstractSyntaxTree& ast, const char* value) {
	WeakReference ref = make_weak_reference<String>(create_flags, ast, value);
	ref.data<String>().construct();
	return ref;
}

WeakReference mint::create_string(AbstractSyntaxTree& ast, const std::string& value) {
	WeakReference ref = make_weak_reference<String>(create_flags, ast, value);
	ref.data<String>().construct();
	return ref;
}

WeakReference mint::create_string(AbstractSyntaxTree& ast, std::string_view value) {
	WeakReference ref = make_weak_reference<String>(create_flags, ast, value);
	ref.data<String>().construct();
	return ref;
}

WeakReference mint::create_regex(AbstractSyntaxTree& ast) {
	WeakReference ref = make_weak_reference<Regex>(create_flags, ast);
	ref.data<Regex>().construct();
	return ref;
}

WeakReference mint::create_regex(AbstractSyntaxTree& ast, const std::string& value) {
	WeakReference ref = make_weak_reference<Regex>(create_flags, ast);
	ref.data<Regex>().initializer = "/" + value + "/";
	ref.data<Regex>().expr = value;
	ref.data<Regex>().construct();
	return ref;
}

WeakReference mint::create_regex(AbstractSyntaxTree& ast, const std::string& initializer, const std::regex& value) {
	WeakReference ref = make_weak_reference<Regex>(create_flags, ast);
	ref.data<Regex>().initializer = "/" + initializer + "/";
	ref.data<Regex>().expr = value;
	ref.data<Regex>().construct();
	return ref;
}

WeakReference mint::create_array(AbstractSyntaxTree& ast) {
	WeakReference ref = make_weak_reference<Array>(create_flags, ast);
	ref.data<Array>().construct();
	return ref;
}

WeakReference mint::create_array(AbstractSyntaxTree& ast, Array::values_type&& values) {
	WeakReference ref = make_weak_reference<Array>(create_flags, ast);
	ref.data<Array>().values = std::move(values);
	ref.data<Array>().construct();
	return ref;
}

WeakReference mint::create_array(AbstractSyntaxTree& ast, std::initializer_list<WeakReference> items) {
	WeakReference ref = make_weak_reference<Array>(create_flags, ast);
	ref.data<Array>().values.reserve(items.size());
	for (const auto& item : items) {
		array_append(ref.data<Array>(), array_item(item));
	}
	ref.data<Array>().construct();
	return ref;
}

WeakReference mint::create_hash(AbstractSyntaxTree& ast) {
	WeakReference ref = make_weak_reference<Hash>(create_flags, ast);
	ref.data<Hash>().construct();
	return ref;
}

WeakReference mint::create_hash(AbstractSyntaxTree& ast, Hash::values_type&& values) {
	WeakReference ref = make_weak_reference<Hash>(create_flags, ast);
	ref.data<Hash>().values = std::move(values);
	ref.data<Hash>().construct();
	return ref;
}

WeakReference mint::create_hash(AbstractSyntaxTree& ast,
    std::initializer_list<std::pair<WeakReference, WeakReference>> items) {
	WeakReference ref = make_weak_reference<Hash>(create_flags, ast);
	ref.data<Hash>().values.reserve(items.size());
	for (const auto& item : items) {
		hash_insert(ref.data<Hash>(), item.first, item.second);
	}
	ref.data<Hash>().construct();
	return ref;
}

WeakReference mint::create_iterator(AbstractSyntaxTree& ast) {
	WeakReference ref = make_weak_reference<Iterator>(create_flags, ast);
	ref.data<Iterator>().construct();
	return ref;
}

WeakReference mint::create_iterator(FromGenerator from_generator, AbstractSyntaxTree& ast, std::size_t stack_size) {
	WeakReference ref = make_weak_reference<Iterator>(create_flags, from_generator, ast, stack_size);
	ref.data<Iterator>().construct();
	return ref;
}

WeakReference mint::create_iterator(FromInclusiveRange from_inclusive_range, AbstractSyntaxTree& ast, double begin,
    double end) {
	WeakReference ref = make_weak_reference<Iterator>(create_flags, from_inclusive_range, ast, begin, end);
	ref.data<Iterator>().construct();
	return ref;
}

WeakReference mint::create_iterator(FromExclusiveRange from_exclusive_range, AbstractSyntaxTree& ast, double begin,
    double end) {
	WeakReference ref = make_weak_reference<Iterator>(create_flags, from_exclusive_range, ast, begin, end);
	ref.data<Iterator>().construct();
	return ref;
}

WeakReference mint::create_iterator_over(AbstractSyntaxTree& ast, const Reference& ref) {

	if (is_instance_of(ref, Class::iterator)) {
		return ref;
	}

	auto iterator = make_weak_reference<Iterator>(create_flags, ast, ref);
	iterator.data<Iterator>().construct();
	return iterator;
}

WeakReference mint::create_iterator_over(AbstractSyntaxTree& ast, Reference&& ref) {

	if (is_instance_of(ref, Class::iterator)) {
		return std::move(ref);
	}

	auto iterator = make_weak_reference<Iterator>(create_flags, ast, std::move(ref));
	iterator.data<Iterator>().construct();
	return iterator;
}

#ifdef MINT_OS_WINDOWS
WeakReference mint::create_handle(AbstractSyntaxTree& ast, mint::handle_t handle) {
	WeakReference ref = make_weak_reference<LibObject<std::remove_pointer_t<mint::handle_t>>>(create_flags, ast, handle);
	ref.data<LibObject<std::remove_pointer_t<mint::handle_t>>>().construct();
	return ref;
}

mint::handle_t mint::to_handle(const Reference& reference) {
	return reference.data<LibObject<std::remove_pointer_t<HANDLE>>>().ptr;
}

mint::handle_t* mint::to_handle_ptr(const Reference& reference) {
	return &reference.data<LibObject<std::remove_pointer_t<HANDLE>>>().ptr;
}
#else
WeakReference mint::create_handle(AbstractSyntaxTree& ast, mint::handle_t handle) {
	WeakReference ref = make_weak_reference<LibObject<void>>(create_flags, ast, reinterpret_cast<void*>(handle));
	ref.data<LibObject<void>>().construct();
	return ref;
}

mint::handle_t mint::to_handle(const Reference& reference) {
	return static_cast<handle_t>(std::bit_cast<intptr_t>(reference.data<LibObject<void>>().ptr));
}

mint::handle_t* mint::to_handle_ptr(const Reference& reference) {
	return std::bit_cast<handle_t*>(&reference.data<LibObject<void>>().ptr);
}
#endif

WeakReference mint::get_member_ignore_visibility(AbstractSyntaxTree& ast, const Reference& reference,
    const Symbol& member) {

	switch (reference.data().format()) {
	case Data::package_format:
		for (PackageData* package_data = &reference.data<Package>().data; package_data != nullptr;
		    package_data = package_data->get_owner_package()) {
			if (auto it = package_data->symbols().find(member); it != package_data->symbols().end()) {
				return it->second;
			}
		}
		break;

	case Data::object_format:
		{
			auto& object = reference.data<Object>();

			if (auto* info = object.metadata.find_member(member)) {
				if (is_object(object)) {
					return Class::MemberInfo::get(*info, object);
				}
				return {Reference::const_address | Reference::const_value | Reference::global, info->value.data()};
			}

			if (auto* info = object.metadata.find_global(member)) {
				return info->value;
			}

			for (PackageData* package = &object.metadata.get_package(); package != nullptr;
			    package = package->get_owner_package()) {
				if (auto it = package->symbols().find(member); it != package->symbols().end()) {
					return {Reference::const_address | Reference::const_value, it->second.data()};
				}
			}
		}
		break;

	default:
		GlobalData& externals = ast.global_data();
		if (auto it = externals.symbols().find(member); it != externals.symbols().end()) {
			return {Reference::const_address | Reference::const_value, it->second.data()};
		}
	}

	return {};
}

WeakReference mint::get_member_ignore_visibility(PackageData& package, const Symbol& member) {
	for (PackageData* package_data = &package; package_data != nullptr;
	    package_data = package_data->get_owner_package()) {
		if (auto it = package_data->symbols().find(member); it != package_data->symbols().end()) {
			return it->second;
		}
	}
	return {};
}

WeakReference mint::get_member_ignore_visibility(Object& object, const Symbol& member) {
	if (auto* info = object.metadata.find_member(member)) {
		return Class::MemberInfo::get(*info, object);
	}
	return {};
}

WeakReference mint::get_global_ignore_visibility(Object& object, const Symbol& global) {
	if (auto* info = object.metadata.find_global(global)) {
		return info->value;
	}
	return {};
}

WeakReference mint::find_enum_value(Object& object, double value) {
	for (auto [symbol, info] : object.metadata.globals()) {
		auto& value_ref = info.get().value;
		if (is_instance_of(value_ref, Data::number_format) && value_ref.data<Number>().value == value) {
			return value_ref;
		}
	}
	return {};
}
