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

#include "mint/memory/symboltable.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/class.h"

using namespace std;
using namespace mint;

SymbolTable::SymbolTable(Class *metadata) :
	m_metadata(metadata),
	m_fasts(nullptr) {

}

SymbolTable::~SymbolTable() {
	delete [] m_fasts;
}

Class *SymbolTable::get_metadata() const {
	return m_metadata;
}

PackageData *SymbolTable::get_package() const {

	if (m_package.empty()) {
		return GlobalData::instance();
	}

	return m_package.back();
}

WeakReference &SymbolTable::createFastReference(const Symbol &name, size_t index) {
	return *(m_fasts[index] = std::make_unique<WeakReference>(get_symbol(this, name)));
}

WeakReference &SymbolTable::createFastReference(Reference::Flags flags, const Symbol &name, size_t index) {
	return *(m_fasts[index] = std::make_unique<WeakReference>(WeakReference::share(m_symbols.emplace(name, WeakReference(flags)).first->second)));
}
