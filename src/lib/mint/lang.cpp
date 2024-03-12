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

#include "mint/memory/functiontool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/casttool.h"
#include "mint/scheduler/scheduler.h"
#include "mint/scheduler/processor.h"
#include "mint/scheduler/process.h"
#include "mint/system/filesystem.h"
#include "mint/system/error.h"
#include "mint/debug/debugtool.h"
#include "mint/ast/cursor.h"

#include "evalresultprinter.h"

using namespace mint;
using namespace std;

static string to_module_path(const string &root_path, const string &file_path) {
	string module_path = FileSystem::instance().relative_path(root_path, file_path);
	module_path = module_path.substr(0, module_path.find('.'));
	for_each(module_path.begin(), module_path.end(), [](char &ch) {
		if (ch == FileSystem::separator) {
			ch = '.';
		}
	});
	return module_path;
}

static string to_system_path(const string &root_path, const string &module_path) {
	string file_path = module_path;
	for_each(file_path.begin(), file_path.end(), [](char &ch) {
		if (ch == '.') {
			ch = FileSystem::separator;
		}
	});
	return root_path + FileSystem::separator + file_path;
}

MINT_FUNCTION(mint_lang_modules_roots, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	WeakReference result = create_array();

	for (const string &path : FileSystem::instance().library_path()) {
		array_append(result.data<Array>(), create_string(path));
	}
	
	helper.return_value(std::move(result));
}

static void find_module_recursive_helper(Array *result, const string root_path, const string directory_path) {
	FileSystem &fs = FileSystem::instance();
	for (auto it = fs.browse(directory_path); it != fs.end(); ++it) {
		const string file_name = *it;
		if (file_name == "." || file_name == "..") {
			continue;
		}
		const string file_path = directory_path + FileSystem::separator + file_name;
		if (fs.is_directory(file_path)) {
			find_module_recursive_helper(result, root_path, file_path);
		}
		else if (is_module_file(file_path)) {
			array_append(result, create_string(to_module_path(root_path, file_path)));
		}
	}
}

