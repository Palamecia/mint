#include "memory/functiontool.h"
#include "memory/globaldata.h"
#include "memory/casttool.h"
#include "memory/builtin/regex.h"
#include "scheduler/scheduler.h"
#include "scheduler/processor.h"
#include "scheduler/process.h"
#include "ast/cursor.h"

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_lang_get_object_locals, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference object = move(helper.popParameter());
	SharedReference result = create_hash();

	switch (object->data()->format) {
	case Data::fmt_object:
		if (Object *data = object->data<Object>()) {
			for (auto &symbol : data->metadata->members()) {
				if (!(symbol.second->value.flags() & (Reference::private_visibility | Reference::protected_visibility | Reference::package_visibility))) {
					hash_insert(result->data<Hash>(), create_string(symbol.first.str()), SharedReference::weak(symbol.second->value));
				}
			}
		}
		break;

	default:
		break;
	}

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_lang_get_locals, 0, cursor) {

	cursor->exitCall();
	cursor->exitCall();

	SharedReference result = create_hash();

	for (auto &symbol : cursor->symbols()) {
		hash_insert(result->data<Hash>(), create_string(symbol.first.str()), SharedReference::weak(symbol.second));
	}

	cursor->stack().emplace_back(move(result));
}

MINT_FUNCTION(mint_lang_get_object_globals, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference object = move(helper.popParameter());
	SharedReference result = create_hash();

	switch (object->data()->format) {
	case Data::fmt_object:
		if (Object *data = object->data<Object>()) {
			for (auto &symbol : data->metadata->globals().members()) {
				if (!(symbol.second->value.flags() & (Reference::private_visibility | Reference::protected_visibility | Reference::package_visibility))) {
					hash_insert(result->data<Hash>(), create_string(symbol.first.str()), SharedReference::weak(symbol.second->value));
				}
			}
		}
		break;

	case Data::fmt_package:
		if (PackageData *data = object->data<Package>()->data) {
			for (auto &symbol : data->symbols()) {
				hash_insert(result->data<Hash>(), create_string(symbol.first.str()), SharedReference::weak(symbol.second));
			}
		}
		break;

	default:
		break;
	}

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_lang_get_globals, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	SharedReference result = create_hash();

	for (auto &symbol : GlobalData::instance().symbols()) {
		hash_insert(result->data<Hash>(), create_string(symbol.first.str()), SharedReference::weak(symbol.second));
	}

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_lang_get_object_types, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference object = move(helper.popParameter());
	SharedReference result = create_hash();

	switch (object->data()->format) {
	case Data::fmt_object:
		if (Object *data = object->data<Object>()) {
			for (int i = 0; ClassDescription *description = data->metadata->globals().getClassDescription(i); ++i) {
				if (Class::TypeInfo *type = data->metadata->globals().getClass(Symbol(description->name()))) {
					if (!(type->flags & (Reference::private_visibility | Reference::protected_visibility | Reference::package_visibility))) {
						hash_insert(result->data<Hash>(),
									create_string(description->name().str()),
									SharedReference::strong(type->description->makeInstance()));
					}
				}
			}
		}
		break;

	case Data::fmt_package:
		if (PackageData *data = object->data<Package>()->data) {
			for (int i = 0; ClassDescription *description = data->getClassDescription(i); ++i) {
				if (Class *type = data->getClass(Symbol(description->name()))) {
					hash_insert(result->data<Hash>(),
								create_string(description->name().str()),
								SharedReference::strong(type->makeInstance()));
				}
			}
		}
		break;

	default:
		break;
	}

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_lang_get_types, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	SharedReference result = create_hash();

	for (int i = 0; ClassDescription *description = GlobalData::instance().getClassDescription(i); ++i) {
		if (Class *type = GlobalData::instance().getClass(Symbol(description->name()))) {
			hash_insert(result->data<Hash>(),
						create_string(description->name().str()),
						SharedReference::strong(type->makeInstance()));
		}
	}

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_lang_is_main, 0, cursor) {

	cursor->exitCall();
	cursor->exitCall();

	bool has_va_args = cursor->symbols().find("va_args") != cursor->symbols().end();
	bool is_first_module = !cursor->callInProgress();

	cursor->stack().emplace_back(create_boolean(has_va_args && is_first_module));
}

MINT_FUNCTION(mint_lang_exec, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference context = move(helper.popParameter());
	SharedReference src = move(helper.popParameter());

	if (Process *process = Process::fromBuffer(to_string(src) + "\n")) {

		for (auto &symbol : to_hash(cursor, context)) {
			process->cursor()->symbols().emplace(Symbol(to_string(symbol.first)), symbol.second);
		}

		unlock_processor();
		process->setup();

		do {
			process->exec();
		}
		while (process->cursor()->callInProgress());

		process->cleanup();
		delete process;
		lock_processor();
	}
}

