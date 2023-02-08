/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#ifndef MINT_CLASS_H
#define MINT_CLASS_H

#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/ast/symbolmapping.hpp"

#include <optional>
#include <limits>
#include <string>
#include <array>

namespace mint {

class ClassDescription;

class MINT_EXPORT Class : public MemoryRoot {
	friend class ClassDescription;
public:
	enum Metatype {
		object,
		string,
		regex,
		array,
		hash,
		iterator,
		library,
		libobject
	};

	enum Operator {
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
	static constexpr const size_t operator_count = regex_unmatch_operator + 1;

	struct MemberInfo {

		static constexpr const size_t invalid_offset = std::numeric_limits<size_t>::max();
		static inline WeakReference &get(MemberInfo *member, WeakReference *data);
		static inline WeakReference &get(MemberInfo *member, Object *object);

		size_t offset;
		Class *owner;
		WeakReference value;
	};

	using MembersMapping = SymbolMapping<MemberInfo *>;

	Class(const std::string &name, Metatype metatype = object);
	Class(PackageData *package, const std::string &name, Metatype metatype = object);
	virtual ~Class();

	MemberInfo *get_class(const Symbol &name);
	Object *make_instance();

	inline Metatype metatype() const;
	inline std::string full_name() const;
	Symbol name() const;

	PackageData *get_package() const;
	ClassDescription *get_description() const;
	inline MemberInfo *find_operator(Operator op) const;

	inline std::vector<MemberInfo *> &slots();
	inline MembersMapping &members();
	inline MembersMapping &globals();
	size_t size() const;

	const std::vector<Class *> &bases() const;
	bool is_base_of(const Class *other) const;
	bool is_base_or_same(const Class *other) const;
	bool is_direct_base_or_same(const Class *other) const;

	bool is_copyable() const;
	void disable_copy();

	void cleanup_memory();
	void cleanup_metadata();

protected:
	void create_builtin_member(Operator op, WeakReference &&value = WeakReference());
	void create_builtin_member(Operator op, std::pair<int, Module::Handle *> member);
	void create_builtin_member(const Symbol &symbol, WeakReference &&value = WeakReference());
	void create_builtin_member(const Symbol &symbol, std::pair<int, Module::Handle *> member);

	void mark() override {
		for (MemberInfo *op : m_operators) {
			if (op) {
				op->value.data()->mark();
			}
		}
		for (auto it = m_members.begin(); it != m_members.end(); ++it) {
			it->second->value.data()->mark();
		}
		for (auto it = m_globals.begin(); it != m_globals.end(); ++it) {
			it->second->value.data()->mark();
		}
	}

private:
	Metatype m_metatype;
	bool m_copyable = true;

	std::string m_name;
	PackageData *m_package;
	ClassDescription *m_description = nullptr;

	std::array<MemberInfo *, operator_count> m_operators;
	std::vector<MemberInfo *> m_slots;
	MembersMapping m_members;
	MembersMapping m_globals;
};

WeakReference &Class::MemberInfo::get(MemberInfo *member, WeakReference *data) {
	return member->offset == invalid_offset ? member->value : data[member->offset];
}

WeakReference &Class::MemberInfo::get(MemberInfo *member, Object *object) {
	return member->offset == invalid_offset ? member->value : object->data[member->offset];
}

Class::Metatype Class::metatype() const { return m_metatype; }
std::string Class::full_name() const { return m_name; }
Class::MemberInfo *Class::find_operator(Operator op) const { return m_operators[op]; }
std::vector<Class::MemberInfo *> &Class::slots() { return m_slots; }
Class::MembersMapping &Class::members() { return m_members; }
Class::MembersMapping &Class::globals() { return m_globals; }

MINT_EXPORT Symbol get_operator_symbol(Class::Operator op);
MINT_EXPORT std::optional<Class::Operator> get_symbol_operator(const Symbol &symbol);

}

#endif // MINT_CLASS_H
