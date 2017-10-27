#include "memory/memorytool.h"
#include "memory/globaldata.h"
#include "memory/casttool.h"
#include "memory/builtin/string.h"
#include "ast/cursor.h"
#include "system/utf8iterator.h"
#include "system/error.h"

using namespace std;

size_t get_base(Cursor *cursor) {
	return cursor->stack().size() - 1;
}

string type_name(const Reference &ref) {

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
		return ((Object *)ref.data())->metadata->name();
	case Data::fmt_function:
		return "function";
	}

	return string();
}

Printer *to_printer(SharedReference ref) {

	switch (ref->data()->format) {
	case Data::fmt_number:
		return new Printer((int)((Number*)ref->data())->value);
	case Data::fmt_object:
		if (((Object *)ref->data())->metadata->metatype() == Class::string) {
			return new Printer(((String *)ref->data())->str.c_str());
		}
	default:
		error("cannot open printer from '%s'", type_name(*ref).c_str());
		break;
	}

	return nullptr;
}

void print(Printer *printer, SharedReference ref) {

	if (printer) {
		switch (ref->data()->format) {
		case Data::fmt_none:
			printer->print(Printer::none);
			break;
		case Data::fmt_null:
			printer->print(Printer::null);
			break;
		case Data::fmt_number:
			printer->print(((Number*)ref->data())->value);
			break;
		case Data::fmt_boolean:
			printer->print(to_string(*ref).c_str());
			break;
		case Data::fmt_object:
			switch (((Object *)ref->data())->metadata->metatype()) {
			case Class::string:
				printer->print(((String *)ref->data())->str.c_str());
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

void capture_symbol(Cursor *cursor, const char *symbol) {

	SharedReference &function = cursor->stack().back();

	for (auto &signature : ((Function *)function->data())->mapping) {
		auto item = cursor->symbols().find(symbol);
		if (item != cursor->symbols().end()) {
			auto result = signature.second.capture->insert(*item);
			if (!result.second) {
				result.first->second.clone(item->second);
			}
		}
	}
}

void capture_all_symbols(Cursor *cursor) {

	SharedReference &function = cursor->stack().back();

	for (auto &signature : ((Function *)function->data())->mapping) {
		for (auto item : cursor->symbols()) {
			auto result = signature.second.capture->insert(item);
			if (!result.second) {
				result.first->second.clone(item.second);
			}
		}
	}
}

void init_call(Cursor *cursor) {

	if (cursor->stack().back()->data()->format == Data::fmt_object) {

		Object *object = (Object *)cursor->stack().back()->data();
		if (object->data == nullptr) {
			object->construct();

			auto it = object->metadata->members().find("new");
			if (it != object->metadata->members().end()) {

				if (it->second->value.flags() & Reference::user_hiden) {
					if (object->metadata != cursor->symbols().metadata) {
						error("could not access protected member 'new' of class '%s'", object->metadata->name().c_str());
					}
				}
				else if (it->second->value.flags() & Reference::child_hiden) {
					if (it->second->owner != cursor->symbols().metadata) {
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

void exit_call(Cursor *cursor) {

	if (!cursor->stack().back().isUnique()) {
		Reference &lvalue = *cursor->stack().back();
		Reference *rvalue = new Reference(lvalue);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(rvalue));
	}

	cursor->exitCall();
}

void init_parameter(Cursor *cursor, const std::string &symbol) {

	SharedReference value = cursor->stack().back();
	cursor->stack().pop_back();

	if (value->flags() & Reference::const_value) {
		cursor->symbols()[symbol].copy(*value);
	}
	else {
		cursor->symbols()[symbol].move(*value);
	}
}

Function::mapping_type::iterator find_function_signature(Cursor *cursor, Function::mapping_type &mapping, int signature) {

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

void yield(Cursor *cursor) {

	Reference &default_result = cursor->symbols().defaultResult;
	if (default_result.data()->format == Data::fmt_none) {
		default_result.clone(Reference(Reference::const_ref | Reference::const_value, Reference::alloc<Iterator>()));
	}

	iterator_insert((Iterator *)default_result.data(), SharedReference::unique(new Reference(*cursor->stack().back())));
	cursor->stack().pop_back();
}

void load_default_result(Cursor *cursor) {
	cursor->stack().push_back(SharedReference::unique(new Reference(cursor->symbols().defaultResult)));
}

SharedReference get_symbol_reference(SymbolTable *symbols, const std::string &symbol) {

	if (Class *desc = GlobalData::instance().getClass(symbol)) {
		return SharedReference::unique(new Reference(Reference::standard, desc->makeInstance()));
	}

	auto it = GlobalData::instance().symbols().find(symbol);
	if (it != GlobalData::instance().symbols().end()) {
		return &it->second;
	}

	return &(*symbols)[symbol];
}

SharedReference get_object_member(Cursor *cursor, const std::string &member) {

	Reference *result = nullptr;
	Reference &lvalue = *cursor->stack().back();

	if (lvalue.data()->format != Data::fmt_object) {
		error("non class values dosen't have member '%s'", member.c_str());
	}

	Object *object = (Object *)lvalue.data();

	if (Class *desc = object->metadata->globals().getClass(member)) {
		return SharedReference::unique(new Reference(Reference::standard, desc->makeInstance()));
	}

	auto it_global = object->metadata->globals().members().find(member);
	if (it_global != object->metadata->globals().members().end()) {

		result = &it_global->second->value;

		if (result->data()->format != Data::fmt_none) {
			if (result->flags() & Reference::user_hiden) {
				if (object->metadata != cursor->symbols().metadata) {
					error("could not access protected member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
				}
			}
			else if (result->flags() & Reference::child_hiden) {
				if (it_global->second->owner != cursor->symbols().metadata) {
					error("could not access private member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
				}
			}
		}

		return result;
	}

	if (object->data == nullptr) {

		auto it_member = object->metadata->members().find(member);
		if (it_member != object->metadata->members().end()) {

			if (cursor->symbols().metadata == nullptr) {
				error("could not access member '%s' of class '%s' without object", member.c_str(), object->metadata->name().c_str());
			}
			if (cursor->symbols().metadata->parents().find(object->metadata) == cursor->symbols().metadata->parents().end()) {
				error("class '%s' is not a direct base of '%s'", object->metadata->name().c_str(), cursor->symbols().metadata->name().c_str());
			}
			if (it_member->second->value.flags() & Reference::child_hiden) {
				if (it_member->second->owner != cursor->symbols().metadata) {
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
		if (object->metadata != cursor->symbols().metadata) {
			error("could not access protected member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
		}
	}
	else if (result->flags() & Reference::child_hiden) {
		if (it_member->second->owner != cursor->symbols().metadata) {
			error("could not access private member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
		}
	}

	return result;
}

void reduce_member(Cursor *cursor) {

	SharedReference member = cursor->stack().back();
	cursor->stack().pop_back();
	cursor->stack().pop_back();
	cursor->stack().push_back(member);
}

string var_symbol(Cursor *cursor) {

	Reference var = *cursor->stack().back();
	cursor->stack().pop_back();

	return to_string(var);
}

void create_symbol(Cursor *cursor, const std::string &symbol, Reference::Flags flags) {

	if (flags & Reference::global) {

		auto it = GlobalData::instance().symbols().find(symbol);
		if (it != GlobalData::instance().symbols().end()) {
			if (it->second.data()->format != Data::fmt_none) {
				error("symbol '%s' was already defined in global context", symbol.c_str());
			}
			GlobalData::instance().symbols().erase(it);
		}

		cursor->stack().push_back(&GlobalData::instance().symbols().insert({symbol, Reference(flags)}).first->second);
	}
	else {

		auto it = cursor->symbols().find(symbol);
		if (it != cursor->symbols().end()) {
			if (it->second.data()->format != Data::fmt_none) {
				error("symbol '%s' was already defined in this context", symbol.c_str());
			}
			cursor->symbols().erase(it);
		}

		cursor->stack().push_back(&cursor->symbols().insert({symbol, Reference(flags)}).first->second);
	}
}

void array_append(Cursor *cursor) {

	size_t base = get_base(cursor);

	SharedReference &value = cursor->stack().at(base);
	Reference &array = *cursor->stack().at(base - 1);

	array_append((Array *)array.data(), value);
	cursor->stack().pop_back();
}

void array_append(Array *array, const SharedReference &item) {
	array->values.push_back(SharedReference::unique(new Reference(item->flags() & ~Reference::const_ref, item->data())));
}

SharedReference array_get_item(Array *array, long index) {
	return array->values[array_index(array, index)].get();
}

size_t array_index(Array *array, long index) {

	size_t i = (index < 0) ? index + array->values.size() : index;

	if (i >= array->values.size()) {
		error("array index '%ld' is out of range", index);
	}

	return i;
}

void hash_insert(Cursor *cursor) {

	size_t base = get_base(cursor);

	SharedReference &value = cursor->stack().at(base);
	SharedReference &key = cursor->stack().at(base - 1);
	Reference &hash = *cursor->stack().at(base - 2);

	hash_insert((Hash *)hash.data(), key, value);
	cursor->stack().pop_back();
	cursor->stack().pop_back();
}

void hash_insert(Hash *hash, const Hash::key_type &key, const SharedReference &value) {

	Reference *key_value = new Reference;
	key_value->clone(*key);
	hash->values.emplace(SharedReference::unique(key_value),
						 SharedReference::unique(new Reference(value->flags() & ~Reference::const_ref, value->data())));
}

SharedReference hash_get_item(Hash *hash, const Hash::key_type &key) {
	return hash->values[key].get();
}

Hash::key_type hash_get_key(const Hash::values_type::value_type &item) {
	return item.first;
}

SharedReference hash_get_value(const Hash::values_type::value_type &item) {
	return item.second.get();
}

void iterator_init(Cursor *cursor, size_t length) {

	Reference *it = new Reference(Reference::const_ref, Reference::alloc<Iterator>());
	((Object *)it->data())->construct();

	for (size_t i = 0; i < length; ++i) {
		((Iterator *)it->data())->ctx.push_front(cursor->stack().back());
		cursor->stack().pop_back();
	}

	cursor->stack().push_back(SharedReference::unique(it));
}

void iterator_init(Iterator *iterator, const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_object:
		switch (((Object *)ref.data())->metadata->metatype()) {
		case Class::string:
			for (utf8iterator it = ((String *)ref.data())->str.begin(); it != ((String *)ref.data())->str.end(); ++it) {
				Reference *item = Reference::create<String>();
				((String *)item->data())->construct();
				((String *)item->data())->str = *it;
				iterator_insert(iterator, SharedReference::unique(item));
			}
			return;
		case Class::array:
			for (auto &item : ((Array *)ref.data())->values) {
				iterator_insert(iterator, item.get());
			}
			return;
		case Class::hash:
			for (auto &item : ((Hash *)ref.data())->values) {
				iterator_insert(iterator, hash_get_key(item));
			}
			return;
		case Class::iterator:
			iterator->ctx = ((Iterator *)ref.data())->ctx;
			return;
		default:
			break;
		}
	default:
		iterator_insert(iterator, (Reference *)&ref);
		break;
	}
}

void iterator_insert(Iterator *iterator, const SharedReference &item) {
	iterator->ctx.push_back(item);
}

void iterator_add(Iterator *iterator, const SharedReference &item) {
	iterator->ctx.push_front(item);
}

bool iterator_next(Iterator *iterator, SharedReference &item) {

	if (iterator->ctx.empty()) {
		return false;
	}

	item = iterator->ctx.front();
	iterator->ctx.pop_front();
	return true;
}

void regex_match(Cursor *cursor) {
	((void)cursor);
	error("regex are not supported in this version");
}

void regex_unmatch(Cursor *cursor) {
	((void)cursor);
	error("regex are not supported in this version");
}
