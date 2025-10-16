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

#ifndef MINT_MEMORY_CLASS_H
#define MINT_MEMORY_CLASS_H

#include "mint/ast/module.h"
#include "mint/ast/symbol.h"
#include "mint/config.h"
#include "mint/memory/data.h"
#include "mint/memory/garbagecollector.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <limits>
#include <string>
#include <array>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mint {

class ClassDescription;

class MINT_EXPORT Class : public MemoryRoot {
	friend class ClassDescription;
public:
	enum Metatype : std::uint8_t {
		object,
		string,
		regex,
		array,
		hash,
		iterator,
		library,
		libobject
	};

	static constexpr const std::size_t builtin_class_count = libobject + 1;

	enum Operator : std::uint8_t {
		new_operator,
		delete_operator,
		copy_operator,
		call_operator,
		add_operator,
		sub_operator,
		mul_operator,
		div_operator,
		pow_operator,
		mod_operator,
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
		inc_operator,
		dec_operator,
		not_operator,
		compl_operator,
		shift_left_operator,
		shift_right_operator,
		inclusive_range_operator,
		exclusive_range_operator,
		subscript_operator,
		subscript_move_operator,
		regex_match_operator,
		regex_unmatch_operator
	};

	static constexpr const std::size_t operator_count = regex_unmatch_operator + 1;

	struct MemberInfo {

		static constexpr const std::size_t invalid_offset = std::numeric_limits<std::size_t>::max();
		static inline WeakReference& get(MemberInfo& member, WeakReference* data);
		static inline WeakReference& get(MemberInfo& member, Object& object);

		std::size_t offset;
		std::reference_wrapper<Class> owner;
		WeakReference value;
	};

	Class(PackageData& package, std::string name, Metatype metatype = object);
	Class(Class&&) = delete;
	Class(const Class&) = delete;
	~Class() override;

	Class& operator=(Class&&) = delete;
	Class& operator=(const Class&) = delete;

	[[nodiscard]] inline Metatype metatype() const;
	[[nodiscard]] inline const std::string& full_name() const;
	[[nodiscard]] Symbol name() const;

	[[nodiscard]] PackageData& get_package() const;
	[[nodiscard]] ClassDescription& get_description() const;
	[[nodiscard]] inline MemberInfo* find_operator(Operator op) const;
	[[nodiscard]] inline MemberInfo* find_member(const Symbol& symbol) const;
	[[nodiscard]] inline MemberInfo* find_global(const Symbol& symbol) const;
	[[nodiscard]] MemberInfo* find_class(const Symbol& name) const;

	[[nodiscard]] inline const std::vector<std::reference_wrapper<MemberInfo>>& slots() const;
	[[nodiscard]] std::size_t size() const;

	[[nodiscard]] auto members() {
		return std::views::transform(_members, [](auto& item) -> std::pair<Symbol, std::reference_wrapper<MemberInfo>> {
			return {item.first, *item.second};
		});
	}

	[[nodiscard]] auto globals() {
		return std::views::transform(_globals, [](auto& item) -> std::pair<Symbol, std::reference_wrapper<MemberInfo>> {
			return {item.first, *item.second};
		});
	}

	[[nodiscard]] auto classes() {
		return std::views::filter(_globals, [](auto& item) {
			return item.second->value.data().format() == Data::object_format
			       && item.second->value.template data<Object>().data == nullptr;
		}) | std::views::transform([](auto& item) -> std::pair<Symbol, std::reference_wrapper<MemberInfo>> {
			return {item.first, *item.second};
		});
	}

	[[nodiscard]] const std::vector<std::reference_wrapper<Class>>& bases() const;
	[[nodiscard]] bool is_same(const Class& other) const;
	[[nodiscard]] bool is_base_of(const Class& other) const;
	[[nodiscard]] bool is_base_or_same(const Class& other) const;
	[[nodiscard]] bool is_direct_base_or_same(const Class& other) const;

	[[nodiscard]] bool is_copyable() const;
	void disable_copy();

	void cleanup_memory();
	void cleanup_metadata();

protected:
	void create_builtin_member(Operator op, WeakReference&& value = {});
	void create_builtin_member(Operator op, std::pair<int, Module::Handle&> member);
	void create_builtin_member(const Symbol& symbol, WeakReference&& value = {});
	void create_builtin_member(const Symbol& symbol, std::pair<int, Module::Handle&> member);

	void mark() override {
		for (auto& member : _members) {
			member.second->value.data().mark();
		}
		for (auto& global : _globals) {
			global.second->value.data().mark();
		}
	}

private:
	Metatype _metatype;
	bool _copyable = true;

	std::string _name;
	std::reference_wrapper<PackageData> _package;
	ClassDescription* _description = nullptr;

	std::array<MemberInfo*, operator_count> _operators;
	std::vector<std::reference_wrapper<MemberInfo>> _slots;
	std::unordered_map<Symbol, std::unique_ptr<MemberInfo>> _members;
	std::unordered_map<Symbol, std::unique_ptr<MemberInfo>> _globals;
};

WeakReference& Class::MemberInfo::get(MemberInfo& member, WeakReference* data) {
	return member.offset == invalid_offset ? member.value : data[member.offset];
}

WeakReference& Class::MemberInfo::get(MemberInfo& member, Object& object) {
	return member.offset == invalid_offset ? member.value : object.data[member.offset];
}

Class::Metatype Class::metatype() const {
	return _metatype;
}

const std::string& Class::full_name() const {
	return _name;
}

Class::MemberInfo* Class::find_operator(Operator op) const {
	return _operators[op];
}

Class::MemberInfo* Class::find_member(const Symbol& symbol) const {
	if (auto it = _members.find(symbol); it != _members.end()) {
		return it->second.get();
	}
	return nullptr;
}

Class::MemberInfo* Class::find_global(const Symbol& symbol) const {
	if (auto it = _globals.find(symbol); it != _globals.end()) {
		return it->second.get();
	}
	return nullptr;
}

const std::vector<std::reference_wrapper<Class::MemberInfo>>& Class::slots() const {
	return _slots;
}

MINT_EXPORT Symbol get_operator_symbol(Class::Operator op);
MINT_EXPORT std::optional<Class::Operator> get_symbol_operator(const Symbol& symbol);

}

#endif // MINT_MEMORY_CLASS_H
