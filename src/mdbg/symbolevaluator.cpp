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

#include "symbolevaluator.h"

#include <mint/memory/globaldata.h>
#include <mint/memory/memorytool.h>

using namespace mint;
using namespace std;

SymbolEvaluator::SymbolEvaluator(Cursor *cursor) :
	m_cursor(cursor) {

}

const optional<WeakReference> &SymbolEvaluator::get_reference() const {
	return m_reference;
}

string SymbolEvaluator::get_symbol_name() const {
	return m_symbol_name;
}

bool SymbolEvaluator::on_token(token::Type type, const string &token, string::size_type offset) {
	switch (type) {
	case token::symbol_token:
		switch (m_state) {
		case read_ident:
			m_reference = get_symbol_reference(&m_cursor->symbols(), Symbol(token));
			m_state = read_operator;
			m_symbol_name += token;
			break;

		case read_member:
			if (!m_reference.has_value()) {
				return false;
			}
			m_reference = get_member_reference(*m_reference, Symbol(token));
			m_state = read_operator;
			m_symbol_name += token;
			break;

		default:
			return false;
		}
		break;
	case token::dot_token:
		switch (m_state) {
		case read_operator:
			m_state = read_member;
			m_symbol_name += token;
			break;

		default:
			return false;
		}
		break;
	case token::line_end_token:
	case token::file_end_token:
		return true;
	default:
		return false;
	}
	return true;
}

optional<WeakReference> SymbolEvaluator::get_symbol_reference(SymbolTable *symbols, const Symbol &symbol) {

	if (auto it = symbols->find(symbol); it != symbols->end()) {
		return WeakReference::share(it->second);
	}

	GlobalData *globals = GlobalData::instance();
	if (auto it = globals->symbols().find(symbol); it != globals->symbols().end()) {
		return WeakReference::share(it->second);
	}

	return nullopt;
}

optional<WeakReference> SymbolEvaluator::get_member_reference(Reference &reference, const Symbol &member) {

	switch (reference.data()->format) {
	case Data::fmt_package:
		for (PackageData *package_data = reference.data<Package>()->data; package_data != nullptr; package_data = package_data->get_package()) {
			if (auto it = package_data->symbols().find(member); it != package_data->symbols().end()) {
				return WeakReference::share(it->second);
			}
		}
		break;

	case Data::fmt_object:
		if (Object *object = reference.data<Object>()) {

			if (auto it = object->metadata->members().find(member); it != object->metadata->members().end()) {
				if (mint::is_object(object)) {
					return WeakReference::share(Class::MemberInfo::get(it->second, object));
				}
				else {
					return WeakReference(Reference::const_address | Reference::const_value | Reference::global, it->second->value.data());
				}
			}

			if (auto it = object->metadata->globals().find(member); it != object->metadata->globals().end()) {
				return WeakReference::share(it->second->value);
			}

			for (PackageData *package = object->metadata->get_package(); package != nullptr; package = package->get_package()) {
				if (auto it = package->symbols().find(member); it != package->symbols().end()) {
					return WeakReference(Reference::const_address | Reference::const_value, it->second.data());
				}
			}
		}
		break;

	default:
		GlobalData *externals = GlobalData::instance();
		if (auto it = externals->symbols().find(member); it != externals->symbols().end()) {
			return WeakReference(Reference::const_address | Reference::const_value, it->second.data());
		}
	}

	return nullopt;
}
