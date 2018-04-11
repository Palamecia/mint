#include "memory/memorytool.h"
#include "memory/globaldata.h"
#include "memory/functiontool.h"
#include "memory/objectprinter.h"
#include "memory/casttool.h"
#include "memory/builtin/string.h"
#include "ast/cursor.h"
#include "system/utf8iterator.h"
#include "system/fileprinter.h"
#include "system/error.h"

using namespace std;
using namespace mint;

size_t mint::get_stack_base(Cursor *cursor) {
	return cursor->stack().size() - 1;
}

string mint::type_name(const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
		return "none";
	case Data::fmt_null:
		return "null";
	case Data::fmt_number:
		return "number";
	case Data::fmt_boolean:
		return "boolean";
	case Data::fmt_object:
		return ref.data<Object>()->metadata->name();
	case Data::fmt_function:
		return "function";
	}

	return string();
}

bool mint::is_class(Object *data) {
	return data->data == nullptr;
}

bool mint::is_object(Object *data) {
	return data->data != nullptr;
}

Printer *mint::create_printer(Cursor *cursor) {

	SharedReference ref = cursor->stack().back();
	cursor->stack().pop_back();

	switch (ref->data()->format) {
	case Data::fmt_number:
		return new FilePrinter((int)ref->data<Number>()->value);
	case Data::fmt_object:
		switch (ref->data<Object>()->metadata->metatype()) {
		case Class::string:
			return new FilePrinter(ref->data<String>()->str.c_str());
		case Class::object:
			return new ObjectPrinter(cursor, ref->data<Object>());
		default:
			break;
		}
	default:
		error("cannot open printer from '%s'", type_name(*ref).c_str());
		break;
	}

	return nullptr;
}

void mint::print(Printer *printer, SharedReference ref) {

	if (printer) {
		switch (ref->data()->format) {
		case Data::fmt_none:
			printer->print(Printer::none);
			break;
		case Data::fmt_null:
			printer->print(Printer::null);
			break;
		case Data::fmt_number:
			printer->print(ref->data<Number>()->value);
			break;
		case Data::fmt_boolean:
			printer->print(to_string(*ref).c_str());
			break;
		case Data::fmt_object:
			switch (ref->data<Object>()->metadata->metatype()) {
			case Class::string:
				printer->print(ref->data<String>()->str.c_str());
				break;
			case Class::array:
				printer->print(to_string(*ref).c_str());
				break;
			case Class::hash:
				printer->print(to_string(*ref).c_str());
				break;
			default:
				printer->print(ref->data());
			}
			break;
		case Data::fmt_function:
			printer->print(Printer::function);
			break;
		}
	}
}

void mint::capture_symbol(Cursor *cursor, const char *symbol) {

	SharedReference &function = cursor->stack().back();

	for (auto &signature : function->data<Function>()->mapping) {
		auto item = cursor->symbols().find(symbol);
		if (item != cursor->symbols().end()) {
			auto result = signature.second.capture->insert(*item);
			if (!result.second) {
				result.first->second.clone(item->second);
			}
		}
	}
}

void mint::capture_all_symbols(Cursor *cursor) {

	SharedReference &function = cursor->stack().back();

	for (auto &signature : function->data<Function>()->mapping) {
		for (auto item : cursor->symbols()) {
			auto result = signature.second.capture->insert(item);
			if (!result.second) {
				result.first->second.clone(item.second);
			}
		}
	}
}

void mint::init_call(Cursor *cursor) {

	if (cursor->stack().back()->data()->format == Data::fmt_object) {

		Object *object = cursor->stack().back()->data<Object>();

		if (is_class(object)) {

			Reference *instance = new Reference();
			instance->clone(*cursor->stack().back());
			object = instance->data<Object>();
			object->construct();
			cursor->stack().pop_back();
			cursor->stack().push_back(SharedReference::unique(instance));

			auto it = object->metadata->members().find("new");
			if (it != object->metadata->members().end()) {

				if (it->second->value.flags() & Reference::user_hiden) {
					if (object->metadata != cursor->symbols().getMetadata()) {
						error("could not access protected member 'new' of class '%s'", object->metadata->name().c_str());
					}
				}
				else if (it->second->value.flags() & Reference::child_hiden) {
					if (it->second->owner != cursor->symbols().getMetadata()) {
						error("could not access private member 'new' of class '%s'", object->metadata->name().c_str());
					}
				}

				cursor->waitingCalls().push(&object->data[it->second->offset]);
			}
			else {
				cursor->waitingCalls().push(SharedReference::unique(Reference::create<None>()));
			}
		}
		else {
			auto it = object->metadata->members().find("()");
			if (it != object->metadata->members().end()) {
				cursor->waitingCalls().push(&object->data[it->second->offset]);
			}
			else {
				error("class '%s' dosen't ovreload operator '()'", object->metadata->name().c_str());
			}
		}

		cursor->waitingCalls().top().setMember(true);
	}
	else {
		cursor->waitingCalls().push(cursor->stack().back());
		cursor->stack().pop_back();
	}
}

