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

#include "mint/memory/classtool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/garbagecollector.h"
#include "mint/system/error.h"

using namespace mint;
using namespace std;

Class *mint::create_enum(const string &name, initializer_list<pair<Symbol, optional<intmax_t>>> values) {
	return create_enum(GlobalData::instance(), name, values);
}

Class *mint::create_enum(PackageData *package, const string &name, initializer_list<pair<Symbol, optional<intmax_t>>> values) {

	size_t next_enum_value = 0;
	ClassDescription *desc = new ClassDescription(package, Reference::standard, name);
	const Reference::Flags flags = Reference::const_value | Reference::const_address | Reference::global;

	for (auto &[symbol, value] : values) {
		if (value.has_value()) {
			if (!desc->create_member(symbol, WeakReference(flags, GarbageCollector::instance().alloc<Number>(*value)))) {
				error("%s: member was already defined for enum '%s'", symbol.str().c_str(), name.c_str());
			}
			next_enum_value = *value + 1;
		}
		else {
			if (!desc->create_member(symbol, WeakReference(flags, GarbageCollector::instance().alloc<Number>(next_enum_value++)))) {
				error("%s: member was already defined for enum '%s'", symbol.str().c_str(), name.c_str());
			}
		}
	}

	package->register_class(package->create_class(desc));
	return desc->generate();
}

Class *mint::create_class(const string &name, initializer_list<pair<Symbol, Reference&&>> members) {
	return create_class(GlobalData::instance(), name, {}, members);
}

Class *mint::create_class(PackageData *package, const string &name, initializer_list<pair<Symbol, Reference&&>> members) {
	return create_class(package, name, {}, members);
}

Class *mint::create_class(const string &name, initializer_list<ClassDescription *> bases, initializer_list<pair<Symbol, Reference&&>> members) {
	return create_class(GlobalData::instance(), name, bases, members);
}

Class *mint::create_class(PackageData *package, const string &name, initializer_list<ClassDescription *> bases, initializer_list<pair<Symbol, Reference&&>> members) {

	ClassDescription *desc = new ClassDescription(package, Reference::standard, name);

	for (ClassDescription *base : bases) {
		desc->add_base(base->get_path());
	}

	for (auto &[symbol, member] : members) {
		if (auto op = get_symbol_operator(symbol)) {
			if (!desc->create_member(*op, std::move(member))) {
				error("%s: member was already defined for class '%s'", symbol.str().c_str(), name.c_str());
			}
		}
		else {
			if (!desc->create_member(symbol, std::move(member))) {
				error("%s: member was already defined for class '%s'", symbol.str().c_str(), name.c_str());
			}
		}
	}

	package->register_class(package->create_class(desc));
	return desc->generate();
}
