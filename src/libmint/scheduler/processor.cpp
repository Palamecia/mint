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

#include "mint/scheduler/processor.h"
#include "mint/ast/classregister.h"
#include "mint/ast/module.h"
#include "mint/ast/symbol.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/symboltable.h"
#include "mint/scheduler/scheduler.h"
#include "mint/debug/debuginterface.h"
#include "mint/debug/cursordebugger.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/abstractsyntaxtreewalker.h"
#include "mint/ast/asttools.h"
#include "mint/ast/cursor.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/operatortool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/globaldata.h"
#include "mint/system/assert.h"
#include <atomic>
#include <cassert>
#include <cstddef>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

using namespace mint;

namespace {

constexpr const std::size_t quantum = 64 * 1024;
std::atomic_bool g_single_thread {true};
std::mutex g_step_mutex;

}

namespace {

class DoRunSteps {
	bool _can_continue = true;
public:
	bool walk(Cursor& cursor, std::size_t count = 1) {
		while (_can_continue && count--) {
			mint::walk<void>(cursor, *this);
		}
		return _can_continue;
	}

	static void on_load_module(Cursor& cursor, const Symbol& symbol) {
		load_module(cursor, symbol.str());
	}

	static void on_load_fast(Cursor& cursor, const Symbol& symbol, std::size_t index) {
		cursor.stack().emplace_back(cursor.symbols().get_fast(symbol, index));
	}

	static void on_load_symbol(Cursor& cursor, const Symbol& symbol) {
		cursor.stack().emplace_back(get_symbol(cursor, symbol));
	}

	static void on_load_member(Cursor& cursor, const Symbol& symbol) {
		auto [member, _] = get_member(cursor, cursor.stack().back(), symbol);
		reduce_member(cursor, std::move(member));
	}

	static void on_load_operator(Cursor& cursor, Class::Operator op) {
		auto [member, _] = get_operator(cursor, cursor.stack().back(), op);
		reduce_member(cursor, std::move(member));
	}

	static void on_load_constant(Cursor& cursor, const Reference& constant) {
		cursor.stack().emplace_back(constant);
	}

	static void on_load_var_symbol(Cursor& cursor) {
		cursor.stack().emplace_back(get_symbol(cursor, var_symbol(cursor)));
	}

	static void on_load_var_member(Cursor& cursor) {
		Symbol&& symbol = var_symbol(cursor);
		auto [member, _] = get_member(cursor, cursor.stack().back(), symbol);
		reduce_member(cursor, std::move(member));
	}

	static void on_load_defined_member(Cursor& cursor, const Symbol& symbol) {
		const auto& object = cursor.stack().back();
		if (!is_instance_of(object, Data::Format::none)) {
			auto [member, _] = get_member(cursor, object, symbol);
			reduce_member(cursor, std::move(member));
		}
	}

	static void on_load_defined_operator(Cursor& cursor, Class::Operator op) {
		const auto& object = cursor.stack().back();
		if (!is_instance_of(object, Data::Format::none)) {
			auto [member, _] = get_operator(cursor, object, op);
			reduce_member(cursor, std::move(member));
		}
	}

	static void on_load_defined_var_member(Cursor& cursor) {
		Symbol&& symbol = var_symbol(cursor);
		const auto& object = cursor.stack().back();
		if (!is_instance_of(object, Data::Format::none)) {
			auto [member, _] = get_member(cursor, object, symbol);
			reduce_member(cursor, std::move(member));
		}
	}

	static void on_clone_reference(Cursor& cursor) {
		WeakReference reference = std::move(cursor.stack().back());
		cursor.stack().back() = WeakReference(copy_from, reference);
		cursor.stack().emplace_back(std::move(reference));
	}

	static void on_reload_reference(Cursor& cursor) {
		cursor.stack().emplace_back(cursor.stack().back());
	}

	static void on_unload_reference(Cursor& cursor) {
		cursor.stack().pop_back();
	}

	static void on_load_extra_arguments(Cursor& cursor) {
		load_extra_arguments(cursor);
	}

	static void on_reset_symbol(Cursor& cursor, const Symbol& symbol) {
		cursor.symbols().erase(symbol);
	}

	static void on_reset_fast(Cursor& cursor, const Symbol& symbol, std::size_t index) {
		cursor.symbols().erase_fast(symbol, index);
	}

