/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#include "mint/ast/class_register.h"
#include "mint/ast/symbol.h"
#include "mint/debug/line_info.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/data.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/global_data.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"
#include "mint/scheduler/processor.h"
#include "mint/scheduler/process.h"
#include "mint/system/filesystem.h"
#include "mint/system/error.h"
#include "mint/debug/debug_tools.h"
#include "mint/ast/cursor.h"

#include "eval_result_printer.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <utility>

namespace {

std::filesystem::path add_module_extension(std::filesystem::path path) {
	return path.replace_extension(".mn");
}

void find_module_recursive_helper(mint::AbstractSyntaxTree& ast, mint::Array& result,
    const std::filesystem::path& root_path, const std::filesystem::path& directory_path) {
	for (const auto& entry : std::filesystem::directory_iterator {directory_path}) {
		if (entry.is_directory()) {
			find_module_recursive_helper(ast, result, root_path, entry.path());
		}
		else if (mint::is_module_file(entry.path())) {
			array_append(result, mint::create_string(ast, mint::FileSystem::to_module_path(root_path, entry.path())));
		}
	}
}

mint::Reference mint_lang_modules_roots(mint::Cursor& cursor) {
	return mint::create_array(cursor.ast(),
	    {std::from_range, std::views::transform(mint::FileSystem::instance().library_path(),
	                          [&ast = cursor.ast()](const std::filesystem::path& path) {
		                          return mint::create_string(ast, path.generic_string());
	                          })});
}

mint::Reference mint_lang_modules_list(mint::Cursor& cursor, const mint::Reference& module_path) {

	const auto module_path_str = to_string(module_path);
	mint::Reference result = mint::create_array(cursor.ast());

	for (const std::filesystem::path& path : mint::FileSystem::instance().library_path()) {
		if (const auto root_path = std::filesystem::absolute(path); module_path_str.empty()) {
			find_module_recursive_helper(cursor.ast(), result.data<mint::Array>(), root_path, root_path);
		}
		else if (const auto file_path = mint::FileSystem::to_system_path(root_path, module_path_str);
		    std::filesystem::exists(add_module_extension(file_path))) {
			array_append(result.data<mint::Array>(), mint::create_string(cursor.ast(), module_path_str));
		}
		else if (std::filesystem::exists(file_path)) {
			find_module_recursive_helper(cursor.ast(), result.data<mint::Array>(), root_path, file_path);
		}
	}

	return result;
}

mint::Reference mint_lang_main_module_path(mint::Cursor& cursor) {
	return mint::create_string(cursor.ast(), mint::FileSystem::instance().get_main_module_path().generic_string());
}

mint::Reference mint_lang_to_module_path(mint::Cursor& cursor, const mint::Reference& file_path) {

	const auto file_path_str = to_string(file_path);

	if (mint::is_module_file(std::filesystem::absolute(file_path_str))) {
		for (const std::filesystem::path& path : mint::FileSystem::instance().library_path()) {
			if (const auto root_path = std::filesystem::absolute(path);
			    mint::FileSystem::is_subpath(file_path_str, root_path)) {
				return mint::create_string(cursor.ast(), mint::FileSystem::to_module_path(root_path, file_path_str));
			}
		}
	}

	return {};
}

mint::Reference mint_lang_to_file_path(mint::Cursor& cursor, const mint::Reference& module_path) {

	const std::filesystem::path file_path = std::filesystem::absolute(mint::to_system_path(to_string(module_path)));

	if (std::filesystem::exists(file_path)) {
		return mint::create_string(cursor.ast(), file_path.generic_string());
	}

	return {};
}

mint::Reference mint_lang_get_object_locals(mint::Cursor& cursor, const mint::Reference& object) {

	mint::Reference result = mint::create_hash(cursor.ast());

	if (mint::is_instance_of(object, mint::Data::Format::object)) {
		for (const auto& [symbol, member] : object.data<mint::Object>().metadata.members()) {
			if (!(member.get().value.flags() & mint::Reference::visibility_mask)) {
				hash_insert(result.data<mint::Hash>(), mint::create_string(cursor.ast(), symbol.str()),
				    member.get().value);
			}
		}
	}

	return result;
}

mint::Reference mint_lang_get_object_globals(mint::Cursor& cursor, const mint::Reference& object) {

	mint::Reference result = mint::create_hash(cursor.ast());

	switch (object.data().format()) {
	case mint::Data::Format::object:
		for (const auto& [symbol, member] : object.data<mint::Object>().metadata.globals()) {
			if (!(member.get().value.flags() & mint::Reference::visibility_mask)) {
				hash_insert(result.data<mint::Hash>(), mint::create_string(cursor.ast(), symbol.str()),
				    member.get().value);
			}
		}
		break;
	case mint::Data::Format::package:
		for (const auto& [symbol, member] : object.data<mint::Package>().data.symbols()) {
			hash_insert(result.data<mint::Hash>(), mint::create_string(cursor.ast(), symbol.str()), member);
		}
		break;
	default:
		break;
	}

	return result;
}

mint::Reference mint_lang_get_globals(mint::Cursor& cursor) {

	mint::Reference result = mint::create_hash(cursor.ast());

	for (const auto& [symbol, member] : cursor.ast().global_data().symbols()) {
		hash_insert(result.data<mint::Hash>(), mint::create_string(cursor.ast(), symbol.str()), member);
	}

	return result;
}

mint::Reference mint_lang_get_object_types(mint::Cursor& cursor, const mint::Reference& object) {

	mint::Reference result = mint::create_hash(cursor.ast());

	switch (object.data().format()) {
	case mint::Data::Format::object:
		for (const auto& [symbol, type] : object.data<mint::Object>().metadata.classes()) {
			if (!(type.get().value.flags() & mint::Reference::visibility_mask)) {
				hash_insert(result.data<mint::Hash>(), mint::create_string(cursor.ast(), symbol.str()),
				    mint::create_alias(type.get().value.data<mint::Object>().metadata));
			}
		}
		break;
	case mint::Data::Format::package:
		for (const auto& [symbol, type] : object.data<mint::Package>().data.classes()) {
			hash_insert(result.data<mint::Hash>(), mint::create_string(cursor.ast(), symbol.str()),
			    mint::create_alias(type));
		}
		break;
	default:
		break;
	}

	return result;
}

mint::Reference mint_lang_get_types(mint::Cursor& cursor) {

	mint::Reference result = mint::create_hash(cursor.ast());

	for (const auto& [symbol, type] : cursor.ast().global_data().classes()) {
		hash_insert(result.data<mint::Hash>(), mint::create_string(cursor.ast(), symbol.str()),
		    mint::create_alias(type));
	}

	return result;
}

mint::Reference mint_at_exit(mint::FunctionHelper& helper, const mint::Reference& callback) {

	struct Callback {
		Callback(mint::Scheduler& scheduler, const mint::Reference& function) :
		    _scheduler(scheduler),
		    _function(std::make_shared<mint::RootReference>(function)) {}

		void operator()(int status) {
			_scheduler.get().invoke(*_function, mint::create_number(status));
		}

	private:
		std::reference_wrapper<mint::Scheduler> _scheduler;
		std::shared_ptr<mint::RootReference> _function;
	};

	mint::Scheduler& scheduler = helper.scheduler();
	scheduler.add_exit_callback(Callback(scheduler, callback));
	return {};
}

mint::Reference mint_at_error(mint::FunctionHelper& helper, const mint::Reference& callback) {

	struct Callback {
		Callback(mint::Scheduler& scheduler, const mint::Reference& function) :
		    _scheduler(scheduler),
		    _function(std::make_shared<mint::RootReference>(function)) {}

		void operator()(const std::string& message) {
			mint::Reference backtrace = mint::create_array(_scheduler.get().ast());
			if (const mint::Process* process = mint::Scheduler::current_process()) {
				for (const mint::LineInfo& info : process->cursor().dump()) {
					array_append(backtrace.data<mint::Array>(),
					    array_item(create_iterator_from(process->cursor(),
					        mint::create_string(_scheduler.get().ast(), info.module_name()),
					        mint::create_unsigned_number(info.line_number()))));
				}
			}
			_scheduler.get().invoke(*_function, mint::create_string(_scheduler.get().ast(), message),
			    std::move(backtrace));
		}

	private:
		std::reference_wrapper<mint::Scheduler> _scheduler;
		std::shared_ptr<mint::RootReference> _function;
	};

	mint::add_error_callback(Callback(helper.scheduler(), callback));
	return {};
}

mint::Reference mint_lang_exec(mint::FunctionHelper& helper, const mint::Reference& src,
    const mint::Reference& context) {

	if (auto process = mint::Process::from_buffer(helper.scheduler(), to_string(src) + "\n")) {

		for (auto& symbol : to_hash(context)) {
			process->cursor().symbols().emplace(mint::Symbol(to_string(symbol.first)), symbol.second);
		}

		mint::unlock_processor();
		process->setup();

		do {
			switch (process->exec()) {
			case mint::ProcessStatus::paused:
			case mint::ProcessStatus::completed:
			case mint::ProcessStatus::failed:
				// TODO: handle statuses properly
				break;
			}
		}
		while (process->cursor().call_in_progress());

		process->cleanup();
		mint::lock_processor();
	}

	return {};
}

mint::Reference mint_lang_eval(mint::FunctionHelper& helper, const mint::Reference& src,
    const mint::Reference& context) {

	if (auto process = mint::Process::from_buffer(helper.scheduler(), to_string(src) + "\n")) {

		for (auto& symbol : to_hash(context)) {
			process->cursor().symbols().emplace(mint::Symbol(to_string(symbol.first)), symbol.second);
		}

		auto printer = std::make_unique<EvalResultPrinter>(process->cursor());
		auto& printer_ref = *printer;
		process->cursor().open_printer(std::move(printer));
		mint::unlock_processor();
		process->setup();

		do {
			switch (process->exec()) {
			case mint::ProcessStatus::completed:
			case mint::ProcessStatus::paused:
			case mint::ProcessStatus::failed:
				// TODO: handle statuses properly
				break;
			}
		}
		while (process->cursor().call_in_progress());

		auto result = printer_ref.result();
		process->cleanup();
		mint::lock_processor();
		return result;
	}

	return {};
}

}

