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

	SharedReference ref = cursor->stack().back();
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

	SharedReference extra = cursor->stack().back();
	SharedReference args = SharedReference::unique(Reference::create(iterator_init(extra)));

	cursor->stack().pop_back();

	while (SharedReference item = iterator_next(args->data<Iterator>())) {
		Reference *argument = new Reference(item->flags(), item->data());
		cursor->stack().emplace_back(SharedReference::unique(argument));
		cursor->waitingCalls().top().addExtraArgument();
	}
}

void mint::capture_symbol(Cursor *cursor, const char *symbol) {

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
		for (auto item : cursor->symbols()) {
			signature.second.capture->insert(item);
		}
	}
}

void mint::init_call(Cursor *cursor) {

	if (cursor->stack().back()->data()->format == Data::fmt_object) {

		Object *object = cursor->stack().back()->data<Object>();

		if (is_class(object)) {

			if (object->metadata->metatype() == Class::object) {
				Reference *instance = new Reference();
				SharedReference prototype = cursor->stack().back();
				instance->clone(*prototype);
				object = instance->data<Object>();
				object->construct();
				cursor->stack().pop_back();
				cursor->stack().emplace_back(SharedReference::unique(instance));
			}
			else {
				/*
				 * Builtin classes can not be aliased, there is no need to clone the prototype.
				 */
				object->construct();
			}

			auto it = object->metadata->members().find("new");
			if (it != object->metadata->members().end()) {

				if (it->second->value.flags() & Reference::protected_visibility) {
					if (!it->second->owner->isBaseOrSame(cursor->symbols().getMetadata())) {
						error("could not access protected member 'new' of class '%s'", object->metadata->name().c_str());
					}
				}
				else if (it->second->value.flags() & Reference::private_visibility) {
					if (it->second->owner != cursor->symbols().getMetadata()) {
						error("could not access private member 'new' of class '%s'", object->metadata->name().c_str());
					}
				}
				else if (it->second->value.flags() & Reference::package_visibility) {
					if (it->second->owner->getPackage() != cursor->symbols().getPackage()) {
						error("could not access package member 'new' of class '%s'", object->metadata->name().c_str());
					}
				}

				cursor->waitingCalls().push(SharedReference::linked(object->referenceManager(), object->data + it->second->offset));
			}
			else {
				cursor->waitingCalls().push(SharedReference::unique(Reference::create<None>()));
			}
		}
		else {
			auto it = object->metadata->members().find("()");
			if (it != object->metadata->members().end()) {
				cursor->waitingCalls().push(SharedReference::linked(object->referenceManager(), object->data + it->second->offset));
			}
			else {
				error("class '%s' dosen't ovreload operator '()'", object->metadata->name().c_str());
			}
		}

		cursor->waitingCalls().top().setMember(true);
		cursor->waitingCalls().top().setMetadata(object->metadata);
	}
	else {
		cursor->waitingCalls().push(cursor->stack().back());
		cursor->stack().pop_back();
	}
}

void mint::init_member_call(Cursor *cursor, const string &member) {

	Class::MemberInfo infos;
	bool constructor = false;
	SharedReference &reference = cursor->stack().back();
	SharedReference function = get_object_member(cursor, *reference, member, &infos);

	if (function->data()->format == Data::fmt_object) {
		constructor = is_class(function->data<Object>());
	}

	if (function->flags() & Reference::global) {
		cursor->stack().pop_back();
	}

	cursor->stack().emplace_back(function);

	init_call(cursor);

	if (!constructor) {
		cursor->waitingCalls().top().setMetadata(infos.owner);
	}
}

void mint::exit_call(Cursor *cursor) {
	cursor->exitCall();
}

void mint::init_parameter(Cursor *cursor, const string &symbol) {

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

		cursor->stack().emplace_back(SharedReference::unique(new Reference(Reference::standard, va_args)));
	}

	return it;
}

void mint::yield(Cursor *cursor) {

	Iterator *generator = cursor->symbols().generator();

	assert(generator);

	Cursor::ExecutionMode mode = cursor->executionMode();
	SharedReference defaultResult = SharedReference::unsafe(&cursor->symbols().defaultResult());
	SharedReference item = cursor->stack().back();

	cursor->stack().pop_back();
	iterator_insert(generator, SharedReference::unique(new Reference(*item)));

	switch (mode) {
	case Cursor::single_pass:
		break;

	case Cursor::interruptible:
		cursor->stack().emplace_back(defaultResult);
		break;
	}
}