	static void on_declare_fast(Cursor& cursor, const Symbol& symbol, std::size_t index, Reference::Flags flags) {
		declare_symbol(cursor, symbol, index, flags);
	}

	static void on_declare_symbol(Cursor& cursor, const Symbol& symbol, Reference::Flags flags) {
		declare_symbol(cursor, symbol, flags);
	}

	static void on_declare_function(Cursor& cursor, const Symbol& symbol, Reference::Flags flags) {
		declare_function(cursor, symbol, flags);
	}

	static void on_function_overload(Cursor& cursor) {
		function_overload_from_stack(cursor);
	}

	static void on_alloc_iterator(Cursor& cursor) {
		cursor.waiting_calls().emplace(make_weak_reference<Iterator>(Reference::const_address, cursor.ast()));
	}

	static void on_init_iterator(Cursor& cursor, std::size_t length) {
		iterator_new(cursor, length);
	}

	static void on_alloc_array(Cursor& cursor) {
		cursor.waiting_calls().emplace(make_weak_reference<Array>(Reference::const_address, cursor.ast()));
	}

	static void on_init_array(Cursor& cursor, std::size_t length) {
		array_new(cursor, length);
	}

	static void on_alloc_hash(Cursor& cursor) {
		cursor.waiting_calls().emplace(make_weak_reference<Hash>(Reference::const_address, cursor.ast()));
	}

	static void on_init_hash(Cursor& cursor, std::size_t length) {
		hash_new(cursor, length);
	}

	static void on_create_lib(Cursor& cursor) {
		constexpr auto flags = Reference::const_address | mint::Reference::const_value | Reference::temporary;
		cursor.stack().emplace_back(make_weak_reference<Library>(flags, cursor.ast()));
	}

	static void on_regex_match(Cursor& cursor) {
		regex_match(cursor);
	}

	static void on_regex_unmatch(Cursor& cursor) {
		regex_unmatch(cursor);
	}

	static void on_strict_eq_operator(Cursor& cursor) {
		strict_eq_operator(cursor);
	}

	static void on_strict_ne_operator(Cursor& cursor) {
		strict_ne_operator(cursor);
	}

	static void on_open_package(Cursor& cursor, Package& package) {
		cursor.symbols().open_package(package.data);
	}

	static void on_close_package(Cursor& cursor) {
		cursor.symbols().close_package();
	}

	static void on_register_class(Cursor& cursor, ClassRegister::Id id) {
		cursor.symbols().get_package().register_class(id);
	}

	static void on_move_operator(Cursor& cursor) {
		move_operator(cursor);
	}

	static void on_copy_operator(Cursor& cursor) {
		copy_operator(cursor);
	}

	static void on_add_operator(Cursor& cursor) {
		add_operator(cursor);
	}

	static void on_sub_operator(Cursor& cursor) {
		sub_operator(cursor);
	}

	static void on_mod_operator(Cursor& cursor) {
		mod_operator(cursor);
	}

	static void on_mul_operator(Cursor& cursor) {
		mul_operator(cursor);
	}

	static void on_div_operator(Cursor& cursor) {
		div_operator(cursor);
	}

	static void on_pow_operator(Cursor& cursor) {
		pow_operator(cursor);
	}

	static void on_is_operator(Cursor& cursor) {
		is_operator(cursor);
	}

	static void on_eq_operator(Cursor& cursor) {
		eq_operator(cursor);
	}

	static void on_ne_operator(Cursor& cursor) {
		ne_operator(cursor);
	}

	static void on_lt_operator(Cursor& cursor) {
		lt_operator(cursor);
	}

	static void on_gt_operator(Cursor& cursor) {
		gt_operator(cursor);
	}

	static void on_le_operator(Cursor& cursor) {
		le_operator(cursor);
	}

	static void on_ge_operator(Cursor& cursor) {
		ge_operator(cursor);
	}

	static void on_inc_operator(Cursor& cursor) {
		inc_operator(cursor);
	}

	static void on_dec_operator(Cursor& cursor) {
		dec_operator(cursor);
	}

	static void on_not_operator(Cursor& cursor) {
		not_operator(cursor);
	}

	static void on_and_operator(Cursor& cursor) {
		and_operator(cursor);
	}

	static void on_or_operator(Cursor& cursor) {
		or_operator(cursor);
	}

