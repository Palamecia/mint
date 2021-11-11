#include "memory/memorytool.h"
#include "memory/globaldata.h"
#include "memory/functiontool.h"
#include "memory/objectprinter.h"
#include "memory/casttool.h"
#include "memory/builtin/string.h"
#include "memory/builtin/regex.h"
#include "ast/cursor.h"
#include "system/utf8iterator.h"
#include "system/fileprinter.h"
#include "system/assert.h"
#include "system/error.h"

using namespace std;
using namespace mint;

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
		return reference.data<Object>()->metadata->name();
	case Data::fmt_package:
		return "package";
	case Data::fmt_function:
		return "function";
	}

	return string();
}

bool mint::is_class(const Object *data) {
	return data->data == nullptr;
}

bool mint::is_object(const Object *data) {
	return data->data != nullptr;
}

Printer *mint::create_printer(Cursor *cursor) {

	WeakReference ref = move(cursor->stack().back());
	cursor->stack().pop_back();

	switch (ref.data()->format) {
	case Data::fmt_number:
		return new FilePrinter(static_cast<int>(ref.data<Number>()->value));
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::string:
			return new FilePrinter(ref.data<String>()->str.c_str());
		case Class::object:
			return new ObjectPrinter(cursor, ref.flags(), ref.data<Object>());
		default:
			break;
		}
		fall_through;
	default:
		error("cannot open printer from '%s'", type_name(ref).c_str());
	}
}

void mint::print(Printer *printer, Reference &reference) {

	assert(printer);

	Printer::DataType type = Printer::none;

	switch (reference.data()->format) {
	case Data::fmt_none:
		type = Printer::none;
		break;

	case Data::fmt_null:
		type = Printer::null;
		break;

	case Data::fmt_number:
		printer->print(reference.data<Number>()->value);
		return;

	case Data::fmt_boolean:
		printer->print(reference.data<Boolean>()->value);
		return;

	case Data::fmt_object:
		switch (reference.data<Object>()->metadata->metatype()) {
		case Class::string:
			printer->print(reference.data<String>()->str.c_str());
			return;

		case Class::regex:
			type = Printer::regex;
			break;

		case Class::array:
			type = Printer::array;
			break;

		case Class::hash:
			type = Printer::hash;
			break;

		case Class::iterator:
			type = Printer::iterator;
			break;

		default:
			type = Printer::object;
			break;
		}
		break;

	case Data::fmt_package:
		type = Printer::package;
		break;

	case Data::fmt_function:
		type = Printer::function;
		break;
	}

	if (!printer->print(type, reference.data())) {
		printer->print(to_string(reference).c_str());
	}
}

