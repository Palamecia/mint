#include "memory/functiontool.h"
#include "memory/globaldata.h"
#include "memory/casttool.h"
#include "memory/builtin/regex.h"
#include "scheduler/scheduler.h"
#include "scheduler/processor.h"
#include "scheduler/process.h"
#include "ast/cursor.h"

using namespace mint;

MINT_FUNCTION(mint_lang_get_object_locals, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference object = helper.popParameter();
	SharedReference result = SharedReference::unique(Reference::create<Hash>());

	switch (object->data()->format) {
	case Data::fmt_object:
		if (Object *data = object->data<Object>()) {
			for (auto &symbol : data->metadata->members()) {
				if (!(symbol.second->value.flags() & (Reference::private_visibility | Reference::protected_visibility | Reference::package_visibility))) {
					hash_insert(result->data<Hash>(), create_string(symbol.first), SharedReference::linked(data->referenceManager(), &symbol.second->value));
				}
			}
		}
		break;

	default:
		break;
	}

	helper.returnValue(result);
}

MINT_FUNCTION(mint_lang_get_locals, 0, cursor) {

	cursor->exitCall();
	ReferenceManager *manager = cursor->symbols().referenceManager();
	SharedReference result = SharedReference::unique(Reference::create<Hash>());

	for (auto &symbol : cursor->symbols()) {
		hash_insert(result->data<Hash>(),
					create_string(symbol.first),
					SharedReference::linked(manager, &symbol.second));
	}

	cursor->stack().emplace_back(result);
}

MINT_FUNCTION(mint_lang_get_object_globals, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference object = helper.popParameter();
	SharedReference result = SharedReference::unique(Reference::create<Hash>());

	switch (object->data()->format) {
	case Data::fmt_object:
		if (Object *data = object->data<Object>()) {
			for (auto &symbol : data->metadata->globals().members()) {
				if (!(symbol.second->value.flags() & (Reference::private_visibility | Reference::protected_visibility | Reference::package_visibility))) {
					hash_insert(result->data<Hash>(), create_string(symbol.first), SharedReference::unsafe(&symbol.second->value));
				}
			}
		}
		break;

	case Data::fmt_package:
		if (PackageData *data = object->data<Package>()->data) {
			for (auto &symbol : data->symbols()) {
				hash_insert(result->data<Hash>(), create_string(symbol.first), SharedReference::linked(data->symbols().referenceManager(), &symbol.second));
			}
		}
		break;

	default:
		break;
	}

	helper.returnValue(result);
}

MINT_FUNCTION(mint_lang_get_globals, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	SharedReference result = SharedReference::unique(Reference::create<Hash>());
	ReferenceManager *manager = GlobalData::instance().symbols().referenceManager();

	for (auto &symbol : GlobalData::instance().symbols()) {
		hash_insert(result->data<Hash>(),
					create_string(symbol.first),
					SharedReference::linked(manager, &symbol.second));
	}

	helper.returnValue(result);
}

MINT_FUNCTION(mint_lang_get_object_types, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference object = helper.popParameter();
	SharedReference result = SharedReference::unique(Reference::create<Hash>());

	switch (object->data()->format) {
	case Data::fmt_object:
		if (Object *data = object->data<Object>()) {
			for (int i = 0; ClassDescription *description = data->metadata->globals().getClassDescription(i); ++i) {
				if (Class::TypeInfo *type = data->metadata->globals().getClass(description->name())) {
					if (!(type->flags & (Reference::private_visibility | Reference::protected_visibility | Reference::package_visibility))) {
						hash_insert(result->data<Hash>(),
									create_string(description->name()),
									SharedReference::unique(Reference::create(type->description->makeInstance())));
					}
				}
			}
		}
		break;

	case Data::fmt_package:
		if (PackageData *data = object->data<Package>()->data) {
			for (int i = 0; ClassDescription *description = data->getClassDescription(i); ++i) {
				if (Class *type = data->getClass(description->name())) {
					hash_insert(result->data<Hash>(),
								create_string(description->name()),
								SharedReference::unique(Reference::create(type->makeInstance())));
				}
			}
		}
		break;

	default:
		break;
	}

	helper.returnValue(result);
}

