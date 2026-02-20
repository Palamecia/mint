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

#ifndef MINT_MEMORY_FUNCTIONTOOL_H
#define MINT_MEMORY_FUNCTIONTOOL_H

#include "mint/ast/classregister.h"
#include "mint/ast/cursor.h"
#include "mint/ast/module.h"
#include "mint/ast/symbol.h"
#include "mint/config.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/class.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/assert.h"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <regex>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#ifdef MINT_OS_WINDOWS
#include <BaseTsd.h>
using HANDLE = void*;
using ssize_t = SSIZE_T;
#endif

#define MINT_RAW_FUNCTION(__name, __argc, __cursor) \
	extern "C" MINT_DECL_EXPORT void __name##_##__argc(mint::Cursor& __cursor)

namespace mint::internal {

consteval bool is_valid_signature(std::string_view argc) {
	return std::ranges::all_of(argc, [](char ch) {
		return ch >= '0' && ch <= '9';
	});
}

}

#define MINT_EXPORT_FUNCTION(__func, __argc) \
	extern "C" MINT_DECL_EXPORT void __func##_##__argc(mint::Cursor& cursor) { \
		static_assert(mint::internal::is_valid_signature(#__argc)); \
		static_assert((__argc) == mint::FunctionHelper::FunctionInfo<decltype(__func)>::argc); \
		mint::FunctionHelper::call<__func>(cursor); \
	}

#define MINT_EXPORT_FUNCTION_OVERLOAD(__func, __argc, ...) \
	extern "C" MINT_DECL_EXPORT void __func##_##__argc(mint::Cursor& cursor) { \
		static_assert(mint::internal::is_valid_signature(#__argc)); \
		static_assert((__argc) == mint::FunctionHelper::FunctionInfo<mint::WeakReference(__VA_ARGS__)>::argc); \
		mint::FunctionHelper::call<static_cast<mint::WeakReference (*)(__VA_ARGS__)>(__func)>(cursor); \
	}

namespace mint {

constexpr auto create_flags = Reference::const_address | Reference::const_value | Reference::temporary;

class FunctionHelper;

consteval int variadic(int signature) {
	return ~signature;
}

class MINT_EXPORT ReferenceHelper {
	std::reference_wrapper<const FunctionHelper> _function;
	std::reference_wrapper<Reference> _reference;
public:
	constexpr ReferenceHelper(const FunctionHelper& function, Reference& reference) :
	    _function(function),
	    _reference(reference) {}

	ReferenceHelper operator[](const Symbol& symbol) const;
	[[nodiscard]] ReferenceHelper member(const Symbol& symbol) const;

	constexpr const Reference& operator*() const {
		return _reference;
	}

	constexpr const Reference* operator->() const {
		return &_reference.get();
	}

	[[nodiscard]] constexpr const Reference& get() const {
		return _reference;
	}

	[[nodiscard]] constexpr Reference& get() {
		return _reference;
	}

	[[nodiscard]] WeakReference copy() const;
	[[nodiscard]] WeakReference share();
};

class MINT_EXPORT FunctionHelper {
	std::reference_wrapper<Cursor> _cursor;
	std::size_t _argc;
	std::size_t _top;
public:
	Cursor& cursor();
	std::span<WeakReference> parameters();

	template<class T>
	constexpr T parameter(std::size_t index) {
		assert(index < _argc);
		if constexpr (std::is_same_v<T, ReferenceHelper>) {
			return {*this, load_from_stack(_cursor, _top + index)};
		}
		else {
			return load_from_stack(_cursor, _top + index);
		}
	}

	[[nodiscard]] ReferenceHelper reference(const Symbol& symbol) const {
		SymbolTable& symbols = _cursor.get().ast().global_data().symbols();
		if (auto it = symbols.find(symbol); it != symbols.end()) {
			return {*this, it->second};
		}
		return {*this, mint::GlobalData::none_ref()};
	}

	[[nodiscard]] ReferenceHelper member(const Reference& object, const Symbol& symbol) const {
		auto [member, _] = get_member(_cursor, object, symbol);
		return {*this, _cursor.get().stack().emplace_back(std::move(member))};
	}

	Scheduler& scheduler() {
		auto* scheduler = Scheduler::instance();
		assert_x(scheduler, __func__, "execution should be done using a scheduler");
		assert_x(&scheduler->ast() == &_cursor.get().ast(), __func__, "execution uses a wrong scheduler");
		return *scheduler;
	}

	template<typename T>
	struct FunctionInfo;

	template<std::convertible_to<WeakReference> Ret, typename... Args>
	struct FunctionInfo<Ret (*)(Cursor&, Args...)> {
		static constexpr std::size_t argc = sizeof...(Args);
		static constexpr bool use_helper = false;
	};

	template<std::convertible_to<WeakReference> Ret, typename... Args>
	struct FunctionInfo<Ret(Cursor&, Args...)> : public FunctionInfo<Ret (*)(Cursor&, Args...)> {};

	template<std::convertible_to<WeakReference> Ret, typename... Args>
	struct FunctionInfo<Ret (*)(FunctionHelper&, Args...)> {
		static constexpr std::size_t argc = sizeof...(Args);
		static constexpr bool use_helper = true;
	};

	template<std::convertible_to<WeakReference> Ret, typename... Args>
	struct FunctionInfo<Ret(FunctionHelper&, Args...)> : public FunctionInfo<Ret (*)(FunctionHelper&, Args...)> {};

	template<auto function_ref>
	struct FunctionCaller {
		template<std::convertible_to<WeakReference> Ret, typename... Args>
		static WeakReference call_impl(FunctionHelper& helper, Ret (*func)(Cursor&, Args...)) {
			return [func]<std::size_t... i>(FunctionHelper& helper, std::index_sequence<i...>) {
				return func(helper.cursor(), helper.parameter<Args>(i)...);
			}(helper, std::index_sequence_for<Args...>());
		}

		template<std::convertible_to<WeakReference> Ret, typename... Args>
		static WeakReference call_impl(FunctionHelper& helper, Ret (*func)(FunctionHelper&, Args...)) {
			return [func]<std::size_t... i>(FunctionHelper& helper, std::index_sequence<i...>) {
				return func(helper, helper.parameter<Args>(i)...);
			}(helper, std::index_sequence_for<Args...>());
		}

		static WeakReference call(FunctionHelper& helper) {
			return call_impl(helper, function_ref);
		}
	};

	template<auto function_ref>
	static constexpr void call(Cursor& cursor) {
		auto helper = FunctionHelper(cursor, FunctionInfo<decltype(function_ref)>::argc);
		helper.return_value(FunctionCaller<function_ref>::call(helper));
	}

private:
	FunctionHelper(Cursor& cursor, std::size_t argc);

	void return_value(Reference&& value);
};

MINT_EXPORT WeakReference create_function();
MINT_EXPORT WeakReference create_function(Function::Mapping mapping);
MINT_EXPORT WeakReference create_function(int signature, Function::Signature&& handle);
MINT_EXPORT WeakReference create_function(const std::pair<int, Function::Signature>& mapping);
MINT_EXPORT WeakReference create_function(AbstractSyntaxTree& ast, Module::Info& module, int signature,
    const std::string& function);

MINT_EXPORT WeakReference create_none();
MINT_EXPORT WeakReference create_null();

MINT_EXPORT WeakReference create_number(double value);
MINT_EXPORT WeakReference create_signed_number(std::intmax_t value);
MINT_EXPORT WeakReference create_unsigned_number(std::uintmax_t value);
MINT_EXPORT WeakReference create_boolean(bool value);

MINT_EXPORT WeakReference create_alias(Class& type);
MINT_EXPORT WeakReference create_object(Class& type);

MINT_EXPORT WeakReference create_string(AbstractSyntaxTree& ast);
MINT_EXPORT WeakReference create_string(AbstractSyntaxTree& ast, const char* value);
MINT_EXPORT WeakReference create_string(AbstractSyntaxTree& ast, const std::string& value);
MINT_EXPORT WeakReference create_string(AbstractSyntaxTree& ast, std::string_view value);

MINT_EXPORT WeakReference create_regex(AbstractSyntaxTree& ast);
MINT_EXPORT WeakReference create_regex(AbstractSyntaxTree& ast, const std::string& value);
MINT_EXPORT WeakReference create_regex(AbstractSyntaxTree& ast, const std::string& initializer, const std::regex& value);

MINT_EXPORT WeakReference create_array(AbstractSyntaxTree& ast);
MINT_EXPORT WeakReference create_array(AbstractSyntaxTree& ast, Array::values_type&& values);
MINT_EXPORT WeakReference create_array(AbstractSyntaxTree& ast, std::initializer_list<WeakReference> items);

MINT_EXPORT WeakReference create_hash(AbstractSyntaxTree& ast);
MINT_EXPORT WeakReference create_hash(AbstractSyntaxTree& ast, Hash::values_type&& values);
MINT_EXPORT WeakReference create_hash(AbstractSyntaxTree& ast,
    std::initializer_list<std::pair<WeakReference, WeakReference>> items);

MINT_EXPORT WeakReference create_iterator(AbstractSyntaxTree& ast);
MINT_EXPORT WeakReference create_iterator(FromGenerator from_generator, AbstractSyntaxTree& ast, std::size_t stack_size);
MINT_EXPORT WeakReference create_iterator(FromInclusiveRange from_inclusive_range, AbstractSyntaxTree& ast,
    double begin, double end);
MINT_EXPORT WeakReference create_iterator(FromExclusiveRange from_exclusive_range, AbstractSyntaxTree& ast,
    double begin, double end);

MINT_EXPORT WeakReference create_iterator_over(Cursor& cursor, const Reference& ref);
MINT_EXPORT WeakReference create_iterator_over(Cursor& cursor, Reference&& ref);

template<std::derived_from<Reference>... Items>
WeakReference create_iterator_from(Cursor& cursor, Items... items) {
	WeakReference ref = make_weak_reference<Iterator>(create_flags, cursor.ast(), sizeof...(items));
	(iterator_yield(cursor, ref.data<Iterator>(), std::forward<Items>(items)), ...);
	ref.data<Iterator>().construct();
	return ref;
}

template<class Type>
WeakReference create_c_object(AbstractSyntaxTree& ast, Type* object) {
	WeakReference ref = make_weak_reference<LibObject<Type>>(create_flags, ast, object);
	ref.data<LibObject<Type>>().construct();
	return ref;
}

#ifdef MINT_OS_WINDOWS
using handle_t = HANDLE;
MINT_EXPORT WeakReference create_handle(AbstractSyntaxTree& ast, handle_t handle);
MINT_EXPORT handle_t to_handle(const Reference& reference);
MINT_EXPORT handle_t* to_handle_ptr(const Reference& reference);
#else
using handle_t = int;
MINT_EXPORT WeakReference create_handle(AbstractSyntaxTree& ast, handle_t handle);
MINT_EXPORT handle_t to_handle(const Reference& reference);
MINT_EXPORT handle_t* to_handle_ptr(const Reference& reference);
#endif

// ...

MINT_EXPORT WeakReference get_member_ignore_visibility(AbstractSyntaxTree& ast, const Reference& reference,
    const Symbol& member);
MINT_EXPORT WeakReference get_member_ignore_visibility(PackageData& package, const Symbol& member);
MINT_EXPORT WeakReference get_member_ignore_visibility(Object& object, const Symbol& member);
MINT_EXPORT WeakReference get_global_ignore_visibility(Object& object, const Symbol& global);
MINT_EXPORT WeakReference find_enum_value(Object& object, double value);

}

#endif // MINT_MEMORY_FUNCTIONTOOL_H