void mint::load_extra_arguments(Cursor *cursor) {

	WeakReference args = WeakReference::create(iterator_init(cursor->stack().back()));
	cursor->stack().pop_back();

	while (optional<WeakReference> &&item = iterator_next(args.data<Iterator>())) {
		cursor->stack().emplace_back(WeakReference(item->flags(), item->data()));
		cursor->waitingCalls().top().addExtraArgument();
	}
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

void mint::capture_all_symbols(Cursor *cursor) {

	Reference &function = cursor->stack().back();

	for (auto &signature : function.data<Function>()->mapping) {
		signature.second.capture->clear();
		for (auto &item : cursor->symbols()) {
			signature.second.capture->emplace(item.first, WeakReference(item.second.flags(), item.second.data()));
		}
	}
}

void mint::init_call(Cursor *cursor) {

	if (cursor->stack().back().data()->format != Data::fmt_object) {
		cursor->waitingCalls().emplace(move(cursor->stack().back()));
		cursor->stack().pop_back();
	}
	else {

		Object *object = cursor->stack().back().data<Object>();
		Cursor::Call::Flags flags = Cursor::Call::member_call;
		Class *metadata = object->metadata;

		if (is_class(object)) {

			if (metadata->metatype() == Class::object) {
				WeakReference instance = WeakReference::clone(cursor->stack().back().data());
				object = instance.data<Object>();
				object->construct();
				cursor->stack().back() = move(instance);
			}
			else {
				/*
				 * Builtin classes can not be aliased, there is no need to clone the prototype.
				 */
				object->construct();
			}

			if (Class::MemberInfo *info = metadata->findOperator(Class::new_operator)) {

				switch (info->value.flags() & Reference::visibility_mask) {
				case Reference::protected_visibility:
					if (UNLIKELY(!is_protected_accessible(info->owner, cursor->symbols().getMetadata()))) {
						error("could not access protected member 'new' of class '%s'", metadata->name().c_str());
					}
					break;
				case Reference::private_visibility:
					if (UNLIKELY(info->owner != cursor->symbols().getMetadata())) {
						error("could not access private member 'new' of class '%s'", metadata->name().c_str());
					}
					break;
				case Reference::package_visibility:
					if (UNLIKELY(info->owner->getPackage() != cursor->symbols().getPackage())) {
						error("could not access package member 'new' of class '%s'", metadata->name().c_str());
					}
					break;
				}

				cursor->waitingCalls().emplace(WeakReference::share(object->data[info->offset]));
				metadata = info->owner;
			}
			else {
				cursor->waitingCalls().emplace(WeakReference::create<None>());
			}
		}
		else if (Class::MemberInfo *info = metadata->findOperator(Class::call_operator)) {
			cursor->waitingCalls().emplace(WeakReference::share(object->data[info->offset]));
			flags |= Cursor::Call::operator_call;
			metadata = info->owner;
		}
		else {
			error("class '%s' dosen't ovreload operator '()'", metadata->name().c_str());
		}

		Cursor::Call &call = cursor->waitingCalls().top();
		call.setMetadata(metadata);
		call.setFlags(flags);
	}
}

void mint::init_member_call(Cursor *cursor, const Symbol &member) {

	Class *owner = nullptr;
	Reference &reference = cursor->stack().back();
	WeakReference function = get_object_member(cursor, reference, member, &owner);

	if (function.flags() & Reference::global) {
		cursor->stack().pop_back();
	}

	cursor->stack().emplace_back(move(function));

	init_call(cursor);

	Cursor::Call &call = cursor->waitingCalls().top();

	if (!call.getMetadata()) {
		call.setMetadata(owner);
	}

	if (call.getFlags() & Cursor::Call::operator_call) {
		function = move(cursor->stack().back());
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(function));
	}
}

void mint::init_operator_call(Cursor *cursor, Class::Operator op) {

	Class *owner = nullptr;
	Reference &reference = cursor->stack().back();
	WeakReference function = get_object_operator(cursor, reference, op, &owner);

	if (function.flags() & Reference::global) {
		cursor->stack().pop_back();
	}

	cursor->stack().emplace_back(move(function));

	init_call(cursor);

	Cursor::Call &call = cursor->waitingCalls().top();

	if (!call.getMetadata()) {
		call.setMetadata(owner);
	}

	if (call.getFlags() & Cursor::Call::operator_call) {
		function = move(cursor->stack().back());
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(function));
	}
}

void mint::exit_call(Cursor *cursor) {
	cursor->exitCall();
}

