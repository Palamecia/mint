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

#include "mint/memory/memorytool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/objectprinter.h"
#include "mint/memory/casttool.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/system/error.h"
#include "mint/ast/fileprinter.h"
#include "mint/ast/cursor.h"

using namespace std;
using namespace mint;

static bool ensure_not_defined(const Symbol &symbol, SymbolTable &symbols) {

	auto it = symbols.find(symbol);
	if (it != symbols.end()) {
		if (UNLIKELY(it->second.data()->format != Data::fmt_none)) {
			return false;
		}
		symbols.erase(it);
	}

	return true;
}

string mint::type_name(const Reference &reference) {

	switch (reference.data()->format) {
	case Data::fmt_none:
		return "none";
	case Data::fmt_null:
		return "null";
	case Data::fmt_number:
		return "number";
	case Data::fmt_boolean:
		return "boolean";
	case Data::fmt_object:
		return reference.data<Object>()->metadata->full_name();
	case Data::fmt_package:
		return "package";
	case Data::fmt_function:
		return "function";
	}

	return {};
}

bool mint::is_instance_of(const Reference &reference, const string &type_name) {
	switch (reference.data()->format) {
	case Data::fmt_object:
		if (reference.data<Object>()->metadata->metatype() == Class::object) {
			return type_name == reference.data<Object>()->metadata->full_name();
		}
		break;
	case Data::fmt_package:
		return type_name == reference.data<Package>()->data->full_name();
	default:
		break;
	}
	return false;
}

Printer *mint::create_printer(Cursor *cursor) {

	WeakReference ref = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	switch (ref.data()->format) {
	case Data::fmt_number:
		return new FilePrinter(static_cast<int>(to_integer(ref.data<Number>()->value)));
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::string:
			return new FilePrinter(ref.data<String>()->str.c_str());
		case Class::object:
			return new ObjectPrinter(cursor, ref.flags(), ref.data<Object>());
		default:
			break;
		}
		[[fallthrough]];
	default:
		error("cannot open printer from '%s'", type_name(ref).c_str());
	}
}

void mint::print(Printer *printer, Reference &reference) {

	assert(printer);

	printer->print(reference);
}

void mint::load_extra_arguments(Cursor *cursor) {

	WeakReference args = WeakReference::create(iterator_init(cursor->stack().back()));

	cursor->stack().pop_back();
	args.data<Iterator>()->ctx.finalize();
	
	cursor->waiting_calls().top().add_extra_argument(args.data<Iterator>()->ctx.size());
	cursor->stack().insert(cursor->stack().end(),
						   std::make_move_iterator(args.data<Iterator>()->ctx.begin()),
						   std::make_move_iterator(args.data<Iterator>()->ctx.end()));
}

void mint::capture_symbol(Cursor *cursor, const Symbol &symbol) {

	Reference &function = cursor->stack().back();

	for (auto &signature : function.data<Function>()->mapping) {
		signature.second.capture->erase(symbol);
		auto item = cursor->symbols().find(symbol);
		if (item != cursor->symbols().end()) {
			signature.second.capture->emplace(item->first, WeakReference(item->second.flags(), item->second.data()));
		}
	}
}

