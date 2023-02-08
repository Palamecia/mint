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

#ifndef MINT_GLOBALDATA_H
#define MINT_GLOBALDATA_H

#include "mint/ast/classregister.h"
#include "mint/memory/symboltable.h"

#include <array>

namespace mint {

class MINT_EXPORT PackageData : public ClassRegister {
	PackageData(const PackageData &) = delete;
	PackageData &operator =(const PackageData &) = delete;
public:
	Symbol name() const;
	std::string full_name() const;

	Path get_path() const;

	PackageData *get_package() const;
	PackageData *get_package(const Symbol &name);
	PackageData *find_package(const Symbol &name) const;

	void register_class(Id id);
	Class *get_class(const Symbol &name);

	inline SymbolTable &symbols();
	
	void cleanup_memory() override;
	void cleanup_metadata() override;

protected:
	PackageData(const std::string &name, PackageData *owner = nullptr);
	~PackageData() override;

private:
	Symbol m_name;
	PackageData *m_owner;
	SymbolMapping<PackageData *> m_packages;
	SymbolTable m_symbols;
};

class MINT_EXPORT GlobalData : public PackageData {
	GlobalData(const GlobalData &) = delete;
	GlobalData &operator =(const GlobalData &) = delete;
public:
	static GlobalData *instance();

	template<class BuiltinClass>
	BuiltinClass *builtin(Class::Metatype type);

	inline Reference *none_ref();
	inline Reference *null_ref();

	void cleanup_builtin();

protected:
	GlobalData();
	~GlobalData() override;
	friend class AbstractSyntaxTree;

private:
	static GlobalData *g_instance;
	std::array<Class *, 8> m_builtin;
	Reference *m_none = nullptr;
	Reference *m_null = nullptr;
};

SymbolTable &PackageData::symbols() { return m_symbols; }

template<class BuiltinClass>
BuiltinClass *GlobalData::builtin(Class::Metatype type) {
	if (BuiltinClass *instance = static_cast<BuiltinClass *>(m_builtin[type])) {
		return instance;
	}
	return static_cast<BuiltinClass *>(m_builtin[type] = new BuiltinClass);
}

Reference *GlobalData::none_ref() {
	return m_none ? m_none : m_none = new StrongReference(Reference::const_address | Reference::const_value, new None);
}

Reference *GlobalData::null_ref() {
	return m_null ? m_null : m_null = new StrongReference(Reference::const_address | Reference::const_value, new Null);
}

}

#endif // MINT_GLOBALDATA_H