	static void on_band_operator(Cursor& cursor) {
		band_operator(cursor);
	}

	static void on_bor_operator(Cursor& cursor) {
		bor_operator(cursor);
	}

	static void on_xor_operator(Cursor& cursor) {
		xor_operator(cursor);
	}

	static void on_compl_operator(Cursor& cursor) {
		compl_operator(cursor);
	}

	static void on_pos_operator(Cursor& cursor) {
		pos_operator(cursor);
	}

	static void on_neg_operator(Cursor& cursor) {
		neg_operator(cursor);
	}

	static void on_shift_left_operator(Cursor& cursor) {
		shift_left_operator(cursor);
	}

	static void on_shift_right_operator(Cursor& cursor) {
		shift_right_operator(cursor);
	}

	static void on_inclusive_range_operator(Cursor& cursor) {
		inclusive_range_operator(cursor);
	}

	static void on_exclusive_range_operator(Cursor& cursor) {
		exclusive_range_operator(cursor);
	}

	static void on_subscript_operator(Cursor& cursor) {
		subscript_operator(cursor);
	}

	static void on_subscript_move_operator(Cursor& cursor) {
		subscript_move_operator(cursor);
	}

	static void on_typeof_operator(Cursor& cursor) {
		typeof_operator(cursor);
	}

	static void on_membersof_operator(Cursor& cursor) {
		membersof_operator(cursor);
	}

	static void on_find_operator(Cursor& cursor) {
		find_operator(cursor);
	}

	static void on_in_operator(Cursor& cursor) {
		in_operator(cursor);
	}

	static void on_find_defined_symbol(Cursor& cursor, const Symbol& symbol) {
		find_defined_symbol(cursor, symbol);
	}

	static void on_find_defined_member(Cursor& cursor, const Symbol& symbol) {
		find_defined_member(cursor, symbol);
	}

	static void on_find_defined_var_symbol(Cursor& cursor) {
		find_defined_symbol(cursor, var_symbol(cursor));
	}

	static void on_find_defined_var_member(Cursor& cursor) {
		find_defined_member(cursor, var_symbol(cursor));
	}

	static void on_check_defined(Cursor& cursor) {
		check_defined(cursor);
	}

	static void on_find_init(Cursor& cursor) {
		find_init(cursor);
	}

	static void on_find_next(Cursor& cursor) {
		find_next(cursor);
	}

	static void on_find_check(Cursor& cursor, std::size_t offset) {
		find_check(cursor, offset);
	}

	static void on_range_init(Cursor& cursor) {
		range_init(cursor);
	}

	static void on_range_next(Cursor& cursor) {
		range_next(cursor);
	}

	static void on_range_check(Cursor& cursor, std::size_t offset) {
		range_check(cursor, offset);
	}

	static void on_range_iterator_check(Cursor& cursor, std::size_t offset) {
		range_iterator_check(cursor, offset);
	}

	static void on_range_expression_check(Cursor& cursor, std::size_t offset) {
		range_expression_check(cursor, offset);
	}

	static void on_begin_generator_expression(Cursor& cursor, std::size_t offset) {
		cursor.call_generator_expression(offset);
	}

	static void on_begin_async_generator_expression(Cursor& cursor, std::size_t offset) {
		cursor.call_async_generator_expression(offset);
	}

	static void on_end_generator_expression(Cursor& cursor) {
		assert(cursor.is_in_generator());
		cursor.exit_call();
	}

	static void on_end_async_generator_expression(Cursor& cursor) {
		assert(cursor.is_in_generator() && cursor.is_in_coroutine());
		const mint::WeakReference coroutine = cursor.coroutine();
		coroutine.data<Coroutine>().exit(cursor);
	}

	static void on_unpack_generator_expression(Cursor& cursor) {

		auto value = std::move(cursor.stack().back());
		assert(is_iterator(value));

		if (!value.data<Iterator>().ctx.empty()) {
			cursor.stack().back() = std::move(value.data<Iterator>().ctx.get());
		}
		else {
			cursor.stack().back() = create_none();
		}
	}

	static void on_open_printer(Cursor& cursor) {
		cursor.open_printer(create_printer(cursor));
	}

	static void on_close_printer(Cursor& cursor) {
		cursor.close_printer();
	}