void mint::init_parameter(Cursor *cursor, const Symbol &symbol, size_t index) {

	Reference &value = cursor->stack().back();
	SymbolTable &symbols = cursor->symbols();

	if ((value.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
		symbols.setup_fast(symbol, index).copy(value);
	}
	else {
		symbols.setup_fast(symbol, index).move(value);
	}

	cursor->stack().pop_back();
}

Function::mapping_type::iterator mint::find_function_signature(Cursor *cursor, Function::mapping_type &mapping, int signature) {

	auto it = mapping.find(signature);

	if (it != mapping.end()) {
		return it;
	}

	it = mapping.lower_bound(-signature);

	if (it != mapping.end()) {

		const int required = -it->first;

		if (UNLIKELY(required < 0)) {
			return mapping.end();
		}

		Iterator *va_args = Reference::alloc<Iterator>();
		va_args->construct();

		while (required < signature--) {
			va_args->ctx.emplace_front(move(cursor->stack().back()));
			cursor->stack().pop_back();
		}

		cursor->stack().emplace_back(WeakReference(Reference::standard, va_args));
	}

	return it;
}

void mint::yield(Cursor *cursor) {

	Iterator *generator = cursor->symbols().generator();

	assert(generator);

	const Cursor::ExecutionMode mode = cursor->executionMode();
	WeakReference defaultResult = WeakReference::share(cursor->symbols().defaultResult());
	WeakReference item = move(cursor->stack().back());

	cursor->stack().pop_back();
	iterator_insert(generator, WeakReference(item.flags(), item.data()));

	switch (mode) {
	case Cursor::single_pass:
	case Cursor::resumed:
		break;

	case Cursor::interruptible:
		cursor->stack().emplace_back(move(defaultResult));
		break;
	}
}

void mint::load_generator_result(Cursor *cursor) {

	switch (cursor->executionMode()) {
	case Cursor::single_pass:
	case Cursor::resumed:
		break;

	case Cursor::interruptible:
		cursor->stack().emplace_back(WeakReference::share(cursor->symbols().defaultResult()));
		break;
	}
}

void mint::load_current_result(Cursor *cursor) {

	Iterator *generator = cursor->symbols().generator();

	if (UNLIKELY(generator != nullptr)) {

		const Cursor::ExecutionMode mode = cursor->executionMode();
		WeakReference defaultResult = WeakReference::share(cursor->symbols().defaultResult());
		WeakReference item = move(cursor->stack().back());

		cursor->setExecutionMode(Cursor::single_pass);
		cursor->stack().pop_back();
		iterator_insert(generator, move(item));

		switch (mode) {
		case Cursor::single_pass:
		case Cursor::resumed:
			break;

		case Cursor::interruptible:
			cursor->stack().emplace_back(move(defaultResult));
			break;
		}
	}
}

void mint::load_default_result(Cursor *cursor) {
	cursor->stack().emplace_back(WeakReference::share(cursor->symbols().defaultResult()));
}

WeakReference mint::get_symbol_reference(SymbolTable *symbols, const Symbol &symbol) {

	auto it_local = symbols->find(symbol);
	if (it_local != symbols->end()) {
		return WeakReference::share(it_local->second);
	}

	PackageData *package = symbols->getPackage();

	auto it_package = package->symbols().find(symbol);
	if (it_package != package->symbols().end()) {
		return WeakReference::share(it_package->second);
	}

	static GlobalData &g_global = GlobalData::instance();

	if (package != &g_global) {
		auto it_global = g_global.symbols().find(symbol);
		if (it_global != g_global.symbols().end()) {
			return WeakReference::share(it_global->second);
		}
	}

	return WeakReference::share((*symbols)[symbol]);
}

WeakReference mint::get_object_member(Cursor *cursor, const Reference &reference, const Symbol &member, Class **owner) {

	switch (reference.data()->format) {
	case Data::fmt_package:
		if (PackageData *package = reference.data<Package>()->data) {

			auto it_package = package->symbols().find(member);
			if (UNLIKELY(it_package == package->symbols().end())) {
				error("package '%s' has no member '%s'", package->name().c_str(), member.str().c_str());
			}

			if (owner) {
				*owner = nullptr;
			}

			return WeakReference::share(it_package->second);
		}

		break;

	case Data::fmt_object:
		if (Object *object = reference.data<Object>()) {

			auto it_member = object->metadata->members().find(member);
			if (it_member != object->metadata->members().end()) {
				if (is_object(object)) {

					Reference &result = object->data[it_member->second->offset];

					switch (result.flags() & Reference::visibility_mask) {
					case Reference::protected_visibility:
						if (UNLIKELY(!is_protected_accessible(it_member->second->owner, cursor->symbols().getMetadata()))) {
							error("could not access protected member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
						}
						break;
					case Reference::private_visibility:
						if (UNLIKELY(it_member->second->owner != cursor->symbols().getMetadata())) {
							error("could not access private member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
						}
						break;
					case Reference::package_visibility:
						if (UNLIKELY(it_member->second->owner->getPackage() != cursor->symbols().getPackage())) {
							error("could not access package member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
						}
						break;
					}

					if (owner) {
						*owner = it_member->second->owner;
					}

					return WeakReference::share(result);
				}
				else {

					if (UNLIKELY(cursor->symbols().getMetadata() == nullptr)) {
						error("could not access member '%s' of class '%s' without object", member.str().c_str(), object->metadata->name().c_str());
					}
					if (UNLIKELY(cursor->symbols().getMetadata()->bases().find(object->metadata) == cursor->symbols().getMetadata()->bases().end())) {
						error("class '%s' is not a direct base of '%s'", object->metadata->name().c_str(), cursor->symbols().getMetadata()->name().c_str());
					}
					if (it_member->second->value.flags() & Reference::private_visibility) {
						if (UNLIKELY(it_member->second->owner != cursor->symbols().getMetadata())) {
							error("could not access private member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
						}
					}

					if (owner) {
						*owner = it_member->second->owner;
					}

					WeakReference result(Reference::const_address | Reference::const_value | Reference::global);
					result.copy(it_member->second->value);
					return result;
				}
			}

			auto it_global = object->metadata->globals().find(member);
			if (it_global != object->metadata->globals().end()) {

				Reference &result = it_global->second->value;

				if (result.data()->format != Data::fmt_none) {
					switch (result.flags() & Reference::visibility_mask) {
					case Reference::protected_visibility:
						if (UNLIKELY(!is_protected_accessible(it_global->second->owner, cursor->symbols().getMetadata()))) {
							if (result.data()->format == Data::fmt_object && is_class(result.data<Object>())) {
								if (UNLIKELY(!is_protected_accessible(result.data<Object>()->metadata, cursor->symbols().getMetadata()))) {
									error("could not access protected member type '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
								}
							}
							else {
								error("could not access protected member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
							}
						}
						break;
					case Reference::private_visibility:
						if (UNLIKELY(it_global->second->owner != cursor->symbols().getMetadata())) {
							if (result.data()->format == Data::fmt_object && is_class(result.data<Object>())) {
								if (UNLIKELY(result.data<Object>()->metadata != cursor->symbols().getMetadata())) {
									error("could not access private member type '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
								}
							}
							else {
								error("could not access private member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
							}
						}
						break;
					case Reference::package_visibility:
						if (UNLIKELY(it_global->second->owner->getPackage() != cursor->symbols().getPackage())) {
							if (result.data()->format == Data::fmt_object && is_class(result.data<Object>())) {
								if (UNLIKELY(result.data<Object>()->metadata->getPackage() != cursor->symbols().getPackage())) {
									error("could not access package member type '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
								}
							}
							else {
								error("could not access package member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
							}
						}
						break;
					}
				}

				if (owner) {
					*owner = it_global->second->owner;
				}

				return WeakReference::share(result);
			}

			if (is_object(object)) {
				error("class '%s' has no member '%s'", object->metadata->name().c_str(), member.str().c_str());
			}
			else {
				error("class '%s' has no global member '%s'", object->metadata->name().c_str(), member.str().c_str());
			}
		}

		break;

	default:
		error("non class values dosen't have member '%s'", member.str().c_str());
	}

	return WeakReference();
}

WeakReference mint::get_object_operator(Cursor *cursor, const Reference &reference, Class::Operator op, Class **owner) {

	switch (reference.data()->format) {
	case Data::fmt_object:
		if (Class::MemberInfo *member = reference.data<Object>()->metadata->findOperator(op)) {

			Object *object = reference.data<Object>();

			if (is_object(object)) {

				Reference &result = object->data[member->offset];

				switch (result.flags() & Reference::visibility_mask) {
				case Reference::protected_visibility:
					if (UNLIKELY(!is_protected_accessible(member->owner, cursor->symbols().getMetadata()))) {
						error("could not access protected member '%s' of class '%s'", get_operator_symbol(op).str().c_str(), object->metadata->name().c_str());
					}
					break;
				case Reference::private_visibility:
					if (UNLIKELY(member->owner != cursor->symbols().getMetadata())) {
						error("could not access private member '%s' of class '%s'", get_operator_symbol(op).str().c_str(), object->metadata->name().c_str());
					}
					break;
				case Reference::package_visibility:
					if (UNLIKELY(member->owner->getPackage() != cursor->symbols().getPackage())) {
						error("could not access package member '%s' of class '%s'", get_operator_symbol(op).str().c_str(), object->metadata->name().c_str());
					}
					break;
				}

				if (owner) {
					*owner = member->owner;
				}

				return WeakReference::share(result);
			}
			else {

				if (UNLIKELY(cursor->symbols().getMetadata() == nullptr)) {
					error("could not access member '%s' of class '%s' without object", get_operator_symbol(op).str().c_str(), object->metadata->name().c_str());
				}
				if (UNLIKELY(cursor->symbols().getMetadata()->bases().find(object->metadata) == cursor->symbols().getMetadata()->bases().end())) {
					error("class '%s' is not a direct base of '%s'", object->metadata->name().c_str(), cursor->symbols().getMetadata()->name().c_str());
				}
				if (member->value.flags() & Reference::private_visibility) {
					if (UNLIKELY(member->owner != cursor->symbols().getMetadata())) {
						error("could not access private member '%s' of class '%s'", get_operator_symbol(op).str().c_str(), object->metadata->name().c_str());
					}
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
			error("class '%s' has no member '%s'", reference.data<Object>()->metadata->name().c_str(), get_operator_symbol(op).str().c_str());
		}
		else {
			error("class '%s' has no global member '%s'", reference.data<Object>()->metadata->name().c_str(), get_operator_symbol(op).str().c_str());
		}

	default:
		error("non class values dosen't have member '%s'", get_operator_symbol(op).str().c_str());
	}
}

void mint::reduce_member(Cursor *cursor, Reference &&member) {
	cursor->stack().back() = move(member);
}

Class::MemberInfo *mint::get_member_infos(Object *object, const Reference &member) {

	for (auto &infos : object->metadata->members()) {
		if (member.data() == infos.second->value.data()) {
			return infos.second;
		}
		if (member.data() == object->data[infos.second->offset].data()) {
			return infos.second;
		}
	}

	return nullptr;
}

bool mint::is_protected_accessible(Class *owner, Class *context) {
	return owner->isBaseOrSame(context) || (context && context->isBaseOf(owner));
}

Symbol mint::var_symbol(Cursor *cursor) {

	WeakReference var = move(cursor->stack().back());
	cursor->stack().pop_back();
	return Symbol(to_string(var));
}

void mint::create_symbol(Cursor *cursor, const Symbol &symbol, Reference::Flags flags) {

	if (flags & Reference::global) {

		PackageData *package = cursor->symbols().getPackage();

		auto it = package->symbols().find(symbol);
		if (it != package->symbols().end()) {
			if (UNLIKELY(it->second.data()->format != Data::fmt_none)) {
				error("symbol '%s' was already defined in global context", symbol.str().c_str());
			}
			package->symbols().erase(it);
		}

		cursor->stack().emplace_back(WeakReference::share(package->symbols().emplace(symbol, WeakReference(flags)).first->second));
	}
	else {

		auto it = cursor->symbols().find(symbol);
		if (it != cursor->symbols().end()) {
			if (UNLIKELY(it->second.data()->format != Data::fmt_none)) {
				error("symbol '%s' was already defined in this context", symbol.str().c_str());
			}
			cursor->symbols().erase(it);
		}

		cursor->stack().emplace_back(WeakReference::share(cursor->symbols().emplace(symbol, WeakReference(flags)).first->second));
	}
}