MINT_FUNCTION(mint_lang_modules_list, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const string module_path = to_string(helper.pop_parameter());
	WeakReference result = create_array();

	for (const string &path : FileSystem::instance().library_path()) {
		const string root_path = FileSystem::instance().absolute_path(path);
		if (module_path.empty()) {
			find_module_recursive_helper(result.data<Array>(), root_path, root_path);
		}
		else {
			const string file_path = to_system_path(root_path, module_path);
			if (FileSystem::instance().check_file_access(file_path + ".mn", FileSystem::exists)) {
				array_append(result.data<Array>(), create_string(module_path));
			}
			else {
				find_module_recursive_helper(result.data<Array>(), root_path, file_path);
			}
		}
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_lang_main_module_path, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.return_value(create_string(get_main_module_path()));
}

MINT_FUNCTION(mint_lang_to_module_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const string file_path = FileSystem::instance().absolute_path(to_string(helper.pop_parameter()));

	if (is_module_file(file_path)) {
		for (const string &path : FileSystem::instance().library_path()) {
			string root_path = FileSystem::instance().absolute_path(path);
			auto pos = file_path.find(FileSystem::separator, root_path.size());
			if (pos != string::npos && root_path == file_path.substr(0, pos)) {
				helper.return_value(create_string(to_module_path(root_path, file_path)));
				return;
			}
		}
	}
}

MINT_FUNCTION(mint_lang_to_file_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const string module_path = to_string(helper.pop_parameter());
	const string file_path = FileSystem::instance().absolute_path(to_system_path(module_path));

	if (FileSystem::instance().check_file_access(file_path, FileSystem::exists)) {
		helper.return_value(create_string(file_path));
	}
}

MINT_FUNCTION(mint_lang_load_module, 1, cursor) {
	auto &stack = cursor->stack();
	Reference &module_path = stack.back();
	stack.back() = create_boolean(cursor->load_module(to_string(module_path)));
}

MINT_FUNCTION(mint_lang_backtrace, 1, cursor) {

	Reference &thread_id = cursor->stack().back();
	WeakReference result = create_array();

	cursor->exit_call();
	cursor->exit_call();

	if (is_instance_of(thread_id, Data::fmt_none)) {
		for (const LineInfo &info : cursor->dump()) {
			array_append(result.data<Array>(), array_item(create_iterator(create_string(info.module_name()),
																		  create_number(info.line_number()))));
		}
	}
	else if (Scheduler *scheduler = Scheduler::instance()) {
		if (Process *thread = scheduler->find_thread(to_integer(cursor, thread_id))) {
			for (const LineInfo &info : thread->cursor()->dump()) {
				array_append(result.data<Array>(), array_item(create_iterator(create_string(info.module_name()),
																			  create_number(info.line_number()))));
			}
		}
	}

	cursor->stack().back() = std::move(result);
}

MINT_FUNCTION(mint_lang_get_object_locals, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &object = helper.pop_parameter();
	WeakReference result = create_hash();

	switch (object.data()->format) {
	case Data::fmt_object:
		if (Object *data = object.data<Object>()) {
			for (auto &symbol : data->metadata->members()) {
				if (!(symbol.second->value.flags() & Reference::visibility_mask)) {
					hash_insert(result.data<Hash>(), create_string(symbol.first.str()), symbol.second->value);
				}
			}
		}
		break;

	default:
		break;
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_lang_get_locals, 0, cursor) {
	
	cursor->exit_call();
	cursor->exit_call();

	WeakReference result = create_hash();

	for (auto &symbol : cursor->symbols()) {
		hash_insert(result.data<Hash>(), create_string(symbol.first.str()), symbol.second);
	}

	cursor->stack().emplace_back(std::move(result));
}

MINT_FUNCTION(mint_lang_get_object_globals, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &object = helper.pop_parameter();
	WeakReference result = create_hash();

	switch (object.data()->format) {
	case Data::fmt_object:
		if (Object *data = object.data<Object>()) {
			for (auto &symbol : data->metadata->globals()) {
				if (!(symbol.second->value.flags() & Reference::visibility_mask)) {
					hash_insert(result.data<Hash>(), create_string(symbol.first.str()), symbol.second->value);
				}
			}
		}
		break;

	case Data::fmt_package:
		if (PackageData *data = object.data<Package>()->data) {
			for (auto &symbol : data->symbols()) {
				hash_insert(result.data<Hash>(), create_string(symbol.first.str()), symbol.second);
			}
		}
		break;

	default:
		break;
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_lang_get_globals, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	WeakReference result = create_hash();

	for (auto &symbol : GlobalData::instance()->symbols()) {
		hash_insert(result.data<Hash>(), create_string(symbol.first.str()), symbol.second);
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_lang_get_object_types, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &object = helper.pop_parameter();
	WeakReference result = create_hash();

	switch (object.data()->format) {
	case Data::fmt_object:
		if (Object *data = object.data<Object>()) {
			if (ClassDescription *description = data->metadata->get_description()) {
				for (ClassDescription::Id i = 0; ClassDescription *child = description->get_class_description(i); ++i) {
					if (Class::MemberInfo *type = data->metadata->get_class(child->name())) {
						if (!(type->value.flags() & Reference::visibility_mask)) {
							hash_insert(result.data<Hash>(), create_string(child->name().str()), WeakReference::create(type->value.data<Object>()->metadata->make_instance()));
						}
					}
				}
			}
		}
		break;

	case Data::fmt_package:
		if (PackageData *data = object.data<Package>()->data) {
			for (ClassDescription::Id i = 0; ClassDescription *description = data->get_class_description(i); ++i) {
				if (Class *type = data->get_class(description->name())) {
					hash_insert(result.data<Hash>(), create_string(description->name().str()), WeakReference::create(type->make_instance()));
				}
			}
		}
		break;

	default:
		break;
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_lang_get_types, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	WeakReference result = create_hash();

	for (ClassRegister::Id i = 0; ClassDescription *description = GlobalData::instance()->get_class_description(i); ++i) {
		if (Class *type = GlobalData::instance()->get_class(Symbol(description->name()))) {
			hash_insert(result.data<Hash>(), create_string(description->name().str()), WeakReference::create(type->make_instance()));
		}
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_lang_is_main, 0, cursor) {
	
	cursor->exit_call();
	cursor->exit_call();

	bool has_va_args = cursor->symbols().find("va_args") != cursor->symbols().end();
	bool is_first_module = !cursor->call_in_progress();

	cursor->stack().emplace_back(create_boolean(has_va_args && is_first_module));
}

MINT_FUNCTION(mint_at_exit, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &callback = helper.pop_parameter();

	struct callback_t {
		callback_t(WeakReference &&function) :
			function(std::make_shared<StrongReference>(std::move(function))) {

		}
		void operator ()(int status) {
			if (Scheduler* scheduler = Scheduler::instance()) {
				scheduler->invoke(*function, create_number(status));
			}
		}
	private:
		std::shared_ptr<StrongReference> function;
	};

	if (Scheduler* scheduler = Scheduler::instance()) {
		scheduler->add_exit_callback(callback_t { std::move(callback) });
	}
}

MINT_FUNCTION(mint_at_error, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &callback = helper.pop_parameter();

	struct callback_t {
		callback_t(WeakReference &&function) :
			function(std::make_shared<StrongReference>(std::move(function))) {

		}
		void operator ()() {
			if (Scheduler *scheduler = Scheduler::instance()) {
				WeakReference backtrace = create_array();
				if (Process *process = scheduler->current_process()) {
					for (const LineInfo &info : process->cursor()->dump()) {
						array_append(backtrace.data<Array>(), array_item(create_iterator(create_string(info.module_name()),
																						 create_number(info.line_number()))));
					}
				}
				scheduler->invoke(*function, create_string(get_error_message()), std::move(backtrace));
			}
		}
	private:
		std::shared_ptr<StrongReference> function;
	};

	add_error_callback(callback_t { std::move(callback) });
}

MINT_FUNCTION(mint_lang_exec, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &context = helper.pop_parameter();
	Reference &src = helper.pop_parameter();
	
	if (Process *process = Process::from_buffer(cursor->ast(), to_string(src) + "\n")) {

		for (auto &symbol : to_hash(cursor, context)) {
			process->cursor()->symbols().emplace(Symbol(to_string(symbol.first)), symbol.second);
		}

		unlock_processor();
		process->setup();

		do {
			process->exec();
		}
		while (process->cursor()->call_in_progress());

		process->cleanup();
		delete process;
		lock_processor();
	}
}

MINT_FUNCTION(mint_lang_eval, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &context = helper.pop_parameter();
	Reference &src = helper.pop_parameter();
	
	if (Process *process = Process::from_buffer(cursor->ast(), to_string(src) + "\n")) {

		for (auto &symbol : to_hash(cursor, context)) {
			process->cursor()->symbols().emplace(Symbol(to_string(symbol.first)), symbol.second);
		}

		EvalResultPrinter printer;
		process->cursor()->open_printer(&printer);
		unlock_processor();
		process->setup();

		do {
			process->exec();
		}
		while (process->cursor()->call_in_progress());
		
		helper.return_value(printer.result());
		process->cleanup();
		delete process;
		lock_processor();
	}
}

MINT_FUNCTION(mint_lang_create_object_global, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &name = helper.pop_parameter();
	Reference &object = helper.pop_parameter();

	Symbol symbol(to_string(name));

	switch (object.data()->format) {
	case Data::fmt_object:
		if (Object *data = object.data<Object>()) {
			if (data->metadata->globals().find(symbol) == data->metadata->globals().end()) {
				Class::MemberInfo *member = new Class::MemberInfo;
				member->owner = data->metadata;
				member->offset = Class::MemberInfo::invalid_offset;
				member->value = WeakReference(Reference::global | value.flags(), value.data());
				data->metadata->globals().emplace(symbol, member);
				helper.return_value(create_boolean(true));
			}
			else{
				helper.return_value(create_boolean(false));
			}
		}
		else{
			helper.return_value(create_boolean(false));
		}
		break;

	case Data::fmt_package:
		if (PackageData *data = object.data<Package>()->data) {
			if (data->symbols().find(symbol) == data->symbols().end()) {
				data->symbols().emplace(symbol, WeakReference(Reference::global | value.flags(), value.data()));
				helper.return_value(create_boolean(true));
			}
			else{
				helper.return_value(create_boolean(false));
			}
		}
		else{
			helper.return_value(create_boolean(false));
		}
		break;

	default:
		helper.return_value(create_boolean(false));
		break;
	}
}

MINT_FUNCTION(mint_lang_create_global, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &value = helper.pop_parameter();
	Reference &name = helper.pop_parameter();

	SymbolTable *symbols = &GlobalData::instance()->symbols();
	Symbol symbol(to_string(name));

	if (symbols->find(symbol) == symbols->end()) {
		symbols->emplace(symbol, WeakReference(Reference::global | value.flags(), value.data()));
		helper.return_value(create_boolean(true));
	}
	else{
		helper.return_value(create_boolean(false));
	}
}