MINT_EXPORT_FUNCTION(mint_lang_modules_roots, 0);
MINT_EXPORT_FUNCTION(mint_lang_modules_list, 1);
MINT_EXPORT_FUNCTION(mint_lang_main_module_path, 0);
MINT_EXPORT_FUNCTION(mint_lang_to_module_path, 1);
MINT_EXPORT_FUNCTION(mint_lang_to_file_path, 1)

MINT_RAW_FUNCTION(mint_lang_load_module, 1, cursor) {
	auto& stack = cursor.stack();
	const auto& module_path = stack.back();
	stack.back() = mint::create_boolean(cursor.load_module(to_string(module_path)));
}

MINT_RAW_FUNCTION(mint_lang_backtrace, 1, cursor) {

	const auto& thread_id = cursor.stack().back();
	auto result = mint::create_array(cursor.ast());

	cursor.exit_call();
	cursor.exit_call();

	if (is_instance_of(thread_id, mint::Data::Format::none)) {
		for (const mint::LineInfo& info : cursor.dump()) {
			array_append(result.data<mint::Array>(),
			    array_item(create_iterator_from(cursor, mint::create_string(cursor.ast(), info.module_name()),
			        mint::create_unsigned_number(info.line_number()))));
		}
	}
	else if (const auto* scheduler = mint::Scheduler::instance()) {
		if (const auto* thread = scheduler->find_thread(mint::to_integer<mint::Process::ThreadId>(cursor, thread_id))) {
			for (const mint::LineInfo& info : thread->cursor().dump()) {
				array_append(result.data<mint::Array>(),
				    array_item(create_iterator_from(cursor, mint::create_string(cursor.ast(), info.module_name()),
				        mint::create_unsigned_number(info.line_number()))));
			}
		}
	}

	cursor.stack().back() = std::move(result);
}