	static void on_print(Cursor& cursor) {

		auto reference = std::move(cursor.stack().back());
		cursor.stack().pop_back();

		auto* printer = cursor.printer();
		assert(printer);
		printer->print(reference);
	}

	static void on_or_pre_check(Cursor& cursor, std::size_t offset) {
		or_pre_check(cursor, offset);
	}

	static void on_and_pre_check(Cursor& cursor, std::size_t offset) {
		and_pre_check(cursor, offset);
	}

	static void on_case_jump(Cursor& cursor, std::size_t offset) {
		if (to_boolean(cursor.stack().back())) {
			cursor.jmp(offset);
			cursor.stack().pop_back();
		}
		cursor.stack().pop_back();
	}

	static void on_zero_jump(Cursor& cursor, std::size_t offset) {
		if (!to_boolean(cursor.stack().back())) {
			cursor.jmp(offset);
		}
		cursor.stack().pop_back();
	}

	static void on_jump(Cursor& cursor, std::size_t offset) {
		cursor.jmp(offset);
	}

	static void on_set_retrieve_point(Cursor& cursor, std::size_t offset) {
		cursor.set_retrieve_point(offset);
	}

	static void on_unset_retrieve_point(Cursor& cursor) {
		cursor.unset_retrieve_point();
	}

	static void on_raise(Cursor& cursor) {
		WeakReference exception = std::move(cursor.stack().back());
		cursor.stack().pop_back();
		cursor.raise(std::move(exception));
	}

	static void on_await(Cursor& cursor) {
		auto object = cursor.stack().back();
		switch (object.data().format()) {
		case Data::Format::object:
			if (auto* method = object.data<Object>().metadata.find_member(builtin_symbols::await_method)) {
				init_member_call(cursor, builtin_symbols::await_method, *method);
				call_member_operator(cursor, 0);
			}
			break;
		case Data::Format::coroutine:
			cursor.stack().pop_back();
			object.data<Coroutine>().await(cursor, std::move(object));
			break;
		default:
			break;
		}
	}

	static void on_resume_coroutine(Cursor& cursor) {

		auto result = std::move(cursor.stack().back());
		cursor.stack().pop_back();

		assert(cursor.is_in_coroutine());
		const mint::WeakReference coroutine = cursor.coroutine();
		coroutine.data<Coroutine>().resume(cursor, std::move(result));
	}

	static void on_yield(Cursor& cursor) {
		const auto item = std::move(cursor.stack().back());
		cursor.stack().pop_back();
		iterator_yield(cursor, cursor.generator().data<Iterator>(), WeakReference(create_from, item));
	}

	static void on_exit_generator(Cursor& cursor) {
		cursor.exit_call();
	}

	static void on_exit_async_generator(Cursor& cursor) {
		assert(cursor.is_in_coroutine());
		const mint::WeakReference coroutine = cursor.coroutine();
		coroutine.data<Coroutine>().exit(cursor);
	}

	static void on_yield_exit_generator(Cursor& cursor) {
		const auto item = std::move(cursor.stack().back());
		cursor.stack().pop_back();
		iterator_return(cursor, cursor.generator().data<Iterator>(), WeakReference(create_from, item));
	}

	static void on_yield_exit_async_generator(Cursor& cursor) {
		const auto item = std::move(cursor.stack().back());
		cursor.stack().pop_back();
		iterator_resume(cursor, cursor.generator().data<Iterator>(), WeakReference(create_from, item));
	}

	static void on_init_capture(Cursor& cursor) {
		auto& function = cursor.stack().back();
		assert(is_instance_of(function, Data::Format::function));
		cursor.stack().back() = WeakReference(copy_from, function.flags() | Reference::temporary, function.data());
	}

	static void on_capture_symbol(Cursor& cursor, const Symbol& symbol) {
		capture_symbol(cursor, symbol);
	}

	static void on_capture_as(Cursor& cursor, const Symbol& symbol) {
		capture_as_symbol(cursor, symbol);
	}

	static void on_capture_all(Cursor& cursor) {
		capture_all_symbols(cursor);
	}

	static void on_call(Cursor& cursor, int signature) {
		call_operator(cursor, signature);
	}

	static void on_call_member(Cursor& cursor, int signature) {
		call_member_operator(cursor, signature);
	}

	static void on_call_builtin(Cursor& cursor, std::size_t index) {
		cursor.ast().call_builtin_method(index, cursor);
	}

