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
	enum Metatype : std::uint8_t {
		OBJECT,
		STRING,
		REGEX,
		ARRAY,
		HASH,
		ITERATOR,
		LIBRARY,
		LIBOBJECT
	};

	static constexpr const size_t BUILTIN_CLASS_COUNT = LIBOBJECT + 1;

	enum Operator : std::uint8_t {
		NEW_OPERATOR,
		DELETE_OPERATOR,
		COPY_OPERATOR,
		CALL_OPERATOR,
		ADD_OPERATOR,
		SUB_OPERATOR,
		MUL_OPERATOR,
		DIV_OPERATOR,
		POW_OPERATOR,
		MOD_OPERATOR,
		IN_OPERATOR,
		EQ_OPERATOR,
		NE_OPERATOR,
		LT_OPERATOR,
		GT_OPERATOR,
		LE_OPERATOR,
		GE_OPERATOR,
		AND_OPERATOR,
		OR_OPERATOR,
		BAND_OPERATOR,
		BOR_OPERATOR,
		XOR_OPERATOR,
		INC_OPERATOR,
		DEC_OPERATOR,
		NOT_OPERATOR,
		COMPL_OPERATOR,
		SHIFT_LEFT_OPERATOR,
		SHIFT_RIGHT_OPERATOR,
		INCLUSIVE_RANGE_OPERATOR,
		EXCLUSIVE_RANGE_OPERATOR,
		SUBSCRIPT_OPERATOR,
		SUBSCRIPT_MOVE_OPERATOR,
		REGEX_MATCH_OPERATOR,
		REGEX_UNMATCH_OPERATOR
	};

	static constexpr const size_t OPERATOR_COUNT = REGEX_UNMATCH_OPERATOR + 1;

	struct MemberInfo {

		static constexpr const size_t INVALID_OFFSET = std::numeric_limits<size_t>::max();
		static inline WeakReference &get(MemberInfo *member, WeakReference *data);
		static inline WeakReference &get(MemberInfo *member, Object *object);

		size_t offset;
		Class *owner = nullptr;
		WeakReference value;
	};

	using MembersMapping = SymbolMapping<MemberInfo *>;

	Class(Class &&) = delete;
	Class(const Class &) = default;
	explicit Class(const std::string &name, Metatype metatype = OBJECT);
	Class(PackageData *package, std::string name, Metatype metatype = OBJECT);
	~Class() override;

	Class &operator=(Class &&) = delete;
	Class &operator=(const Class &) = default;

	MemberInfo *get_class(const Symbol &name);
	Object *make_instance();

	[[nodiscard]] inline Metatype metatype() const;
	[[nodiscard]] inline const std::string &full_name() const;
	[[nodiscard]] Symbol name() const;

	[[nodiscard]] PackageData *get_package() const;
	[[nodiscard]] ClassDescription *get_description() const;
	[[nodiscard]] inline MemberInfo *find_operator(Operator op) const;

	inline std::vector<MemberInfo *> &slots();
	inline MembersMapping &members();
	inline MembersMapping &globals();
	[[nodiscard]] size_t size() const;

	[[nodiscard]] const std::vector<Class *> &bases() const;
	bool is_base_of(const Class *other) const;
	bool is_base_or_same(const Class *other) const;
	bool is_direct_base_or_same(const Class *other) const;

	[[nodiscard]] bool is_copyable() const;
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
		for (auto &member : m_members) {
			member.second->value.data()->mark();
		}
		for (auto &global : m_globals) {
			global.second->value.data()->mark();
		}
	}

private:
	Metatype m_metatype;
	bool m_copyable = true;

	std::string m_name;
	PackageData *m_package;
	ClassDescription *m_description = nullptr;

	std::array<MemberInfo *, OPERATOR_COUNT> m_operators;
	std::vector<MemberInfo *> m_slots;
	MembersMapping m_members;
	MembersMapping m_globals;
};

WeakReference &Class::MemberInfo::get(MemberInfo *member, WeakReference *data) {
	return member->offset == INVALID_OFFSET ? member->value : data[member->offset];
}

WeakReference &Class::MemberInfo::get(MemberInfo *member, Object *object) {
	return member->offset == INVALID_OFFSET ? member->value : object->data[member->offset];
}

Class::Metatype Class::metatype() const {
	return m_metatype;
}

const std::string &Class::full_name() const {
	return m_name;
}

Class::MemberInfo *Class::find_operator(Operator op) const {
	return m_operators[op];
}

std::vector<Class::MemberInfo *> &Class::slots() {
	return m_slots;
}

Class::MembersMapping &Class::members() {
	return m_members;
}

Class::MembersMapping &Class::globals() {
	return m_globals;
}

MINT_EXPORT Symbol get_operator_symbol(Class::Operator op);
MINT_EXPORT std::optional<Class::Operator> get_symbol_operator(const Symbol &symbol);

}

#endif // MINT_CLASS_H