void mint::load_current_result(Cursor *cursor) {

	if (Iterator *generator = cursor->symbols().generator()) {

		SharedReference item = cursor->stack().back();
		cursor->setExecutionMode(Cursor::single_pass);
		cursor->stack().pop_back();

		iterator_insert(generator, item);
		load_default_result(cursor);
	}
}

void mint::load_default_result(Cursor *cursor) {
	cursor->stack().emplace_back(SharedReference::unique(new Reference(cursor->symbols().defaultResult())));
}

SharedReference mint::get_symbol_reference(SymbolTable *symbols, const string &symbol) {

	if (Class *desc = symbols->getPackage()->getClass(symbol)) {
		return SharedReference::unique(new Reference(Reference::standard, desc->makeInstance()));
	}

	auto it_local = symbols->getPackage()->symbols().find(symbol);
	if (it_local != symbols->getPackage()->symbols().end()) {
		return SharedReference::linked(symbols->getPackage()->symbols().referenceManager(), &it_local->second);
	}

	if (Class *desc = GlobalData::instance().getClass(symbol)) {
		return SharedReference::unique(new Reference(Reference::standard, desc->makeInstance()));
	}

	auto it_global = GlobalData::instance().symbols().find(symbol);
	if (it_global != GlobalData::instance().symbols().end()) {
		return SharedReference::linked(GlobalData::instance().symbols().referenceManager(), &it_global->second);
	}

	return SharedReference::linked(symbols->referenceManager(), &(*symbols)[symbol]);
}