void mint::capture_as_symbol(Cursor *cursor, const Symbol &symbol) {

	WeakReference reference = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	Reference &function = cursor->stack().back();

	for (auto &signature : function.data<Function>()->mapping) {
		signature.second.capture->erase(symbol);
		if ((reference.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
			(*signature.second.capture)[symbol].copy(reference);
		}
		else {
			(*signature.second.capture)[symbol].move(reference);
		}
	}
}

void mint::capture_all_symbols(Cursor *cursor) {

	Reference &function = cursor->stack().back();

	for (auto &signature : function.data<Function>()->mapping) {
		signature.second.capture->clear();
		for (auto &item : cursor->symbols()) {
			signature.second.capture->emplace(item.first, WeakReference(item.second.flags(), item.second.data()));
		}
	}
}

static Cursor::Call &setup_member_call(Cursor *cursor, Reference &reference) {

	assert(reference.data()->format == Data::fmt_object);
	Object *object = reference.data<Object>();
	Cursor::Call::Flags flags = Cursor::Call::member_call;
	Class *metadata = object->metadata;

	if (mint::is_class(object)) {

		if (metadata->metatype() == Class::object) {
			WeakReference instance = WeakReference::clone(reference.data());
			object = instance.data<Object>();
			object->construct();
			reference = std::move(instance);
		}
		else {
			/*
			 * Builtin classes can not be aliased, there is no need to clone the prototype.
			 */
			object->construct();
		}

		if (Class::MemberInfo *info = metadata->find_operator(Class::new_operator)) {

			switch (info->value.flags() & Reference::visibility_mask) {
			case Reference::protected_visibility:
				if (UNLIKELY(!is_protected_accessible(cursor, info->owner))) {
					error("could not access protected member 'new' of class '%s'", metadata->full_name().c_str());
				}
				break;
			case Reference::private_visibility:
				if (UNLIKELY(!is_private_accessible(cursor, info->owner))) {
					error("could not access private member 'new' of class '%s'", metadata->full_name().c_str());
				}
				break;
			case Reference::package_visibility:
				if (UNLIKELY(!is_package_accessible(cursor, info->owner))) {
					error("could not access package member 'new' of class '%s'", metadata->full_name().c_str());
				}
				break;
			}
			
			cursor->waiting_calls().emplace(WeakReference::share(Class::MemberInfo::get(info, object)));
			metadata = info->owner;
		}
		else {
			cursor->waiting_calls().emplace(WeakReference::create<None>());
		}
	}
	else if (Class::MemberInfo *info = metadata->find_operator(Class::call_operator)) {
		cursor->waiting_calls().emplace(WeakReference::share(Class::MemberInfo::get(info, object)));
		flags |= Cursor::Call::operator_call;
		metadata = info->owner;
	}
	else {
		error("class '%s' dosen't ovreload operator '()'", metadata->full_name().c_str());
	}
	
	Cursor::Call &call = cursor->waiting_calls().top();
	call.set_metadata(metadata);
	call.set_flags(flags);
	return call;
}

void mint::init_call(Cursor *cursor) {
	if (cursor->stack().back().data()->format != Data::fmt_object) {
		cursor->waiting_calls().emplace(std::forward<Reference>(cursor->stack().back()));
		cursor->stack().pop_back();
	}
	else {
		setup_member_call(cursor, cursor->stack().back());
	}
}

void mint::init_call(Cursor *cursor, Reference &function) {
	if (function.data()->format != Data::fmt_object) {
		cursor->waiting_calls().emplace(std::forward<Reference>(function));
	}
	else {
		setup_member_call(cursor, function);
	}
}

void mint::init_member_call(Cursor *cursor, const Symbol &member) {

	Class *owner = nullptr;
	WeakReference function = get_member(cursor, cursor->stack().back(), member, &owner);

	if (function.flags() & Reference::global) {
		cursor->stack().pop_back();
	}

	if (function.data()->format != Data::fmt_object) {
		cursor->waiting_calls().emplace(std::forward<Reference>(function));
		cursor->waiting_calls().top().set_metadata(owner);
	}
	else if (setup_member_call(cursor, function).get_flags() & Cursor::Call::operator_call) {
		cursor->stack().back() = std::forward<Reference>(function);
	}
	else {
		cursor->stack().emplace_back(std::forward<Reference>(function));
	}
}

void mint::init_operator_call(Cursor *cursor, Class::Operator op) {

	Class *owner = nullptr;
	WeakReference function = get_operator(cursor, cursor->stack().back(), op, &owner);

	if (function.flags() & Reference::global) {
		cursor->stack().pop_back();
	}

	if (function.data()->format != Data::fmt_object) {
		cursor->waiting_calls().emplace(std::forward<Reference>(function));
		cursor->waiting_calls().top().set_metadata(owner);
	}
	else if (setup_member_call(cursor, function).get_flags() & Cursor::Call::operator_call) {
		cursor->stack().back() = std::forward<Reference>(function);
	}
	else {
		cursor->stack().emplace_back(std::forward<Reference>(function));
	}
}

void mint::exit_call(Cursor *cursor) {
	cursor->exit_call();
}

void mint::init_exception(Cursor *cursor, const Symbol &symbol) {

	Reference &value = cursor->stack().back();
	SymbolTable &symbols = cursor->symbols();

	if (!ensure_not_defined(symbol, symbols)) {
		error("symbol '%s' was already defined in this context", symbol.str().c_str());
	}

	symbols.emplace(symbol, value);
	cursor->stack().pop_back();
}

void mint::reset_exception(Cursor *cursor, const Symbol &symbol) {
	SymbolTable &symbols = cursor->symbols();
	symbols.erase(symbol);
}

void mint::init_parameter(Cursor *cursor, const Symbol &symbol, mint::Reference::Flags flags, size_t index) {

	Reference &value = cursor->stack().back();
	SymbolTable &symbols = cursor->symbols();

	if (flags & Reference::const_value) {
		symbols.setup_fast(symbol, index, flags).move(value);
	}
	else if ((value.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
		symbols.setup_fast(symbol, index, flags).copy(value);
	}
	else {
		symbols.setup_fast(symbol, index, flags).move(value);
	}

	cursor->stack().pop_back();
}

Function::mapping_type::iterator mint::find_function_signature(Cursor *cursor, Function::mapping_type &mapping, int signature) {

	auto it = mapping.find(signature);

	if (it != mapping.end()) {
		return it;
	}

	it = mapping.lower_bound(~signature);

	if (it != mapping.end()) {

		auto &stack = cursor->stack();
		const int required = ~it->first;

		if (UNLIKELY(required < 0)) {
			return mapping.end();
		}

		Iterator *va_args = GarbageCollector::instance().alloc<Iterator>();
		va_args->construct();

		const auto from = std::prev(stack.end(), signature - required);
		const auto to = stack.end();
		for (auto it = from; it != to; ++it) {
			va_args->ctx.emplace(std::forward<Reference>(*it));
		}

		stack.erase(from, to);
		stack.emplace_back(Reference::standard, va_args);
	}

	return it;
}

bool mint::has_signature(Function::mapping_type &mapping, int signature) {

	auto it = mapping.find(signature);

	if (it != mapping.end()) {
		return true;
	}

	it = mapping.lower_bound(~signature);

	if (it != mapping.end()) {
		return true;
	}

	return false;
}

bool mint::has_signature(Reference &reference, int signature) {
	switch (reference.data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
	case Data::fmt_number:
	case Data::fmt_boolean:
		return signature == 0;
	case Data::fmt_object:
		if (is_object(reference.data<Object>())) {
			if (auto op = reference.data<Object>()->metadata->find_operator(Class::call_operator)) {
				return has_signature(Class::MemberInfo::get(op, reference.data<Object>()), signature);
			}
		}
		else {
			if (auto op = reference.data<Object>()->metadata->find_operator(Class::new_operator)) {
				return has_signature(op->value, signature);
			}
		}
		return signature == 0;
	case Data::fmt_package:
		return false;
	case Data::fmt_function:
		return has_signature(reference.data<Function>()->mapping, signature);
	}
	return false;
}

void mint::yield(Cursor *cursor, Reference &generator) {
	WeakReference item = std::move(cursor->stack().back());
	cursor->stack().pop_back();
	iterator_insert(generator.data<Iterator>(), WeakReference(item.flags(), item.data()));
}

WeakReference mint::get_symbol(SymbolTable *symbols, const Symbol &symbol) {

	if (auto it = symbols->find(symbol); it != symbols->end()) {
		return WeakReference::share(it->second);
	}

	GlobalData *globals = GlobalData::instance();
	if (auto it = globals->symbols().find(symbol); it != globals->symbols().end()) {
		return WeakReference::share(it->second);
	}

	return WeakReference::share((*symbols)[symbol]);
}

WeakReference mint::get_member(Cursor *cursor, const Reference &reference, const Symbol &member, Class **owner) {

	switch (reference.data()->format) {
	case Data::fmt_package:
		for (PackageData *package = reference.data<Package>()->data; package != nullptr; package = package->get_package()) {

			if (auto it = package->symbols().find(member); it != package->symbols().end()) {

				if (owner) {
					*owner = nullptr;
				}

				return WeakReference::share(it->second);
			}
		}

		if (PackageData *package = reference.data<Package>()->data) {
			error("package '%s' has no member '%s'", package->name().str().c_str(), member.str().c_str());
		}

		break;

	case Data::fmt_object:
		if (Object *object = reference.data<Object>()) {

			if (auto it = object->metadata->members().find(member); it != object->metadata->members().end()) {
				if (is_object(object)) {

					Reference &result = Class::MemberInfo::get(it->second, object);

					switch (result.flags() & Reference::visibility_mask) {
					case Reference::protected_visibility:
						if (UNLIKELY(!is_protected_accessible(cursor, it->second->owner))) {
							error("could not access protected member '%s' of class '%s'",
								  member.str().c_str(),
								  object->metadata->full_name().c_str());
						}
						break;
					case Reference::private_visibility:
						if (UNLIKELY(!is_private_accessible(cursor, it->second->owner))) {
							error("could not access private member '%s' of class '%s'",
								  member.str().c_str(),
								  object->metadata->full_name().c_str());
						}
						break;
					case Reference::package_visibility:
						if (UNLIKELY(!is_package_accessible(cursor, it->second->owner))) {
							error("could not access package member '%s' of class '%s'",
								  member.str().c_str(),
								  object->metadata->full_name().c_str());
						}
						break;
					}

					if (owner) {
						*owner = it->second->owner;
					}

					return WeakReference::share(result);
				}
				else {
					
					if (UNLIKELY(cursor->is_in_builtin() || cursor->symbols().get_metadata() == nullptr)) {
						error("could not access member '%s' of class '%s' without object",
							  member.str().c_str(),
							  object->metadata->full_name().c_str());
					}
					if (UNLIKELY(!object->metadata->is_direct_base_or_same(cursor->symbols().get_metadata()))) {
						error("class '%s' is not a direct base of '%s'",
							  object->metadata->full_name().c_str(),
							  cursor->symbols().get_metadata()->full_name().c_str());
					}
					if (UNLIKELY((it->second->value.flags() & Reference::private_visibility) && (it->second->owner != cursor->symbols().get_metadata()))) {
						error("could not access private member '%s' of class '%s'",
							  member.str().c_str(),
							  object->metadata->full_name().c_str());
					}

					if (owner) {
						*owner = it->second->owner;
					}

					return WeakReference(Reference::const_address | Reference::const_value | Reference::global, it->second->value.data());
				}
			}

			if (auto it = object->metadata->globals().find(member); it != object->metadata->globals().end()) {

				Reference &result = it->second->value;

				if (result.data()->format != Data::fmt_none) {
					switch (result.flags() & Reference::visibility_mask) {
					case Reference::protected_visibility:
						if (UNLIKELY(!is_protected_accessible(cursor, it->second->owner))) {
							if (result.data()->format == Data::fmt_object && is_class(result.data<Object>())) {
								if (UNLIKELY(!is_protected_accessible(cursor, result.data<Object>()->metadata))) {
									error("could not access protected member type '%s' of class '%s'",
										  member.str().c_str(),
										  object->metadata->full_name().c_str());
								}
							}
							else {
								error("could not access protected member '%s' of class '%s'",
									  member.str().c_str(),
									  object->metadata->full_name().c_str());
							}
						}
						break;
					case Reference::private_visibility:
						if (UNLIKELY(!is_private_accessible(cursor, it->second->owner))) {
							if (result.data()->format == Data::fmt_object && is_class(result.data<Object>())) {
								if (UNLIKELY(!is_private_accessible(cursor, result.data<Object>()->metadata))) {
									error("could not access private member type '%s' of class '%s'",
										  member.str().c_str(),
										  object->metadata->full_name().c_str());
								}
							}
							else {
								error("could not access private member '%s' of class '%s'",
									  member.str().c_str(),
									  object->metadata->full_name().c_str());
							}
						}
						break;
					case Reference::package_visibility:
						if (UNLIKELY(!is_package_accessible(cursor, it->second->owner))) {
							if (result.data()->format == Data::fmt_object && is_class(result.data<Object>())) {
								if (UNLIKELY(!is_package_accessible(cursor, result.data<Object>()->metadata))) {
									error("could not access package member type '%s' of class '%s'",
										  member.str().c_str(),
										  object->metadata->full_name().c_str());
								}
							}
							else {
								error("could not access package member '%s' of class '%s'",
									  member.str().c_str(),
									  object->metadata->full_name().c_str());
							}
						}
						break;
					}
				}

				if (owner) {
					*owner = it->second->owner;
				}

				return WeakReference::share(result);
			}

			for (PackageData *package = object->metadata->get_package(); package != nullptr; package = package->get_package()) {

				if (auto it = package->symbols().find(member); it != package->symbols().end()) {

					if (owner) {
						*owner = nullptr;
					}

					return WeakReference(Reference::const_address | Reference::const_value, it->second.data());
				}
			}

			if (is_object(object)) {
				error("class '%s' has no member '%s'", object->metadata->full_name().c_str(), member.str().c_str());
			}
			else {
				error("class '%s' has no global member '%s'", object->metadata->full_name().c_str(), member.str().c_str());
			}
		}

		break;

	default:
		GlobalData *externals = GlobalData::instance();
		if (auto it = externals->symbols().find(member); it != externals->symbols().end()) {

			if (owner) {
				*owner = nullptr;
			}

			return WeakReference(Reference::const_address | Reference::const_value, it->second.data());
		}

		error("non class values dosen't have member '%s'", member.str().c_str());
	}

	return {};
}

WeakReference mint::get_operator(Cursor *cursor, const Reference &reference, Class::Operator op, Class **owner) {

	switch (reference.data()->format) {
	case Data::fmt_object:
		if (Class::MemberInfo *member = reference.data<Object>()->metadata->find_operator(op)) {

			Object *object = reference.data<Object>();

			if (is_object(object)) {

				Reference &result = Class::MemberInfo::get(member, object);

				switch (result.flags() & Reference::visibility_mask) {
				case Reference::protected_visibility:
					if (UNLIKELY(!is_protected_accessible(cursor, member->owner))) {
						error("could not access protected member '%s' of class '%s'",
							  get_operator_symbol(op).str().c_str(),
							  object->metadata->full_name().c_str());
					}
					break;
				case Reference::private_visibility:
					if (UNLIKELY(!is_private_accessible(cursor, member->owner))) {
						error("could not access private member '%s' of class '%s'",
							  get_operator_symbol(op).str().c_str(),
							  object->metadata->full_name().c_str());
					}
					break;
				case Reference::package_visibility:
					if (UNLIKELY(!is_package_accessible(cursor, member->owner))) {
						error("could not access package member '%s' of class '%s'",
							  get_operator_symbol(op).str().c_str(),
							  object->metadata->full_name().c_str());
					}
					break;
				}

				if (owner) {
					*owner = member->owner;
				}

				return WeakReference::share(result);
			}
			else {
				
				if (UNLIKELY(cursor->is_in_builtin() || cursor->symbols().get_metadata() == nullptr)) {
					error("could not access member '%s' of class '%s' without object",
						  get_operator_symbol(op).str().c_str(),
						  object->metadata->full_name().c_str());
				}
				if (UNLIKELY(!object->metadata->is_direct_base_or_same(cursor->symbols().get_metadata()))) {
					error("class '%s' is not a direct base of '%s'",
						  object->metadata->full_name().c_str(),
						  cursor->symbols().get_metadata()->full_name().c_str());
				}
				if (UNLIKELY((member->value.flags() & Reference::private_visibility) && (member->owner != cursor->symbols().get_metadata()))) {
					error("could not access private member '%s' of class '%s'",
						  get_operator_symbol(op).str().c_str(),
						  object->metadata->full_name().c_str());
				}

				if (owner) {
					*owner = member->owner;
				}

				WeakReference result(Reference::const_address | Reference::const_value | Reference::global);
				result.copy(member->value);
				return result;
			}
		}

		if (is_object(reference.data<Object>())) {
			error("class '%s' has no member '%s'",
				  reference.data<Object>()->metadata->full_name().c_str(),
				  get_operator_symbol(op).str().c_str());
		}
		else {
			error("class '%s' has no global member '%s'",
				  reference.data<Object>()->metadata->full_name().c_str(),
				  get_operator_symbol(op).str().c_str());
		}

	default:
		error("non class values dosen't have member '%s'", get_operator_symbol(op).str().c_str());
	}
}

void mint::reduce_member(Cursor *cursor, Reference &&member) {
	cursor->stack().back() = std::move(member);
}

Class::MemberInfo *mint::find_member_info(Object *object, const Reference &member) {

	const Data *data_ptr = member.data();

	if (Reference *data = object->data) {
		for (auto &infos : object->metadata->members()) {
			if (data_ptr == infos.second->value.data()) {
				return infos.second;
			}
			if (data_ptr == Class::MemberInfo::get(infos.second, object).data()) {
				return infos.second;
			}
		}
	}
	else {
		for (auto &infos : object->metadata->members()) {
			if (data_ptr == infos.second->value.data()) {
				return infos.second;
			}
		}
	}

	return nullptr;
}

bool mint::is_protected_accessible(Class *owner, Class *context) {
	return owner->is_base_or_same(context) || (context && context->is_base_of(owner));
}

bool mint::is_protected_accessible(Cursor *cursor, Class *owner) {
	return !cursor->is_in_builtin() && is_protected_accessible(owner, cursor->symbols().get_metadata());
}

bool mint::is_private_accessible(Cursor *cursor, Class *owner) {
	return !cursor->is_in_builtin() && owner == cursor->symbols().get_metadata();
}

bool mint::is_package_accessible(Cursor *cursor, Class *owner) {
	return !cursor->is_in_builtin() && owner->get_package() == cursor->symbols().get_package();
}

Symbol mint::var_symbol(Cursor *cursor) {

	WeakReference var = std::move(cursor->stack().back());
	cursor->stack().pop_back();
	return Symbol(to_string(var));
}

void mint::create_symbol(Cursor *cursor, const Symbol &symbol, Reference::Flags flags) {

	if (flags & Reference::global) {

		PackageData *package = cursor->symbols().get_package();

		if (UNLIKELY(!ensure_not_defined(symbol, package->symbols()))) {
			error("symbol '%s' was already defined in global context", symbol.str().c_str());
		}

		cursor->stack().emplace_back(WeakReference::share(package->symbols().emplace(symbol, WeakReference(flags)).first->second));
	}
	else {

		if (UNLIKELY(!ensure_not_defined(symbol, cursor->symbols()))) {
			error("symbol '%s' was already defined in this context", symbol.str().c_str());
		}

		cursor->stack().emplace_back(WeakReference::share(cursor->symbols().emplace(symbol, WeakReference(flags)).first->second));
	}
}

void mint::create_symbol(Cursor *cursor, const Symbol &symbol, size_t index, Reference::Flags flags) {

	if (flags & Reference::global) {

		PackageData *package = cursor->symbols().get_package();

		if (UNLIKELY(!ensure_not_defined(symbol, package->symbols()))) {
			error("symbol '%s' was already defined in global context", symbol.str().c_str());
		}

		cursor->stack().emplace_back(WeakReference::share(package->symbols().emplace(symbol, WeakReference::share(cursor->symbols().setup_fast(symbol, index, flags))).first->second));
	}
	else {

		if (UNLIKELY(!ensure_not_defined(symbol, cursor->symbols()))) {
			error("symbol '%s' was already defined in this context", symbol.str().c_str());
		}

		cursor->stack().emplace_back(WeakReference::share(cursor->symbols().setup_fast(symbol, index, flags)));
	}
}

void mint::create_function(Cursor *cursor, const Symbol &symbol, Reference::Flags flags) {

	assert(flags & Reference::global);

	PackageData *package = cursor->symbols().get_package();

	auto it = package->symbols().find(symbol);
	if (it != package->symbols().end()) {
		switch (it->second.data()->format) {
		case Data::fmt_none:
			it->second = WeakReference(flags, GarbageCollector::instance().alloc<Function>());
			break;
		case Data::fmt_function:
			if (UNLIKELY(flags != it->second.flags())) {
				error("function '%s' was already defined in global context", symbol.str().c_str());
			}
			break;
		default:
			error("symbol '%s' was already defined in this context", symbol.str().c_str());
		}
	}
	else {
		it = package->symbols().emplace(symbol, WeakReference(flags, GarbageCollector::instance().alloc<Function>())).first;
	}

	cursor->stack().emplace_back(WeakReference::share(it->second));
}

void mint::function_overload_from_stack(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &signature = load_from_stack(cursor, base);
	Reference &function = load_from_stack(cursor, base - 1);

	for (const auto &item : signature.data<Function>()->mapping) {
		if (UNLIKELY(!function.data<Function>()->mapping.insert(item).second)) {
			error("defined function already takes %d%s parameter(s)", abs(item.first), item.first < 0 ? "..." : "");
		}
	}

	cursor->stack().pop_back();
}
