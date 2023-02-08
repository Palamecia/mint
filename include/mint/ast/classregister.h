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

#ifndef MINT_CLASSREGISTER_H
#define MINT_CLASSREGISTER_H

#include "mint/ast/symbolmapping.hpp"
#include "mint/memory/class.h"

#include <vector>
#include <string>

namespace mint {

class ClassDescription;

class MINT_EXPORT ClassRegister {
	ClassRegister(const ClassRegister &) = delete;
	ClassRegister &operator =(const ClassRegister &) = delete;
public:
	class MINT_EXPORT Path {
	public:
		Path(const Path &other, const Symbol &symbol);
		Path(std::initializer_list<Symbol> symbols);
		Path(const Symbol &symbol);
		Path(const Path &other);
		Path();

		ClassDescription *locate(PackageData *package) const;
		std::string to_string() const;

		void append_symbol(const Symbol &symbol);
		void clear();

	private:
		std::vector<Symbol> m_symbols;
	};

	static inline bool is_slot(const Reference &member);

	using Id = size_t;

	ClassRegister();
	virtual ~ClassRegister();

	virtual Id create_class(ClassDescription *desc);

	ClassDescription *find_class_description(const Symbol &name) const;
	ClassDescription *get_class_description(Id id) const;
	size_t count() const;

	virtual void cleanup_memory();
	virtual void cleanup_metadata();

private:
	std::vector<ClassDescription *> m_defined_classes;
};

class MINT_EXPORT ClassDescription : public ClassRegister, public MemoryRoot {
	ClassDescription(const ClassDescription &) = delete;
	ClassDescription &operator =(const ClassDescription &) = delete;
public:
	ClassDescription(PackageData *package, Reference::Flags flags, const std::string &name);
	~ClassDescription() override;

	Symbol name() const;
	std::string full_name() const;
	Reference::Flags flags() const;

	Path get_path() const;
	void add_base(const Path &base);
	Id create_class(ClassDescription *desc) override;

	bool create_member(Class::Operator op, Reference &&value);
	bool create_member(const Symbol &name, Reference &&value);
	bool update_member(Class::Operator op, Reference &&value);
	bool update_member(const Symbol &name, Reference &&value);

	const std::vector<Class *> &bases() const;
	Class *generate();

	void cleanup_memory() override;
	void cleanup_metadata() override;

protected:
	void mark() override {
		for (auto it = m_operators.begin(); it != m_operators.end(); ++it) {
			it->second.data()->mark();
		}
		for (auto it = m_members.begin(); it != m_members.end(); ++it) {
			it->second.data()->mark();
		}
		for (auto it = m_globals.begin(); it != m_globals.end(); ++it) {
			it->second.data()->mark();
		}
	}

private:
	ClassDescription *m_owner;
	PackageData *m_package;
	Reference::Flags m_flags;
	std::vector<Path> m_bases;

	Symbol m_name;
	Class *m_metadata;
	std::vector<Class *> m_bases_metadata;

	std::unordered_map<Class::Operator, WeakReference> m_operators;
	SymbolMapping<WeakReference> m_members;
	SymbolMapping<WeakReference> m_globals;
};

bool ClassRegister::is_slot(const Reference &member) {
	return ((member.flags() & (Reference::const_address | Reference::const_value)) != (Reference::const_address | Reference::const_value))
		   || member.data()->format == Data::fmt_none;
}

}

#endif // MINT_CLASSREGISTER_H