	static void on_call_global_builtin(Cursor& cursor, std::size_t index) {
		cursor.ast().call_global_builtin_method(index, cursor);
	}

	static void on_init_call(Cursor& cursor) {
		init_call(cursor);
	}

	static void on_init_member_call(Cursor& cursor, const Symbol& symbol) {
		init_member_call(cursor, symbol);
	}

	static void on_init_operator_call(Cursor& cursor, Class::Operator op) {
		init_operator_call(cursor, op);
	}

	static void on_init_var_member_call(Cursor& cursor) {
		init_member_call(cursor, var_symbol(cursor));
	}

	static void on_init_defined_member_call(Cursor& cursor, const Symbol& symbol, std::size_t offset) {
		const auto& object = cursor.stack().back();
		if (!is_instance_of(object, Data::Format::none)) {
			init_member_call(cursor, symbol);
		}
		else {
			cursor.jmp(offset);
		}
	}

	static void on_init_defined_operator_call(Cursor& cursor, Class::Operator op, std::size_t offset) {
		const auto& object = cursor.stack().back();
		if (!is_instance_of(object, Data::Format::none)) {
			init_operator_call(cursor, op);
		}
		else {
			cursor.jmp(offset);
		}
	}

	static void on_init_defined_var_member_call(Cursor& cursor, std::size_t offset) {
		const auto symbol = var_symbol(cursor);
		const auto& object = cursor.stack().back();
		if (!is_instance_of(object, Data::Format::none)) {
			init_member_call(cursor, symbol);
		}
		else {
			cursor.jmp(offset);
		}
	}

	static void on_init_exception(Cursor& cursor, const Symbol& symbol) {
		init_exception(cursor, symbol);
	}

	static void on_reset_exception(Cursor& cursor, const Symbol& symbol) {
		reset_exception(cursor, symbol);
	}

	static void on_init_parameter(Cursor& cursor, const Symbol& symbol, Reference::Flags flags, std::size_t index) {
		init_parameter(cursor, symbol, flags, index);
	}

	static void on_exit_call(Cursor& cursor) {
		cursor.exit_call();
	}

	void on_exit_thread(Cursor& /*cursor*/) {
		_can_continue = false;
	}

	void on_exit_exec(Cursor& cursor) {
		auto* scheduler = Scheduler::instance();
		assert_x(scheduler, __func__, "execution should be done using a scheduler");
		scheduler->exit(to_integer<int>(cursor, cursor.stack().back()));
		cursor.stack().pop_back();
		_can_continue = false;
	}

	void on_exit_module(Cursor& cursor) {
		_can_continue = cursor.exit_module();
	}
};

}

ProcessorLocker::ProcessorLocker() {
	lock_processor();
}

ProcessorLocker::~ProcessorLocker() {
	unlock_processor();
}

bool mint::debug_steps(CursorDebugger& cursor, DebugInterface& handle) {

	auto do_run_steps = DoRunSteps();

	do {
		for (std::size_t i = 0; i < quantum; ++i) {
			if (!handle.debug(cursor)) {
				return false;
			}
			if (!do_run_steps.walk(cursor.cursor())) {
				return false;
			}
		}
		static GarbageCollector& g_garbage_collector = GarbageCollector::instance();
		if (g_garbage_collector.is_threshold_exceded()) {
			g_garbage_collector.collect();
		}
	}
	while (g_single_thread);

	return true;
}

bool mint::run_steps(Cursor& cursor) {

	auto do_run_steps = DoRunSteps();

	do {
		if (!do_run_steps.walk(cursor, quantum)) {
			return false;
		}
		static GarbageCollector& g_garbage_collector = GarbageCollector::instance();
		if (g_garbage_collector.is_threshold_exceded()) {
			g_garbage_collector.collect();
		}
	}
	while (g_single_thread);

	return true;
}

bool mint::run_step(Cursor& cursor) {
	auto do_run_steps = DoRunSteps();
	return do_run_steps.walk(cursor);
}

void mint::set_multi_thread(bool enabled) {
	g_single_thread = !enabled;
}

void mint::lock_processor() {
	while (!g_step_mutex.try_lock()) {
		std::this_thread::yield();
	}
}

void mint::unlock_processor() {
	g_step_mutex.unlock();
	if (!g_single_thread) {
		std::this_thread::yield();
	}
}
