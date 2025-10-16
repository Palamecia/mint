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

#include "symbolevaluator.h"

#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/symbol.h"
#include "mint/compiler/token.h"
#include "mint/memory/data.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include <optional>
#include <string>

SymbolEvaluator::SymbolEvaluator(mint::Cursor& cursor) :
    _cursor(cursor) {}

const std::optional<mint::WeakReference>& SymbolEvaluator::get_reference() const {
	return _reference;
}

std::string SymbolEvaluator::get_symbol_name() const {
	return _symbol_name;
}

bool SymbolEvaluator::on_token(mint::Token type, const std::string& token, std::string::size_type /*offset*/) {
	switch (type) {
	case mint::Token::symbol_token:
		switch (_state) {
		case State::read_ident:
			_reference = get_symbol_reference(_cursor.get().symbols(), mint::Symbol(token));
			_state = State::read_operator;
			_symbol_name += token;
			break;

		case State::read_member:
			if (!_reference.has_value()) {
				return false;
			}
			_reference = get_member_reference(*_reference, mint::Symbol(token));
			_state = State::read_operator;
			_symbol_name += token;
			break;

		default:
			return false;
		}
		break;
	case mint::Token::dot_token:
		switch (_state) {
		case State::read_operator:
			_state = State::read_member;
			_symbol_name += token;
			break;

		default:
			return false;
		}
		break;
	case mint::Token::line_end_token:
	case mint::Token::file_end_token:
		return true;
	default:
		return false;
	}
	return true;
}

std::optional<mint::WeakReference> SymbolEvaluator::get_symbol_reference(mint::SymbolTable& symbols,
    const mint::Symbol& symbol) {

	if (auto it = symbols.find(symbol); it != symbols.end()) {
		return it->second;
	}

	mint::GlobalData& globals = _cursor.get().ast().global_data();
	if (auto it = globals.symbols().find(symbol); it != globals.symbols().end()) {
		return it->second;
	}

	return std::nullopt;
}

std::optional<mint::WeakReference> SymbolEvaluator::get_member_reference(const mint::Reference& reference,
    const mint::Symbol& member) {
	switch (reference.data().format()) {
	case mint::Data::package_format:
		for (mint::PackageData* package_data = &reference.data<mint::Package>().data; package_data != nullptr;
		    package_data = package_data->get_owner_package()) {
			if (auto it = package_data->symbols().find(member); it != package_data->symbols().end()) {
				return it->second;
			}
		}
		break;

	case mint::Data::object_format:
		{
			auto& object = reference.data<mint::Object>();
			if (auto* info = object.metadata.find_member(member)) {
				if (mint::is_object(object)) {
					return mint::Class::MemberInfo::get(*info, object);
				}
				constexpr auto flags = mint::Reference::const_address | mint::Reference::const_value
				                       | mint::Reference::global;
				return mint::WeakReference(flags, info->value.data());
			}

			if (auto* info = object.metadata.find_global(member)) {
				return info->value;
			}

			for (mint::PackageData* package = &object.metadata.get_package(); package != nullptr;
			    package = package->get_owner_package()) {
				if (auto it = package->symbols().find(member); it != package->symbols().end()) {
					constexpr auto flags = mint::Reference::const_address | mint::Reference::const_value;
					return mint::WeakReference(flags, it->second.data());
				}
			}
		}
		break;

	default:
		mint::GlobalData& externals = _cursor.get().ast().global_data();
		if (auto it = externals.symbols().find(member); it != externals.symbols().end()) {
			constexpr auto flags = mint::Reference::const_address | mint::Reference::const_value;
			return mint::WeakReference(flags, it->second.data());
		}
	}

	return std::nullopt;
}
