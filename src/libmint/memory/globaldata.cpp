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

#include "mint/memory/globaldata.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/class.h"
#include "mint/system/error.h"

#include <algorithm>

using namespace std;
using namespace mint;

PackageData::PackageData(const string &name, PackageData *owner) :
	m_name(name),
	m_owner(owner) {

}

PackageData::~PackageData() {
	for (auto &package : m_packages) {
		delete package.second;
	}
}

Symbol PackageData::name() const {
	return m_name;
}

string PackageData::full_name() const {
	if (m_owner && m_owner != GlobalData::instance()) {
		return m_owner->full_name() + "." + name().str();
	}
	return name().str();
}

PackageData::Path PackageData::get_path() const {
	if (m_owner) {
		return { m_owner->get_path(), name() };
	}
	return { name() };
}

PackageData *PackageData::get_package() const {
	return m_owner;
}

PackageData *PackageData::get_package(const Symbol &name) {
	auto it = m_packages.find(name);
	if (it == m_packages.end()) {
		PackageData *package = new PackageData(name.str(), this);
		m_symbols.emplace(name, WeakReference(Reference::global | Reference::const_address | Reference::const_value, GarbageCollector::instance().alloc<Package>(package)));
		it = m_packages.emplace(name, package).first;
	}
	return it->second;
}

PackageData *PackageData::find_package(const Symbol &name) const {
	auto it = m_packages.find(name);
	if (it != m_packages.end()) {
		return it->second;
	}
	return nullptr;
}

void PackageData::register_class(ClassRegister::Id id) {

	ClassDescription *desc = get_class_description(id);
	Symbol &&symbol = desc->name();

	if (UNLIKELY(m_symbols.contains(symbol))) {
		string symbol_str = symbol.str();
		error("multiple definition of class '%s'", symbol_str.c_str());
	}

	Class *type = desc->generate();
	m_symbols.emplace(symbol, WeakReference(Reference::global | Reference::const_address | Reference::const_value, type->make_instance()));
}

Class *PackageData::get_class(const Symbol &name) {

	auto it = m_symbols.find(name);
	if (it != m_symbols.end() && it->second.data()->format == Data::fmt_object && is_class(it->second.data<Object>())) {
		return it->second.data<Object>()->metadata;
	}
	return nullptr;
}

void PackageData::cleanup_memory() {
	
	ClassRegister::cleanup_memory();

	for (auto &package : m_packages) {
		package.second->cleanup_memory();
	}

	for (auto symbol = m_symbols.begin(); symbol != m_symbols.end();) {
		if (is_class(symbol->second)) {
			symbol = next(symbol);
		}
		else {
			symbol = m_symbols.erase(symbol);
		}
	}
}

void PackageData::cleanup_metadata() {

	ClassRegister::cleanup_metadata();

	m_symbols.clear();

	for (auto &package : m_packages) {
		package.second->cleanup_metadata();
		delete package.second;
	}

	m_packages.clear();
}

GlobalData *GlobalData::g_instance = nullptr;

GlobalData::GlobalData() : PackageData("(default)") {
	m_builtin.fill(nullptr);
	g_instance = this;
}

GlobalData::~GlobalData() {
	for_each(m_builtin.begin(), m_builtin.end(), default_delete<Class>());
	delete m_none;
	delete m_null;
	g_instance = nullptr;
}

GlobalData *GlobalData::instance() {
	return g_instance;
}

void GlobalData::cleanup_builtin() {

	// cleanup builtin classes
	for_each(m_builtin.begin(), m_builtin.end(), default_delete<Class>());
	m_builtin.fill(nullptr);

	// cleanup builtin refs
	delete m_none;
	m_none = nullptr;

	delete m_null;
	m_null = nullptr;
}