void mint::exit_call(Cursor *cursor) {

	if (!cursor->stack().back().isUnique()) {
		Reference &lvalue = *cursor->stack().back();
		Reference *rvalue = new Reference(lvalue);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(rvalue));
	}

	cursor->exitCall();
}

void mint::init_parameter(Cursor *cursor, const string &symbol) {

	SharedReference value = cursor->stack().back();
	cursor->stack().pop_back();

	if (value->flags() & Reference::const_value) {
		cursor->symbols()[symbol].copy(*value);
	}
	else {
		cursor->symbols()[symbol].move(*value);
	}
}

Function::mapping_type::iterator mint::find_function_signature(Cursor *cursor, Function::mapping_type &mapping, int signature) {

	auto it = mapping.find(signature);

	if (it != mapping.end()) {
		return it;
	}

	for (int required = 1; required <= signature; ++required) {

		it = mapping.find(-required);

		if (it != mapping.end()) {

			Iterator *va_args = Reference::alloc<Iterator>();
			va_args->construct();

			for (int i = 0; i < (signature - required); ++i) {
				iterator_add(va_args, cursor->stack().back());
				cursor->stack().pop_back();
			}

			cursor->stack().push_back(SharedReference::unique(new Reference(Reference::standard, va_args)));
			return it;
		}
	}

	return it;
}

void mint::yield(Cursor *cursor) {

	Reference &default_result = cursor->symbols().defaultResult();
	if (default_result.data()->format == Data::fmt_none) {
		default_result.clone(Reference(Reference::const_ref | Reference::const_value, Reference::alloc<Iterator>()));
	}

	iterator_insert(default_result.data<Iterator>(), SharedReference::unique(new Reference(*cursor->stack().back())));
	cursor->stack().pop_back();
}

void mint::load_default_result(Cursor *cursor) {
	cursor->stack().push_back(SharedReference::unique(new Reference(cursor->symbols().defaultResult())));
}

SharedReference mint::get_symbol_reference(SymbolTable *symbols, const string &symbol) {

	if (Class *desc = GlobalData::instance().getClass(symbol)) {
		return SharedReference::unique(new Reference(Reference::standard, desc->makeInstance()));
	}

	auto it = GlobalData::instance().symbols().find(symbol);
	if (it != GlobalData::instance().symbols().end()) {
		return &it->second;
	}

	return &(*symbols)[symbol];
}

