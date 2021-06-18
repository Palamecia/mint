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

string mint::type_name(const SharedReference &reference) {

	switch (reference->data()->format) {
	case Data::fmt_none:
		return "none";
	case Data::fmt_null:
		return "null";
	case Data::fmt_number:
		return "number";
	case Data::fmt_boolean:
		return "boolean";
	case Data::fmt_object:
		return reference->data<Object>()->metadata->name();
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

	SharedReference ref = move(cursor->stack().back());
	cursor->stack().pop_back();

	switch (ref->data()->format) {
	case Data::fmt_number:
		return new FilePrinter(static_cast<int>(ref->data<Number>()->value));
	case Data::fmt_object:
		switch (ref->data<Object>()->metadata->metatype()) {
		case Class::string:
			return new FilePrinter(ref->data<String>()->str.c_str());
		case Class::object:
			return new ObjectPrinter(cursor, ref->flags(), ref->data<Object>());
		default:
			break;
		}
		fall_through;
	default:
		error("cannot open printer from '%s'", type_name(ref).c_str());
	}
}

void mint::print(Printer *printer, SharedReference &&reference) {

	assert(printer);

	Printer::DataType type = Printer::none;

	switch (reference->data()->format) {
	case Data::fmt_none:
		type = Printer::none;
		break;

	case Data::fmt_null:
		type = Printer::null;
		break;

	case Data::fmt_number:
		printer->print(reference->data<Number>()->value);
		return;

	case Data::fmt_boolean:
		printer->print(reference->data<Boolean>()->value);
		return;

	case Data::fmt_object:
		switch (reference->data<Object>()->metadata->metatype()) {
		case Class::string:
			printer->print(reference->data<String>()->str.c_str());
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

	if (!printer->print(type, reference->data())) {
		printer->print(to_string(reference).c_str());
	}
}

void mint::load_extra_arguments(Cursor *cursor) {

	SharedReference extra = move(cursor->stack().back());
	SharedReference args = SharedReference::strong(iterator_init(extra));

	cursor->stack().pop_back();

	while (SharedReference &&item = iterator_next(args->data<Iterator>())) {
		cursor->stack().emplace_back(SharedReference::strong(item->flags(), item->data()));
		cursor->waitingCalls().top().addExtraArgument();
	}
}

void mint::capture_symbol(Cursor *cursor, const Symbol &symbol) {

	SharedReference &function = cursor->stack().back();

	for (auto &signature : function->data<Function>()->mapping) {
		signature.second.capture->erase(symbol);
		auto item = cursor->symbols().find(symbol);
		if (item != cursor->symbols().end()) {
			signature.second.capture->insert(*item);
		}
	}
}

void mint::capture_all_symbols(Cursor *cursor) {

	SharedReference &function = cursor->stack().back();

	for (auto &signature : function->data<Function>()->mapping) {
		signature.second.capture->clear();
		for (auto &item : cursor->symbols()) {
			signature.second.capture->insert(item);
		}
	}
}

void mint::init_call(Cursor *cursor) {

	if (cursor->stack().back()->data()->format != Data::fmt_object) {
		cursor->waitingCalls().emplace(move(cursor->stack().back()));
		cursor->stack().pop_back();
	}
	else {

		Object *object = cursor->stack().back()->data<Object>();
		Cursor::Call::Flags flags = Cursor::Call::member_call;
		Class *metadata = object->metadata;

		if (is_class(object)) {

			if (metadata->metatype() == Class::object) {
				SharedReference prototype = move(cursor->stack().back());
				WeakReference instance(*prototype, Reference::copy_tag());
				cursor->stack().back() = SharedReference::strong(instance);
				object = instance.data<Object>();
				object->construct();
			}
			else {
				/*
				 * Builtin classes can not be aliased, there is no need to clone the prototype.
				 */
				object->construct();
			}

			auto it = metadata->members().find(Symbol::New);
			if (it != metadata->members().end()) {

				if (it->second->value.flags() & Reference::protected_visibility) {
					if (UNLIKELY(!is_protected_accessible(it->second->owner, cursor->symbols().getMetadata()))) {
						error("could not access protected member 'new' of class '%s'", metadata->name().c_str());
					}
				}
				else if (it->second->value.flags() & Reference::private_visibility) {
					if (UNLIKELY(it->second->owner != cursor->symbols().getMetadata())) {
						error("could not access private member 'new' of class '%s'", metadata->name().c_str());
					}
				}
				else if (it->second->value.flags() & Reference::package_visibility) {
					if (UNLIKELY(it->second->owner->getPackage() != cursor->symbols().getPackage())) {
						error("could not access package member 'new' of class '%s'", metadata->name().c_str());
					}
				}

				cursor->waitingCalls().emplace(SharedReference::weak(object->data[it->second->offset]));
				metadata = it->second->owner;
			}
			else {
				cursor->waitingCalls().emplace(SharedReference::strong<None>());
			}
		}
		else {
			auto it = metadata->members().find(Symbol::CallOperator);
			if (LIKELY(it != metadata->members().end())) {
				cursor->waitingCalls().emplace(SharedReference::weak(object->data[it->second->offset]));
				flags |= Cursor::Call::operator_call;
				metadata = it->second->owner;
			}
			else {
				error("class '%s' dosen't ovreload operator '()'", metadata->name().c_str());
			}
		}

		Cursor::Call &call = cursor->waitingCalls().top();
		call.setMetadata(metadata);
		call.setFlags(flags);
	}
}

void mint::init_member_call(Cursor *cursor, const Symbol &member) {

	Class *owner = nullptr;
	SharedReference &reference = cursor->stack().back();
	SharedReference function = get_object_member(cursor, *reference, member, &owner);

	if (function->flags() & Reference::global) {
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

void mint::init_parameter(Cursor *cursor, const Symbol &symbol) {

	SharedReference &value = cursor->stack().back();
	SymbolTable &symbols = cursor->symbols();

	if (value->flags() & Reference::const_value) {
		symbols[symbol].copy(*value);
	}
	else {
		symbols[symbol].move(*value);
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

		cursor->stack().emplace_back(SharedReference::strong(Reference::standard, va_args));
	}

	return it;
}

void mint::yield(Cursor *cursor) {

	Iterator *generator = cursor->symbols().generator();

	assert(generator);

	const Cursor::ExecutionMode mode = cursor->executionMode();
	SharedReference defaultResult = SharedReference::weak(cursor->symbols().defaultResult());
	SharedReference item = move(cursor->stack().back());

	cursor->stack().pop_back();
	iterator_insert(generator, SharedReference::strong(item->flags(), item->data()));

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
		cursor->stack().emplace_back(SharedReference::strong(cursor->symbols().defaultResult()));
		break;
	}
}

void mint::load_current_result(Cursor *cursor) {

	Iterator *generator = cursor->symbols().generator();

	if (UNLIKELY(generator != nullptr)) {

		const Cursor::ExecutionMode mode = cursor->executionMode();
		SharedReference defaultResult = SharedReference::strong(cursor->symbols().defaultResult());
		SharedReference item = move(cursor->stack().back());

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
	cursor->stack().emplace_back(SharedReference::strong(cursor->symbols().defaultResult()));
}

SharedReference mint::get_symbol_reference(SymbolTable *symbols, const Symbol &symbol) {

	auto it_local = symbols->find(symbol);
	if (it_local != symbols->end()) {
		return SharedReference::weak(it_local->second);
	}

	PackageData *package = symbols->getPackage();

	auto it_package = package->symbols().find(symbol);
	if (it_package != package->symbols().end()) {
		return SharedReference::weak(it_package->second);
	}

	static GlobalData &g_global = GlobalData::instance();

	if (package != &g_global) {
		auto it_global = g_global.symbols().find(symbol);
		if (it_global != g_global.symbols().end()) {
			return SharedReference::weak(it_global->second);
		}
	}

	return SharedReference::weak((*symbols)[symbol]);
}

SharedReference mint::get_object_member(Cursor *cursor, const Reference &reference, const Symbol &member, Class **owner) {

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

			return SharedReference::weak(it_package->second);
		}

		break;

	case Data::fmt_object:
		if (Object *object = reference.data<Object>()) {

			auto it_member = object->metadata->members().find(member);
			if (it_member != object->metadata->members().end()) {
				if (is_object(object)) {

					SharedReference result = SharedReference::weak(object->data[it_member->second->offset]);

					if (result->flags() & Reference::protected_visibility) {
						if (UNLIKELY(!is_protected_accessible(it_member->second->owner, cursor->symbols().getMetadata()))) {
							error("could not access protected member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
						}
					}
					else if (result->flags() & Reference::private_visibility) {
						if (UNLIKELY(it_member->second->owner != cursor->symbols().getMetadata())) {
							error("could not access private member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
						}
					}
					else if (result->flags() & Reference::package_visibility) {
						if (UNLIKELY(it_member->second->owner->getPackage() != cursor->symbols().getPackage())) {
							error("could not access package member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
						}
					}

					if (owner) {
						*owner = it_member->second->owner;
					}

					return result;
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

					SharedReference result = SharedReference::strong(Reference::const_address | Reference::const_value | Reference::global);
					result->copy(it_member->second->value);
					return result;
				}
			}

			auto it_global = object->metadata->globals().members().find(member);
			if (it_global != object->metadata->globals().members().end()) {

				SharedReference result = SharedReference::weak(it_global->second->value);

				if (result->data()->format != Data::fmt_none) {
					if (result->flags() & Reference::protected_visibility) {
						if (UNLIKELY(!is_protected_accessible(it_global->second->owner, cursor->symbols().getMetadata()))) {
							if (result->data()->format == Data::fmt_object && is_class(result->data<Object>())) {
								if (UNLIKELY(!is_protected_accessible(result->data<Object>()->metadata, cursor->symbols().getMetadata()))) {
									error("could not access protected member type '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
								}
							}
							else {
								error("could not access protected member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
							}
						}
					}
					else if (result->flags() & Reference::private_visibility) {
						if (UNLIKELY(it_global->second->owner != cursor->symbols().getMetadata())) {
							if (result->data()->format == Data::fmt_object && is_class(result->data<Object>())) {
								if (UNLIKELY(result->data<Object>()->metadata != cursor->symbols().getMetadata())) {
									error("could not access private member type '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
								}
							}
							else {
								error("could not access private member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
							}
						}
					}
					else if (result->flags() & Reference::package_visibility) {
						if (UNLIKELY(it_global->second->owner->getPackage() != cursor->symbols().getPackage())) {
							if (result->data()->format == Data::fmt_object && is_class(result->data<Object>())) {
								if (UNLIKELY(result->data<Object>()->metadata->getPackage() != cursor->symbols().getPackage())) {
									error("could not access package member type '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
								}
							}
							else {
								error("could not access package member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
							}
						}
					}
				}

				if (owner) {
					*owner = it_global->second->owner;
				}

				return result;
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

	return nullptr;
}

void mint::reduce_member(Cursor *cursor, SharedReference &&member) {
	cursor->stack().back() = move(member);
}

Class::MemberInfo *mint::get_member_infos(Object *object, const SharedReference &member) {

	for (auto &infos : object->metadata->members()) {
		if (member->data() == infos.second->value.data()) {
			return infos.second;
		}
		if (member->data() == object->data[infos.second->offset].data()) {
			return infos.second;
		}
	}

	return nullptr;
}

bool mint::is_protected_accessible(Class *owner, Class *context) {
	return owner->isBaseOrSame(context) || (context && context->isBaseOf(owner));
}

Symbol mint::var_symbol(Cursor *cursor) {

	SharedReference var = move(cursor->stack().back());
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

		cursor->stack().emplace_back(SharedReference::weak(package->symbols().emplace(symbol, WeakReference(flags)).first->second));
	}
	else {

		auto it = cursor->symbols().find(symbol);
		if (it != cursor->symbols().end()) {
			if (UNLIKELY(it->second.data()->format != Data::fmt_none)) {
				error("symbol '%s' was already defined in this context", symbol.str().c_str());
			}
			cursor->symbols().erase(it);
		}

		cursor->stack().emplace_back(SharedReference::weak(cursor->symbols().emplace(symbol, WeakReference(flags)).first->second));
	}
}