class EvalResultPrinter : public Printer {
public:
	bool print(DataType type, void *data) override {
		switch (type) {
		case none:
			return true;

		case null:
			m_results.emplace_back(SharedReference::strong(static_cast<Null *>(data)));
			return true;

		case object:
			m_results.emplace_back(SharedReference::strong(static_cast<Object *>(data)));
			return true;

		case package:
			m_results.emplace_back(SharedReference::strong(static_cast<Package *>(data)));
			return true;

		case function:
			m_results.emplace_back(SharedReference::strong(static_cast<Function *>(data)));
			return true;

		case regex:
			m_results.emplace_back(SharedReference::strong(static_cast<Regex *>(data)));
			return true;

		case array:
			m_results.emplace_back(SharedReference::strong(static_cast<Array *>(data)));
			return true;

		case hash:
			m_results.emplace_back(SharedReference::strong(static_cast<Hash *>(data)));
			return true;

		case iterator:
			m_results.emplace_back(SharedReference::strong(static_cast<Iterator *>(data)));
			return true;
		}

		return false;
	}

	void print(const char *value) override {
		m_results.emplace_back(create_string(value));
	}

	void print(double value) override {
		m_results.emplace_back(create_number(value));
	}

	void print(bool value) override {
		m_results.emplace_back(create_boolean(value));
	}

	SharedReference result() {
		switch (m_results.size()) {
		case 0:
			return SharedReference::strong<None>();

		case 1:
			return move(m_results.front());

		default:
			break;
		}

		SharedReference reference = create_iterator();

		for (SharedReference &item : m_results) {
			iterator_insert(reference->data<Iterator>(), move(item));
		}

		return reference;
	}

private:
	std::vector<SharedReference> m_results;
};

MINT_FUNCTION(mint_lang_eval, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference context = move(helper.popParameter());
	SharedReference src = move(helper.popParameter());

	if (Process *process = Process::fromBuffer(to_string(src) + "\n")) {

		for (auto &symbol : to_hash(cursor, context)) {
			process->cursor()->symbols().emplace(Symbol(to_string(symbol.first)), symbol.second);
		}

		EvalResultPrinter *printer = new EvalResultPrinter;
		process->cursor()->openPrinter(printer);
		unlock_processor();
		process->setup();

		do {
			process->exec();
		}
		while (process->cursor()->callInProgress());

		helper.returnValue(printer->result());
		process->cursor()->closePrinter();
		process->cleanup();
		delete process;
		lock_processor();
	}
}

MINT_FUNCTION(mint_lang_create_object_global, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	SharedReference value = move(helper.popParameter());
	SharedReference name = move(helper.popParameter());
	SharedReference object = move(helper.popParameter());

	Symbol symbol(to_string(name));

	switch (object->data()->format) {
	case Data::fmt_object:
		if (Object *data = object->data<Object>()) {
			if (data->metadata->globals().members().find(symbol) == data->metadata->globals().members().end()) {
				Class::MemberInfo *member = new Class::MemberInfo;
				member->owner = data->metadata;
				member->offset = Class::MemberInfo::InvalidOffset;
				member->value = StrongReference(Reference::global | value->flags(), value->data());
				data->metadata->globals().members().emplace(symbol, member);
				helper.returnValue(create_boolean(true));
			}
			else{
				helper.returnValue(create_boolean(false));
			}
		}
		else{
			helper.returnValue(create_boolean(false));
		}
		break;

	case Data::fmt_package:
		if (PackageData *data = object->data<Package>()->data) {
			if (data->symbols().find(symbol) == data->symbols().end()) {
				data->symbols().emplace(symbol, StrongReference(Reference::global | value->flags(), value->data()));
				helper.returnValue(create_boolean(true));
			}
			else{
				helper.returnValue(create_boolean(false));
			}
		}
		else{
			helper.returnValue(create_boolean(false));
		}
		break;

	default:
		helper.returnValue(create_boolean(false));
		break;
	}
}

MINT_FUNCTION(mint_lang_create_global, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference value = move(helper.popParameter());
	SharedReference name = move(helper.popParameter());

	SymbolTable *symbols = &GlobalData::instance().symbols();
	Symbol symbol(to_string(name));

	if (symbols->find(symbol) == symbols->end()) {
		symbols->emplace(symbol, StrongReference(Reference::global | value->flags(), value->data()));
		helper.returnValue(create_boolean(true));
	}
	else{
		helper.returnValue(create_boolean(false));
	}
}
