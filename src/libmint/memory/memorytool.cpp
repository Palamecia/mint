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

size_t mint::get_stack_base(Cursor *cursor) {
	return cursor->stack().size() - 1;
}

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
	default:
		error("cannot open printer from '%s'", type_name(ref).c_str());
		break;
	}

	return nullptr;
}

void mint::print(Printer *printer, const SharedReference &reference) {

	if (printer) {

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
}

void mint::load_extra_arguments(Cursor *cursor) {

	SharedReference extra = move(cursor->stack().back());
	SharedReference args = SharedReference::strong(iterator_init(extra));

	cursor->stack().pop_back();

	while (SharedReference item = iterator_next(args->data<Iterator>())) {
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
				SharedReference instance = SharedReference::strong(Reference::standard);
				SharedReference prototype = move(cursor->stack().back());
				instance->clone(*prototype);
				object = instance->data<Object>();
				object->construct();
				cursor->stack().back() = move(instance);
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

		int required = -it->first;

		if (required <= 0) {
			return mapping.end();
		}

		Iterator *va_args = Reference::alloc<Iterator>();
		va_args->construct();

		for (int i = 0; i < (signature - required); ++i) {
			iterator_add(va_args, cursor->stack().back());
			cursor->stack().pop_back();
		}

		cursor->stack().emplace_back(SharedReference::strong(Reference::standard, va_args));
	}

	return it;
}

void mint::yield(Cursor *cursor) {

	Iterator *generator = cursor->symbols().generator();

	assert(generator);

	Cursor::ExecutionMode mode = cursor->executionMode();
	SharedReference defaultResult = SharedReference::weak(cursor->symbols().defaultResult());
	SharedReference item = move(cursor->stack().back());

	cursor->stack().pop_back();
	iterator_insert(generator, SharedReference::strong(item->flags(), item->data()));

	switch (mode) {
	case Cursor::single_pass:
		break;

	case Cursor::interruptible:
		cursor->stack().emplace_back(move(defaultResult));
		break;
	}
}

void mint::load_current_result(Cursor *cursor) {

	if (Iterator *generator = cursor->symbols().generator()) {

		SharedReference item = move(cursor->stack().back());
		cursor->setExecutionMode(Cursor::single_pass);
		cursor->stack().pop_back();

		iterator_insert(generator, move(item));
		load_default_result(cursor);
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

	auto it_global = g_global.symbols().find(symbol);
	if (it_global != g_global.symbols().end()) {
		return SharedReference::weak(it_global->second);
	}

	return SharedReference::weak((*symbols)[symbol]);
}

SharedReference mint::get_object_member(Cursor *cursor, const Reference &reference, const Symbol &member, Class **owner) {

	switch (reference.data()->format) {
	case Data::fmt_package:
		if (PackageData *package = reference.data<Package>()->data) {

			if (Class *desc = package->getClass(member)) {

				if (owner) {
					*owner = desc;
				}

				return SharedReference::strong(Reference::global, desc->makeInstance());
			}

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

			if (Class::TypeInfo *type = object->metadata->globals().getClass(member)) {
				if (type->flags & Reference::protected_visibility) {
					if (UNLIKELY(!is_protected_accessible(type->owner, cursor->symbols().getMetadata()) && !is_protected_accessible(type->description, cursor->symbols().getMetadata()))) {
						error("could not access protected member type '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
					}
				}
				else if (type->flags & Reference::private_visibility) {
					if (UNLIKELY(type->owner != cursor->symbols().getMetadata() && type->description != cursor->symbols().getMetadata())) {
						error("could not access private member type '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
					}
				}
				else if (type->flags & Reference::package_visibility) {
					if (UNLIKELY(type->owner->getPackage() != cursor->symbols().getPackage() && type->description->getPackage() != cursor->symbols().getPackage())) {
						error("could not access package member type '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
					}
				}

				if (owner) {
					*owner = type->owner;
				}

				return SharedReference::strong(Reference::global, type->description->makeInstance());
			}

			auto it_global = object->metadata->globals().members().find(member);
			if (it_global != object->metadata->globals().members().end()) {

				SharedReference result = SharedReference::weak(it_global->second->value);

				if (result->data()->format != Data::fmt_none) {
					if (result->flags() & Reference::protected_visibility) {
						if (UNLIKELY(!is_protected_accessible(it_global->second->owner, cursor->symbols().getMetadata()))) {
							error("could not access protected member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
						}
					}
					else if (result->flags() & Reference::private_visibility) {
						if (UNLIKELY(it_global->second->owner != cursor->symbols().getMetadata())) {
							error("could not access private member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
						}
					}
					else if (result->flags() & Reference::package_visibility) {
						if (UNLIKELY(it_global->second->owner->getPackage() != cursor->symbols().getPackage())) {
							error("could not access package member '%s' of class '%s'", member.str().c_str(), object->metadata->name().c_str());
						}
					}
				}

				if (owner) {
					*owner = it_global->second->owner;
				}

				return result;
			}

			if (is_class(object)) {

				auto it_member = object->metadata->members().find(member);
				if (LIKELY(it_member != object->metadata->members().end())) {

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

				error("class '%s' has no global member '%s'", object->metadata->name().c_str(), member.str().c_str());
			}

			auto it_member = object->metadata->members().find(member);
			if (UNLIKELY(it_member == object->metadata->members().end())) {
				error("class '%s' has no member '%s'", object->metadata->name().c_str(), member.str().c_str());
			}

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

		cursor->stack().emplace_back(SharedReference::weak(package->symbols().emplace(symbol, StrongReference(flags)).first->second));
	}
	else {

		auto it = cursor->symbols().find(symbol);
		if (it != cursor->symbols().end()) {
			if (UNLIKELY(it->second.data()->format != Data::fmt_none)) {
				error("symbol '%s' was already defined in this context", symbol.str().c_str());
			}
			cursor->symbols().erase(it);
		}

		cursor->stack().emplace_back(SharedReference::weak(cursor->symbols().emplace(symbol, StrongReference(flags)).first->second));
	}
}

void mint::array_append_from_stack(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &value = load_from_stack(cursor, base);
	SharedReference &array = load_from_stack(cursor, base - 1);

	array_append(array->data<Array>(), value);
	cursor->stack().pop_back();
}

void mint::array_append(Array *array, const SharedReference &item) {
	array->values.emplace_back(array_item(item));
}

SharedReference mint::array_get_item(Array *array, intmax_t index) {
	return SharedReference::weak(array->values[array_index(array, index)]);
}

SharedReference mint::array_get_item(Array::values_type::iterator &it) {
	return SharedReference::weak(*it);
}

SharedReference mint::array_get_item(Array::values_type::value_type &value) {
	return SharedReference::weak(value);
}

size_t mint::array_index(const Array *array, intmax_t index) {

	size_t i = (index < 0) ? static_cast<size_t>(index) + array->values.size() : static_cast<size_t>(index);

	if (UNLIKELY(i >= array->values.size())) {
		error("array index '%ld' is out of range", index);
	}

	return i;
}

WeakReference mint::array_item(const SharedReference &item) {

	StrongReference item_value;

	if (item->flags() & Reference::const_value) {
		item_value.copy(*item);
	}
	else {
		item_value.move(*item);
	}

	return item_value;
}

void mint::hash_insert_from_stack(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &value = load_from_stack(cursor, base);
	SharedReference &key = load_from_stack(cursor, base - 1);
	SharedReference &hash = load_from_stack(cursor, base - 2);

	hash_insert(hash->data<Hash>(), key, value);
	cursor->stack().pop_back();
	cursor->stack().pop_back();
}

Hash::values_type::iterator mint::hash_insert(Hash *hash, const Hash::key_type &key, const SharedReference &value) {
	return hash->values.emplace(hash_key(key), hash_value(value)).first;
}

SharedReference mint::hash_get_item(Hash *hash, const Hash::key_type &key) {

	auto i = hash->values.find(key);

	if (i == hash->values.end()) {
		i = hash_insert(hash, key, SharedReference::strong<None>());
	}

	return SharedReference::weak(i->second);
}

SharedReference mint::hash_get_key(Hash::values_type::iterator &it) {
	return SharedReference::weak(*it->first);
}

SharedReference mint::hash_get_key(Hash::values_type::value_type &item) {
	return SharedReference::weak(*item.first);
}

SharedReference mint::hash_get_value(Hash::values_type::iterator &it) {
	return SharedReference::weak(it->second);
}

SharedReference mint::hash_get_value(Hash::values_type::value_type &item) {
	return SharedReference::weak(item.second);
}

Hash::key_type mint::hash_key(const SharedReference &key) {
	return SharedReference::strong(Reference::const_address | Reference::const_value, key->data());
}

WeakReference mint::hash_value(const SharedReference &value) {

	WeakReference item_value;

	if (value->flags() & Reference::const_value) {
		item_value.copy(*value);
	}
	else {
		item_value.move(*value);
	}

	return item_value;
}

void mint::iterator_init_from_stack(Cursor *cursor, size_t length) {

	SharedReference it = SharedReference::strong(Reference::const_address, Reference::alloc<Iterator>());
	it->data<Iterator>()->construct();

	for (size_t i = 0; i < length; ++i) {
		iterator_add(it->data<Iterator>(), cursor->stack().back());
		cursor->stack().pop_back();
	}

	cursor->stack().emplace_back(move(it));
}

Iterator *mint::iterator_init(SharedReference &ref) {

	Iterator *iterator = nullptr;

	switch (ref->data()->format) {
	case Data::fmt_none:
		iterator = Reference::alloc<Iterator>();
		iterator->construct();
		break;
	case Data::fmt_object:
		switch (ref->data<Object>()->metadata->metatype()) {
		case Class::string:
			iterator = Reference::alloc<Iterator>();
			iterator->construct();
			for (utf8iterator i = ref->data<String>()->str.begin(); i != ref->data<String>()->str.end(); ++i) {
				iterator_insert(iterator, create_string(*i));
			}
			return iterator;
		case Class::array:
			iterator = Reference::alloc<Iterator>();
			iterator->construct();
			for (auto &item : ref->data<Array>()->values) {
				iterator_insert(iterator, array_get_item(item));
			}
			return iterator;
		case Class::hash:
			iterator = Reference::alloc<Iterator>();
			iterator->construct();
			for (auto &item : ref->data<Hash>()->values) {
				Iterator *element = Reference::alloc<Iterator>();
				element->construct();
				iterator_insert(element, hash_get_key(item));
				iterator_insert(element, hash_get_value(item));
				iterator_insert(iterator, SharedReference::strong(element));
			}
			return iterator;
		case Class::iterator:
			return ref->data<Iterator>();
		default:
			break;
		}
	default:
		iterator = Reference::alloc<Iterator>();
		iterator->construct();
		iterator_insert(iterator, move(ref));
		break;
	}

	return iterator;
}

Iterator *mint::iterator_init(SharedReference &&ref) {
	return iterator_init(static_cast<SharedReference &>(ref));
}

void mint::iterator_insert(Iterator *iterator, SharedReference &&item) {
	iterator->ctx.emplace_back(item);
}

void mint::iterator_add(Iterator *iterator, SharedReference &item) {
	iterator->ctx.emplace_front(item);
}

SharedReference mint::iterator_get(Iterator *iterator) {

	if (!iterator->ctx.empty()) {
		return SharedReference::weak(*iterator->ctx.front());
	}

	return nullptr;
}

SharedReference mint::iterator_next(Iterator *iterator) {

	if (!iterator->ctx.empty()) {
		SharedReference item = move(iterator->ctx.front());
		iterator->ctx.pop_front();
		return item;
	}

	return nullptr;
}

void mint::iterator_finalize(SharedReference &ref) {
	if (ref->data()->format == Data::fmt_object) {
		if (ref->data<Object>()->metadata->metatype() == Class::iterator) {
			ref->data<Iterator>()->ctx.finalize();
		}
	}
}