MINT_EXPORT_FUNCTION(mint_lang_get_object_locals, 1);

MINT_RAW_FUNCTION(mint_lang_get_locals, 0, cursor) {

	cursor.exit_call();
	cursor.exit_call();

	mint::Reference result = mint::create_hash(cursor.ast());

	for (auto& symbol : cursor.symbols()) {
		hash_insert(result.data<mint::Hash>(), mint::create_string(cursor.ast(), symbol.first.str()), symbol.second);
	}

	cursor.stack().emplace_back(std::move(result));
}

MINT_EXPORT_FUNCTION(mint_lang_get_object_globals, 1);
MINT_EXPORT_FUNCTION(mint_lang_get_globals, 0);
MINT_EXPORT_FUNCTION(mint_lang_get_object_types, 1);
MINT_EXPORT_FUNCTION(mint_lang_get_types, 0);

MINT_RAW_FUNCTION(mint_lang_is_main, 0, cursor) {

	cursor.exit_call();
	cursor.exit_call();

	const bool has_va_args = cursor.symbols().contains("va_args");
	const bool is_first_module = !cursor.call_in_progress();

	cursor.stack().emplace_back(mint::create_boolean(has_va_args && is_first_module));
}

MINT_EXPORT_FUNCTION(mint_at_exit, 1);
MINT_EXPORT_FUNCTION(mint_at_error, 1);
MINT_EXPORT_FUNCTION(mint_lang_exec, 2);
MINT_EXPORT_FUNCTION(mint_lang_eval, 2);