SharedReference mint::get_object_member(Cursor *cursor, const string &member) {

	Reference *result = nullptr;
	Reference &lvalue = *cursor->stack().back();

	if (lvalue.data()->format != Data::fmt_object) {
		error("non class values dosen't have member '%s'", member.c_str());
	}

	Object *object = lvalue.data<Object>();

	if (Class *desc = object->metadata->globals().getClass(member)) {
		return SharedReference::unique(new Reference(Reference::standard, desc->makeInstance()));
	}

	auto it_global = object->metadata->globals().members().find(member);
	if (it_global != object->metadata->globals().members().end()) {

		result = &it_global->second->value;

		if (result->data()->format != Data::fmt_none) {
			if (result->flags() & Reference::user_hiden) {
				if (object->metadata != cursor->symbols().getMetadata()) {
					error("could not access protected member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
				}
			}
			else if (result->flags() & Reference::child_hiden) {
				if (it_global->second->owner != cursor->symbols().getMetadata()) {
					error("could not access private member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
				}
			}
		}

		return result;
	}

	if (is_class(object)) {

		auto it_member = object->metadata->members().find(member);
		if (it_member != object->metadata->members().end()) {

			if (cursor->symbols().getMetadata() == nullptr) {
				error("could not access member '%s' of class '%s' without object", member.c_str(), object->metadata->name().c_str());
			}
			if (cursor->symbols().getMetadata()->parents().find(object->metadata) == cursor->symbols().getMetadata()->parents().end()) {
				error("class '%s' is not a direct base of '%s'", object->metadata->name().c_str(), cursor->symbols().getMetadata()->name().c_str());
			}
			if (it_member->second->value.flags() & Reference::child_hiden) {
				if (it_member->second->owner != cursor->symbols().getMetadata()) {
					error("could not access private member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
				}
			}

			result = new Reference(Reference::const_ref | Reference::const_value | Reference::global);
			result->copy(it_member->second->value);
			return SharedReference::unique(result);
		}

		error("class '%s' has no global member '%s'", object->metadata->name().c_str(), member.c_str());
	}

	auto it_member = object->metadata->members().find(member);
	if (it_member == object->metadata->members().end()) {
		error("class '%s' has no member '%s'", object->metadata->name().c_str(), member.c_str());
	}

	result = &object->data[it_member->second->offset];

	if (result->flags() & Reference::user_hiden) {
		if (object->metadata != cursor->symbols().getMetadata()) {
			error("could not access protected member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
		}
	}
	else if (result->flags() & Reference::child_hiden) {
		if (it_member->second->owner != cursor->symbols().getMetadata()) {
			error("could not access private member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
		}
	}

	return result;
}

void mint::reduce_member(Cursor *cursor) {

	SharedReference member = cursor->stack().back();
	cursor->stack().pop_back();
	cursor->stack().pop_back();
	cursor->stack().push_back(member);
}

string mint::var_symbol(Cursor *cursor) {

	Reference var = *cursor->stack().back();
	cursor->stack().pop_back();

	return to_string(var);
}

void mint::create_symbol(Cursor *cursor, const string &symbol, Reference::Flags flags) {

	if (flags & Reference::global) {

		auto it = GlobalData::instance().symbols().find(symbol);
		if (it != GlobalData::instance().symbols().end()) {
			if (it->second.data()->format != Data::fmt_none) {
				error("symbol '%s' was already defined in global context", symbol.c_str());
			}
			GlobalData::instance().symbols().erase(it);
		}

		cursor->stack().push_back(&GlobalData::instance().symbols().emplace(symbol, Reference(flags)).first->second);
	}
	else {

		auto it = cursor->symbols().find(symbol);
		if (it != cursor->symbols().end()) {
			if (it->second.data()->format != Data::fmt_none) {
				error("symbol '%s' was already defined in this context", symbol.c_str());
			}
			cursor->symbols().erase(it);
		}

		cursor->stack().push_back(&cursor->symbols().emplace(symbol, Reference(flags)).first->second);
	}
}

void mint::array_append_from_stack(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &value = cursor->stack().at(base);
	Reference &array = *cursor->stack().at(base - 1);

	array_append(array.data<Array>(), value);
	cursor->stack().pop_back();
}

void mint::array_append(Array *array, const SharedReference &item) {
	array->values.push_back(SharedReference::unique(new Reference(item->flags() & ~Reference::const_ref, item->data())));
}

SharedReference mint::array_get_item(const Array *array, long index) {
	return array->values[array_index(array, index)].get();
}

size_t mint::array_index(const Array *array, long index) {

	size_t i = (index < 0) ? index + array->values.size() : index;

	if (i >= array->values.size()) {
		error("array index '%ld' is out of range", index);
	}

	return i;
}

void mint::hash_insert_from_stack(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &value = cursor->stack().at(base);
	SharedReference &key = cursor->stack().at(base - 1);
	Reference &hash = *cursor->stack().at(base - 2);

	hash_insert(hash.data<Hash>(), key, value);
	cursor->stack().pop_back();
	cursor->stack().pop_back();
}

void mint::hash_insert(Hash *hash, const Hash::key_type &key, const SharedReference &value) {

	Reference *key_value = new Reference(Reference::const_ref | Reference::const_value);
	key_value->clone(*key);
	hash->values.emplace(SharedReference::unique(key_value),
						 SharedReference::unique(new Reference(value->flags() & ~Reference::const_ref, value->data())));
}

SharedReference mint::hash_get_item(Hash *hash, const Hash::key_type &key) {
	return hash->values[key].get();
}

Hash::key_type mint::hash_get_key(const Hash::values_type::value_type &item) {
	return item.first.get();
}

SharedReference mint::hash_get_value(const Hash::values_type::value_type &item) {
	return item.second.get();
}

void mint::iterator_init(Cursor *cursor, size_t length) {

	Reference *it = new Reference(Reference::const_ref, Reference::alloc<Iterator>());
	it->data<Object>()->construct();

	for (size_t i = 0; i < length; ++i) {
		it->data<Iterator>()->ctx.push_front(cursor->stack().back());
		cursor->stack().pop_back();
	}

	cursor->stack().push_back(SharedReference::unique(it));
}

void mint::iterator_init(Iterator *iterator, const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
		break;
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::string:
			for (const_utf8iterator i = ref.data<String>()->str.begin(); i != ref.data<String>()->str.end(); ++i) {
				iterator_insert(iterator, create_string(*i));
			}
			return;
		case Class::array:
			for (auto &item : ref.data<Array>()->values) {
				iterator_insert(iterator, item.get());
			}
			return;
		case Class::hash:
			for (auto &item : ref.data<Hash>()->values) {
				iterator_insert(iterator, hash_get_key(item));
			}
			return;
		case Class::iterator:
			iterator->ctx = ref.data<Iterator>()->ctx;
			return;
		default:
			break;
		}
	default:
		iterator_insert(iterator, (Reference *)&ref);
		break;
	}
}

void mint::iterator_insert(Iterator *iterator, const SharedReference &item) {
	iterator->ctx.push_back(item);
}

void mint::iterator_add(Iterator *iterator, const SharedReference &item) {
	iterator->ctx.push_front(item);
}

bool mint::iterator_next(Iterator *iterator, SharedReference &item) {

	if (iterator->ctx.empty()) {
		return false;
	}

	item = iterator->ctx.front();
	iterator->ctx.pop_front();
	return true;
}

void mint::regex_match(Cursor *cursor) {
	((void)cursor);
	error("regex are not supported in this version");
}

void mint::regex_unmatch(Cursor *cursor) {
	((void)cursor);
	error("regex are not supported in this version");
}