SharedReference mint::get_object_member(Cursor *cursor, const Reference &reference, const string &member, Class::MemberInfo *infos) {

	switch (reference.data()->format) {
	case Data::fmt_package:
		if (PackageData *package = reference.data<Package>()->data) {

			if (Class *desc = package->getClass(member)) {

				if (infos) {
					infos->offset = Class::MemberInfo::InvalidOffset;
					infos->owner = desc;
				}

				return SharedReference::unique(new Reference(Reference::global, desc->makeInstance()));
			}

			auto it_package = package->symbols().find(member);
			if (it_package == package->symbols().end()) {
				error("package '%s' has no member '%s'", package->name().c_str(), member.c_str());
			}

			if (infos) {
				infos->offset = Class::MemberInfo::InvalidOffset;
				infos->owner = nullptr;
			}

			return SharedReference::linked(package->symbols().referenceManager(), &it_package->second);
		}

		break;

	case Data::fmt_object:
		if (Object *object = reference.data<Object>()) {

			if (Class::TypeInfo *type = object->metadata->globals().getClass(member)) {
				if (type->flags & Reference::protected_visibility) {
					if (!type->owner->isBaseOrSame(cursor->symbols().getMetadata()) && !type->description->isBaseOrSame(cursor->symbols().getMetadata())) {
						error("could not access protected member type '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
					}
				}
				else if (type->flags & Reference::private_visibility) {
					if (type->owner != cursor->symbols().getMetadata() && type->description != cursor->symbols().getMetadata()) {
						error("could not access private member type '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
					}
				}
				else if (type->flags & Reference::package_visibility) {
					if (type->owner->getPackage() != cursor->symbols().getPackage() && type->description->getPackage() != cursor->symbols().getPackage()) {
						error("could not access package member type '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
					}
				}

				if (infos) {
					infos->offset = Class::MemberInfo::InvalidOffset;
					infos->owner = type->owner;
				}

				return SharedReference::unique(new Reference(Reference::global, type->description->makeInstance()));
			}

			auto it_global = object->metadata->globals().members().find(member);
			if (it_global != object->metadata->globals().members().end()) {

				SharedReference result = SharedReference::unsafe(&it_global->second->value);

				if (result->data()->format != Data::fmt_none) {
					if (result->flags() & Reference::protected_visibility) {
						if (!it_global->second->owner->isBaseOrSame(cursor->symbols().getMetadata())) {
							error("could not access protected member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
						}
					}
					else if (result->flags() & Reference::private_visibility) {
						if (it_global->second->owner != cursor->symbols().getMetadata()) {
							error("could not access private member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
						}
					}
					else if (result->flags() & Reference::package_visibility) {
						if (it_global->second->owner->getPackage() != cursor->symbols().getPackage()) {
							error("could not access package member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
						}
					}
				}

				if (infos) {
					*infos = *it_global->second;
				}

				return result;
			}

			if (is_class(object)) {

				auto it_member = object->metadata->members().find(member);
				if (it_member != object->metadata->members().end()) {

					if (cursor->symbols().getMetadata() == nullptr) {
						error("could not access member '%s' of class '%s' without object", member.c_str(), object->metadata->name().c_str());
					}
					if (cursor->symbols().getMetadata()->bases().find(object->metadata) == cursor->symbols().getMetadata()->bases().end()) {
						error("class '%s' is not a direct base of '%s'", object->metadata->name().c_str(), cursor->symbols().getMetadata()->name().c_str());
					}
					if (it_member->second->value.flags() & Reference::private_visibility) {
						if (it_member->second->owner != cursor->symbols().getMetadata()) {
							error("could not access private member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
						}
					}

					if (infos) {
						*infos = *it_member->second;
					}

					SharedReference result = SharedReference::unique(new Reference(Reference::const_address | Reference::const_value | Reference::global));
					result->copy(it_member->second->value);
					return result;
				}

				error("class '%s' has no global member '%s'", object->metadata->name().c_str(), member.c_str());
			}

			auto it_member = object->metadata->members().find(member);
			if (it_member == object->metadata->members().end()) {
				error("class '%s' has no member '%s'", object->metadata->name().c_str(), member.c_str());
			}

			SharedReference result = SharedReference::linked(object->referenceManager(), object->data + it_member->second->offset);

			if (result->flags() & Reference::protected_visibility) {
				if (!it_member->second->owner->isBaseOrSame(cursor->symbols().getMetadata())) {
					error("could not access protected member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
				}
			}
			else if (result->flags() & Reference::private_visibility) {
				if (it_member->second->owner != cursor->symbols().getMetadata()) {
					error("could not access private member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
				}
			}
			else if (result->flags() & Reference::package_visibility) {
				if (it_member->second->owner->getPackage() != cursor->symbols().getPackage()) {
					error("could not access package member '%s' of class '%s'", member.c_str(), object->metadata->name().c_str());
				}
			}

			if (infos) {
				*infos = *it_member->second;
			}

			return result;
		}

		break;

	default:
		error("non class values dosen't have member '%s'", member.c_str());
	}

	return nullptr;
}

void mint::reduce_member(Cursor *cursor, SharedReference member) {
	cursor->stack().back() = member;
}

Class::MemberInfo *mint::get_member_infos(Object *object, const SharedReference &member) {

	for (auto infos : object->metadata->members()) {
		if (member->data() == infos.second->value.data()) {
			return infos.second;
		}
		if (member->data() == object->data[infos.second->offset].data()) {
			return infos.second;
		}
	}

	return nullptr;
}

string mint::var_symbol(Cursor *cursor) {

	SharedReference var = cursor->stack().back();
	cursor->stack().pop_back();
	return to_string(var);
}

void mint::create_symbol(Cursor *cursor, const string &symbol, Reference::Flags flags) {

	if (flags & Reference::global) {

		PackageData *package = cursor->symbols().getPackage();

		auto it = package->symbols().find(symbol);
		if (it != package->symbols().end()) {
			if (it->second.data()->format != Data::fmt_none) {
				error("symbol '%s' was already defined in global context", symbol.c_str());
			}
			package->symbols().erase(it);
		}

		cursor->stack().emplace_back(SharedReference::linked(package->symbols().referenceManager(), &package->symbols().emplace(symbol, Reference(flags)).first->second));
	}
	else {

		auto it = cursor->symbols().find(symbol);
		if (it != cursor->symbols().end()) {
			if (it->second.data()->format != Data::fmt_none) {
				error("symbol '%s' was already defined in this context", symbol.c_str());
			}
			cursor->symbols().erase(it);
		}

		cursor->stack().emplace_back(SharedReference::linked(cursor->symbols().referenceManager(), &cursor->symbols().emplace(symbol, Reference(flags)).first->second));
	}
}

void mint::array_append_from_stack(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &value = cursor->stack().at(base);
	SharedReference &array = cursor->stack().at(base - 1);

	array_append(array->data<Array>(), value);
	cursor->stack().pop_back();
}

void mint::array_append(Array *array, const SharedReference &item) {
	array->values.push_back(array_item(item));
}

SharedReference mint::array_get_item(Array *array, long index) {
	return SharedReference::linked(array->referenceManager(), array->values[array_index(array, index)].get());
}

size_t mint::array_index(const Array *array, long index) {

	size_t i = (index < 0) ? static_cast<size_t>(index) + array->values.size() : static_cast<size_t>(index);

	if (i >= array->values.size()) {
		error("array index '%ld' is out of range", index);
	}

	return i;
}

SharedReference mint::array_item(const SharedReference &item) {

	Reference *item_value = new Reference();

	if (item->flags() & Reference::const_value) {
		item_value->copy(*item);
	}
	else {
		item_value->move(*item);
	}

	return SharedReference::unique(item_value);
}

void mint::hash_insert_from_stack(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &value = cursor->stack().at(base);
	SharedReference &key = cursor->stack().at(base - 1);
	SharedReference &hash = cursor->stack().at(base - 2);

	hash_insert(hash->data<Hash>(), key, value);
	cursor->stack().pop_back();
	cursor->stack().pop_back();
}

Hash::values_type::iterator mint::hash_insert(Hash *hash, const Hash::key_type &key, const SharedReference &value) {

	Reference *item_key = new Reference(Reference::const_address | Reference::const_value);
	Reference *item_value = new Reference();

	item_key->copy(*key);

	if (value->flags() & Reference::const_value) {
		item_value->copy(*value);
	}
	else {
		item_value->move(*value);
	}

	return hash->values.emplace(SharedReference::unique(item_key), SharedReference::unique(item_value)).first;
}

SharedReference mint::hash_get_item(Hash *hash, const Hash::key_type &key) {

	auto i = hash->values.find(key);

	if (i == hash->values.end()) {
		i = hash_insert(hash, key, SharedReference::unique(Reference::create<None>()));
	}

	return SharedReference::linked(hash->referenceManager(), i->second.get());
}

Hash::key_type mint::hash_get_key(Hash *hash, const Hash::values_type::value_type &item) {
	return SharedReference::linked(hash->referenceManager(), item.first.get());
}

SharedReference mint::hash_get_value(Hash *hash, const Hash::values_type::value_type &item) {
	return SharedReference::linked(hash->referenceManager(), item.second.get());
}

void mint::iterator_init_from_stack(Cursor *cursor, size_t length) {

	Reference *it = new Reference(Reference::const_address, Reference::alloc<Iterator>());
	it->data<Iterator>()->construct();

	for (size_t i = 0; i < length; ++i) {
		iterator_add(it->data<Iterator>(), cursor->stack().back());
		cursor->stack().pop_back();
	}

	cursor->stack().emplace_back(SharedReference::unique(it));
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
				iterator_insert(iterator, SharedReference::linked(ref->data<Array>()->referenceManager(), item.get()));
			}
			return iterator;
		case Class::hash:
			iterator = Reference::alloc<Iterator>();
			iterator->construct();
			for (auto &item : ref->data<Hash>()->values) {
				iterator_insert(iterator, hash_get_key(ref->data<Hash>(), item));
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
		iterator_insert(iterator, ref);
		break;
	}

	return iterator;
}

Iterator *mint::iterator_init(SharedReference &&ref) {
	return iterator_init(static_cast<SharedReference &>(ref));
}

void mint::iterator_insert(Iterator *iterator, SharedReference &item) {
	iterator->ctx.emplace_back(item);
}

void mint::iterator_insert(Iterator *iterator, SharedReference &&item) {
	iterator->ctx.emplace_back(item);
}

void mint::iterator_add(Iterator *iterator, SharedReference &item) {
	iterator->ctx.emplace_front(item);
}

SharedReference mint::iterator_get(Iterator *iterator) {

	if (iterator->ctx.empty()) {
		return nullptr;
	}

	return SharedReference::linked(iterator->referenceManager(), iterator->ctx.front().get());
}

SharedReference mint::iterator_next(Iterator *iterator) {

	if (iterator->ctx.empty()) {
		return nullptr;
	}

	SharedReference item = iterator->ctx.front();
	iterator->ctx.pop_front();
	return item;
}

void mint::iterator_finalize(SharedReference &ref) {
	if (ref->data()->format == Data::fmt_object) {
		if (ref->data<Object>()->metadata->metatype() == Class::iterator) {
			ref->data<Iterator>()->ctx.finalize();
		}
	}
}
