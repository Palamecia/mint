/**
 * Copyright (c) 2025 Gauvain CHERY.
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
#include "mint/scheduler/scheduler.h"
#include "mint/debug/debuginterface.h"
#include "mint/debug/cursordebugger.h"
#include "mint/ast/abstractsyntaxtree.h"
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

using namespace mint;

static constexpr const size_t QUANTUM = 64 * 1024;
static std::atomic_bool g_single_thread(true);
static std::mutex g_step_mutex;

namespace {

bool do_run_steps(Cursor *cursor, size_t count) {

	auto &stack = cursor->stack();
	AbstractSyntaxTree *ast = cursor->ast();

	while (count--) {
		switch (cursor->next().command) {
		case Node::LOAD_MODULE:
			load_module(cursor, cursor->next().symbol->str());
			break;

		case Node::LOAD_FAST:
			{
				Symbol &symbol = *cursor->next().symbol;
				const auto index = static_cast<size_t>(cursor->next().parameter);
				stack.emplace_back(cursor->symbols().get_fast(symbol, index));
			}
			break;
		case Node::LOAD_SYMBOL:
			stack.emplace_back(get_symbol(&cursor->symbols(), *cursor->next().symbol));
			break;
		case Node::LOAD_MEMBER:
			reduce_member(cursor, get_member(cursor, stack.back(), *cursor->next().symbol));
			break;
		case Node::LOAD_OPERATOR:
			reduce_member(cursor,
						  get_operator(cursor, stack.back(), static_cast<Class::Operator>(cursor->next().parameter)));
			break;
		case Node::LOAD_CONSTANT:
			stack.emplace_back(WeakReference::share(*cursor->next().constant));
			break;
		case Node::LOAD_VAR_SYMBOL:
			stack.emplace_back(get_symbol(&cursor->symbols(), var_symbol(cursor)));
			break;
		case Node::LOAD_VAR_MEMBER:
			{
				Symbol &&symbol = var_symbol(cursor);
				reduce_member(cursor, get_member(cursor, stack.back(), symbol));
			}
			break;
		case Node::CLONE_REFERENCE:
			{
				WeakReference reference = std::move(stack.back());
				stack.back() = WeakReference::clone(reference);
				stack.emplace_back(std::forward<Reference>(reference));
			}
			break;
		case Node::RELOAD_REFERENCE:
			stack.emplace_back(WeakReference::share(stack.back()));
			break;
		case Node::UNLOAD_REFERENCE:
			stack.pop_back();
			break;
		case Node::LOAD_EXTRA_ARGUMENTS:
			load_extra_arguments(cursor);
			break;
		case Node::RESET_SYMBOL:
			cursor->symbols().erase(*cursor->next().symbol);
			break;
		case Node::RESET_FAST:
			{
				const Symbol &symbol = *cursor->next().symbol;
				const auto index = static_cast<size_t>(cursor->next().parameter);
				cursor->symbols().erase_fast(symbol, index);
			}
			break;

		case Node::DECLARE_FAST:
			{
				const Symbol &symbol = *cursor->next().symbol;
				const auto index = static_cast<size_t>(cursor->next().parameter);
				const auto flags = static_cast<Reference::Flags>(cursor->next().parameter);
				declare_symbol(cursor, symbol, index, flags);
			}
			break;
		case Node::DECLARE_SYMBOL:
			{
				const Symbol &symbol = *cursor->next().symbol;
				const auto flags = static_cast<Reference::Flags>(cursor->next().parameter);
				declare_symbol(cursor, symbol, flags);
			}
			break;
		case Node::DECLARE_FUNCTION:
			{
				const Symbol &symbol = *cursor->next().symbol;
				const auto flags = static_cast<Reference::Flags>(cursor->next().parameter);
				declare_function(cursor, symbol, flags);
			}
			break;
		case Node::FUNCTION_OVERLOAD:
			function_overload_from_stack(cursor);
			break;
		case Node::ALLOC_ITERATOR:
			cursor->waiting_calls().emplace(
				WeakReference(Reference::CONST_ADDRESS, GarbageCollector::instance().alloc<Iterator>()));
			break;
		case Node::INIT_ITERATOR:
			iterator_new(cursor, static_cast<size_t>(cursor->next().parameter));
			break;
		case Node::ALLOC_ARRAY:
			cursor->waiting_calls().emplace(
				WeakReference(Reference::CONST_ADDRESS, GarbageCollector::instance().alloc<Array>()));
			break;
		case Node::INIT_ARRAY:
			array_new(cursor, static_cast<size_t>(cursor->next().parameter));
			break;
		case Node::ALLOC_HASH:
			cursor->waiting_calls().emplace(
				WeakReference(Reference::CONST_ADDRESS, GarbageCollector::instance().alloc<Hash>()));
			break;
		case Node::INIT_HASH:
			hash_new(cursor, static_cast<size_t>(cursor->next().parameter));
			break;
		case Node::CREATE_LIB:
			stack.emplace_back(WeakReference::create<Library>());
			break;

		case Node::REGEX_MATCH:
			regex_match(cursor);
			break;
		case Node::REGEX_UNMATCH:
			regex_unmatch(cursor);
			break;

		case Node::STRICT_EQ_OP:
			strict_eq_operator(cursor);
			break;
		case Node::STRICT_NE_OP:
			strict_ne_operator(cursor);
			break;

		case Node::OPEN_PACKAGE:
			cursor->symbols().open_package(cursor->next().constant->data<Package>()->data);
			break;
		case Node::CLOSE_PACKAGE:
			cursor->symbols().close_package();
			break;
		case Node::REGISTER_CLASS:
			cursor->symbols().get_package()->register_class(static_cast<ClassRegister::Id>(cursor->next().parameter));
			break;

		case Node::MOVE_OP:
			move_operator(cursor);
			break;
		case Node::COPY_OP:
			copy_operator(cursor);
			break;
		case Node::ADD_OP:
			add_operator(cursor);
			break;
		case Node::SUB_OP:
			sub_operator(cursor);
			break;
		case Node::MOD_OP:
			mod_operator(cursor);
			break;
		case Node::MUL_OP:
			mul_operator(cursor);
			break;
		case Node::DIV_OP:
			div_operator(cursor);
			break;
		case Node::POW_OP:
			pow_operator(cursor);
			break;
		case Node::IS_OP:
			is_operator(cursor);
			break;
		case Node::EQ_OP:
			eq_operator(cursor);
			break;
		case Node::NE_OP:
			ne_operator(cursor);
			break;
		case Node::LT_OP:
			lt_operator(cursor);
			break;
		case Node::GT_OP:
			gt_operator(cursor);
			break;
		case Node::LE_OP:
			le_operator(cursor);
			break;
		case Node::GE_OP:
			ge_operator(cursor);
			break;
		case Node::INC_OP:
			inc_operator(cursor);
			break;
		case Node::DEC_OP:
			dec_operator(cursor);
			break;
		case Node::NOT_OP:
			not_operator(cursor);
			break;
		case Node::AND_OP:
			and_operator(cursor);
			break;
		case Node::OR_OP:
			or_operator(cursor);
			break;
		case Node::BAND_OP:
			band_operator(cursor);
			break;
		case Node::BOR_OP:
			bor_operator(cursor);
			break;
		case Node::XOR_OP:
			xor_operator(cursor);
			break;
		case Node::COMPL_OP:
			compl_operator(cursor);
			break;
		case Node::POS_OP:
			pos_operator(cursor);
			break;
		case Node::NEG_OP:
			neg_operator(cursor);
			break;
		case Node::SHIFT_LEFT_OP:
			shift_left_operator(cursor);
			break;
		case Node::SHIFT_RIGHT_OP:
			shift_right_operator(cursor);
			break;
		case Node::INCLUSIVE_RANGE_OP:
			inclusive_range_operator(cursor);
			break;
		case Node::EXCLUSIVE_RANGE_OP:
			exclusive_range_operator(cursor);
			break;
		case Node::SUBSCRIPT_OP:
			subscript_operator(cursor);
			break;
		case Node::SUBSCRIPT_MOVE_OP:
			subscript_move_operator(cursor);
			break;
		case Node::TYPEOF_OP:
			typeof_operator(cursor);
			break;
		case Node::MEMBERSOF_OP:
			membersof_operator(cursor);
			break;
		case Node::FIND_OP:
			find_operator(cursor);
			break;
		case Node::IN_OP:
			in_operator(cursor);
			break;

		case Node::FIND_DEFINED_SYMBOL:
			find_defined_symbol(cursor, *cursor->next().symbol);
			break;
		case Node::FIND_DEFINED_MEMBER:
			find_defined_member(cursor, *cursor->next().symbol);
			break;
		case Node::FIND_DEFINED_VAR_SYMBOL:
			find_defined_symbol(cursor, var_symbol(cursor));
			break;
		case Node::FIND_DEFINED_VAR_MEMBER:
			find_defined_member(cursor, var_symbol(cursor));
			break;
		case Node::CHECK_DEFINED:
			check_defined(cursor);
			break;

		case Node::FIND_INIT:
			find_init(cursor);
			break;
		case Node::FIND_NEXT:
			find_next(cursor);
			break;
		case Node::FIND_CHECK:
			find_check(cursor, static_cast<size_t>(cursor->next().parameter));
			break;
		case Node::RANGE_INIT:
			range_init(cursor);
			break;
		case Node::RANGE_NEXT:
			range_next(cursor);
			break;
		case Node::RANGE_CHECK:
			range_check(cursor, static_cast<size_t>(cursor->next().parameter));
			break;
		case Node::RANGE_ITERATOR_CHECK:
			range_iterator_check(cursor, static_cast<size_t>(cursor->next().parameter));
			break;

		case Node::BEGIN_GENERATOR_EXPRESSION:
			cursor->begin_generator_expression();
			break;

		case Node::END_GENERATOR_EXPRESSION:
			cursor->end_generator_expression();
			break;

		case Node::YIELD_EXPRESSION:
			cursor->yield_expression(stack.back());
			stack.pop_back();
			break;

		case Node::OPEN_PRINTER:
			cursor->open_printer(create_printer(cursor));
			break;

		case Node::CLOSE_PRINTER:
			cursor->close_printer();
			break;

		case Node::PRINT:
			{
				WeakReference reference = std::move(stack.back());
				stack.pop_back();
				print(cursor->printer(), reference);
			}
			break;

		case Node::OR_PRE_CHECK:
			or_pre_check(cursor, static_cast<size_t>(cursor->next().parameter));
			break;
		case Node::AND_PRE_CHECK:
			and_pre_check(cursor, static_cast<size_t>(cursor->next().parameter));
			break;

		case Node::CASE_JUMP:
			if (to_boolean(stack.back())) {
				cursor->jmp(static_cast<size_t>(cursor->next().parameter));
				stack.pop_back();
			}
			else {
				((void)cursor->next());
			}
			stack.pop_back();
			break;

		case Node::JUMP_ZERO:
			if (to_boolean(stack.back())) {
				((void)cursor->next());
			}
			else {
				cursor->jmp(static_cast<size_t>(cursor->next().parameter));
			}
			stack.pop_back();
			break;

		case Node::JUMP:
			cursor->jmp(static_cast<size_t>(cursor->next().parameter));
			break;

		case Node::SET_RETRIEVE_POINT:
			cursor->set_retrieve_point(static_cast<size_t>(cursor->next().parameter));
			break;
		case Node::UNSET_RETRIEVE_POINT:
			cursor->unset_retrieve_point();
			break;
		case Node::RAISE:
			{
				WeakReference exception = std::move(stack.back());
				stack.pop_back();
				cursor->raise(std::move(exception));
			}
			break;

		case Node::YIELD:
			yield(cursor, cursor->generator());
			break;
		case Node::EXIT_GENERATOR:
			cursor->exit_call();
			break;
		case Node::YIELD_EXIT_GENERATOR:
			yield(cursor, cursor->generator());
			cursor->exit_call();
			break;

		case Node::INIT_CAPTURE:
			assert(is_instance_of(stack.back(), Data::FMT_FUNCTION));
			stack.back() = WeakReference::clone(stack.back());
			break;
		case Node::CAPTURE_SYMBOL:
			capture_symbol(cursor, *cursor->next().symbol);
			break;
		case Node::CAPTURE_AS:
			capture_as_symbol(cursor, *cursor->next().symbol);
			break;
		case Node::CAPTURE_ALL:
			capture_all_symbols(cursor);
			break;
		case Node::CALL:
			call_operator(cursor, cursor->next().parameter);
			break;
		case Node::CALL_MEMBER:
			call_member_operator(cursor, cursor->next().parameter);
			break;
		case Node::CALL_BUILTIN:
			ast->call_builtin_method(static_cast<size_t>(cursor->next().parameter), cursor);
			break;
		case Node::INIT_CALL:
			init_call(cursor);
			break;
		case Node::INIT_MEMBER_CALL:
			init_member_call(cursor, *cursor->next().symbol);
			break;
		case Node::INIT_OPERATOR_CALL:
			init_operator_call(cursor, static_cast<Class::Operator>(cursor->next().parameter));
			break;
		case Node::INIT_VAR_MEMBER_CALL:
			init_member_call(cursor, var_symbol(cursor));
			break;
		case Node::INIT_EXCEPTION:
			init_exception(cursor, *cursor->next().symbol);
			break;
		case Node::RESET_EXCEPTION:
			reset_exception(cursor, *cursor->next().symbol);
			break;
		case Node::INIT_PARAM:
			{
				const Symbol &symbol = *cursor->next().symbol;
				const auto flags = static_cast<Reference::Flags>(cursor->next().parameter);
				const auto index = static_cast<size_t>(cursor->next().parameter);
				init_parameter(cursor, symbol, flags, index);
			}
			break;
		case Node::EXIT_CALL:
			cursor->exit_call();
			break;
		case Node::EXIT_THREAD:
			return false;
		case Node::EXIT_EXEC:
			Scheduler::instance()->exit(static_cast<int>(to_integer(cursor, stack.back())));
			stack.pop_back();
			return false;
		case Node::EXIT_MODULE:
			if (UNLIKELY(!cursor->exit_module())) {
				return false;
			}
		}
	}

	return true;
}

}

bool mint::debug_steps(CursorDebugger *cursor, DebugInterface *handle) {

	lock_processor();

	do {
		for (size_t i = 0; i < QUANTUM; ++i) {
			if (!handle->debug(cursor)) {
				unlock_processor();
				return false;
			}
			if (!do_run_steps(cursor->cursor(), 1)) {
				unlock_processor();
				return false;
			}
		}
	}
	while (g_single_thread);

	unlock_processor();
	return true;
}

bool mint::run_steps(Cursor *cursor) {

	lock_processor();

	do {
		if (!do_run_steps(cursor, QUANTUM)) {
			unlock_processor();
			return false;
		}
	}
	while (g_single_thread);

	unlock_processor();
	return true;
}

bool mint::run_step(Cursor *cursor) {

	lock_processor();

	if (!do_run_steps(cursor, 1)) {
		unlock_processor();
		return false;
	}

	unlock_processor();
	return true;
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