MINT_FUNCTION(mint_lang_get_types, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	SharedReference result = SharedReference::unique(Reference::create<Hash>());

	for (int i = 0; ClassDescription *description = GlobalData::instance().getClassDescription(i); ++i) {
		if (Class *type = GlobalData::instance().getClass(description->name())) {
			hash_insert(result->data<Hash>(),
						create_string(description->name()),
						SharedReference::unique(Reference::create(type->makeInstance())));
		}
	}

	helper.returnValue(result);
}

MINT_FUNCTION(mint_lang_is_class, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference object = helper.popParameter();

	if (object->data()->format == Data::fmt_object) {
		helper.returnValue(create_boolean(is_class(object->data<Object>())));
	}
	else {
		helper.returnValue(create_boolean(false));
	}
}

MINT_FUNCTION(mint_lang_is_object, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference object = helper.popParameter();

	if (object->data()->format == Data::fmt_object) {
		helper.returnValue(create_boolean(is_object(object->data<Object>())));
	}
	else {
		helper.returnValue(create_boolean(true));
	}
}

MINT_FUNCTION(mint_lang_is_main, 0, cursor) {

	cursor->exitCall();

	bool has_va_args = cursor->symbols().find("va_args") != cursor->symbols().end();
	bool is_first_module = !cursor->callInProgress();

	cursor->stack().emplace_back(create_boolean(has_va_args && is_first_module));
}

MINT_FUNCTION(mint_lang_exec, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference context = helper.popParameter();
	SharedReference src = helper.popParameter();

	if (Process *process = Process::fromBuffer(to_string(src) + "\n")) {

		for (auto &symbol : to_hash(cursor, context)) {
			process->cursor()->symbols().emplace(to_string(symbol.first), *symbol.second);
		}

		unlock_processor();
		process->setup();

		do {
			process->exec(Scheduler::quantum);
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
			m_results.push_back(SharedReference::unique(Reference::create(static_cast<Null *>(data))));
			return true;

		case object:
			m_results.push_back(SharedReference::unique(Reference::create(static_cast<Object *>(data))));
			return true;

		case package:
			m_results.push_back(SharedReference::unique(Reference::create(static_cast<Package *>(data))));
			return true;

		case function:
			m_results.push_back(SharedReference::unique(Reference::create(static_cast<Function *>(data))));
			return true;

		case regex:
			m_results.push_back(SharedReference::unique(Reference::create(static_cast<Regex *>(data))));
			return true;

		case array:
			m_results.push_back(SharedReference::unique(Reference::create(static_cast<Array *>(data))));
			return true;

		case hash:
			m_results.push_back(SharedReference::unique(Reference::create(static_cast<Hash *>(data))));
			return true;

		case iterator:
			m_results.push_back(SharedReference::unique(Reference::create(static_cast<Iterator *>(data))));
			return true;
		}

		return false;
	}

	void print(const char *value) override {
		m_results.push_back(create_string(value));
	}

	void print(double value) override {
		m_results.push_back(create_number(value));
	}

	void print(bool value) override {
		m_results.push_back(create_boolean(value));
	}

	SharedReference result() {
		switch (m_results.size()) {
		case 0:
			return SharedReference::unique(Reference::create<None>());

		case 1:
			return m_results.front();

		default:
			break;
		}

		SharedReference reference = SharedReference::unique(Reference::create<Iterator>());
		reference->data<Iterator>()->construct();

		for (SharedReference item : m_results) {
			iterator_insert(reference->data<Iterator>(), item);
		}

		return reference;
	}

private:
	std::vector<SharedReference> m_results;
};

MINT_FUNCTION(mint_lang_eval, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference context = helper.popParameter();
	SharedReference src = helper.popParameter();

	if (Process *process = Process::fromBuffer(to_string(src) + "\n")) {

		for (auto &symbol : to_hash(cursor, context)) {
			process->cursor()->symbols().emplace(to_string(symbol.first), *symbol.second);
		}

		EvalResultPrinter *printer = new EvalResultPrinter;
		process->cursor()->openPrinter(printer);
		unlock_processor();
		process->setup();

		do {
			process->exec(Scheduler::quantum);
		}
		while (process->cursor()->callInProgress());

		helper.returnValue(printer->result());
		process->cursor()->closePrinter();
		process->cleanup();
		delete process;
		lock_processor();
	}
}
