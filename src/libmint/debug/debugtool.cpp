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

#include "mint/debug/debugtool.h"
#include "mint/ast/classregister.h"
#include "mint/ast/module.h"
#include "mint/ast/node.h"
#include "mint/ast/symbol.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/casttool.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/abstractsyntaxtreewalker.h"
#include "mint/ast/cursor.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/system/filesystem.h"
#include "mint/system/string.h"
#include "mint/system/utf8.h"

#include <cstddef>
#include <filesystem>
#include <format>
#include <functional>
#include <fstream>
#include <ostream>
#include <ranges>
#include <cmath>
#include <string>
#include <string_view>

using namespace mint;

namespace {

std::string escape_sequence(std::string_view c) {
	if (c.size() != 1) {
		return std::format("x{}", std::views::transform(c, [](const auto ch) {
			return std::format("{:02X}", static_cast<int>(ch));
		}) | std::views::join);
	}
	switch (const auto ch = c[0]) {
	case '\0':
		return "0";
	case '\a':
		return "a";
	case '\b':
		return "b";
	case '\x1B':
		return "e";
	case '\t':
		return "t";
	case '\n':
		return "n";
	case '\v':
		return "v";
	case '\f':
		return "f";
	case '\r':
		return "r";
	default:
		return std::format("x{:02X}", static_cast<int>(ch));
	}
}

class DumpCommand {
	std::reference_wrapper<std::ostream> _stream;
public:
	DumpCommand(std::ostream& stream) :
	    _stream(stream) {}

	Node::Command walk(Cursor& cursor) {
		_stream.get() << to_debug_string(cursor.offset()) << " ";
		const auto command = mint::walk<Node::Command>(cursor, *this);
		_stream.get() << '\n';
		return command;
	}

	Node::Command on_load_module(Cursor& /*cursor*/, const Symbol& symbol) {
		_stream.get() << to_debug_string("LOAD_MODULE");
		_stream.get() << " " << symbol.str();
		return Node::Command::load_module;
	}

	Node::Command on_load_fast(Cursor& /*cursor*/, const Symbol& symbol, std::size_t index) {
		_stream.get() << to_debug_string("LOAD_FAST");
		_stream.get() << " " << symbol.str();
		_stream.get() << " " << index;
		return Node::Command::load_fast;
	}

	Node::Command on_load_symbol(Cursor& /*cursor*/, const Symbol& symbol) {
		_stream.get() << to_debug_string("LOAD_SYMBOL");
		_stream.get() << " " << symbol.str();
		return Node::Command::load_symbol;
	}

	Node::Command on_load_member(Cursor& /*cursor*/, const Symbol& symbol) {
		_stream.get() << to_debug_string("LOAD_MEMBER");
		_stream.get() << " " << symbol.str();
		return Node::Command::load_member;
	}

	Node::Command on_load_operator(Cursor& /*cursor*/, Class::Operator op) {
		_stream.get() << to_debug_string("LOAD_OPERATOR");
		_stream.get() << " " << get_operator_symbol(op).str();
		return Node::Command::load_operator;
	}

	Node::Command on_load_constant(Cursor& cursor, const Reference& constant) {
		_stream.get() << to_debug_string("LOAD_CONSTANT");
		_stream.get() << " " << to_debug_string(cursor, constant);
		return Node::Command::load_constant;
	}

	Node::Command on_load_var_symbol(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("LOAD_VAR_SYMBOL");
		return Node::Command::load_var_symbol;
	}

	Node::Command on_load_var_member(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("LOAD_VAR_MEMBER");
		return Node::Command::load_var_member;
	}

	Node::Command on_load_defined_member(Cursor& /*cursor*/, const Symbol& symbol) {
		_stream.get() << to_debug_string("LOAD_DEFINED_MEMBER");
		_stream.get() << " " << symbol.str();
		return Node::Command::load_defined_member;
	}

	Node::Command on_load_defined_operator(Cursor& /*cursor*/, Class::Operator op) {
		_stream.get() << to_debug_string("LOAD_DEFINED_OPERATOR");
		_stream.get() << " " << get_operator_symbol(op).str();
		return Node::Command::load_defined_operator;
	}

	Node::Command on_load_defined_var_member(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("LOAD_DEFINED_VAR_MEMBER");
		return Node::Command::load_defined_var_member;
	}

	Node::Command on_clone_reference(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("CLONE_REFERENCE");
		return Node::Command::clone_reference;
	}

	Node::Command on_reload_reference(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("RELOAD_REFERENCE");
		return Node::Command::reload_reference;
	}

	Node::Command on_unload_reference(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("UNLOAD_REFERENCE");
		return Node::Command::unload_reference;
	}

	Node::Command on_load_extra_arguments(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("LOAD_EXTRA_ARGUMENTS");
		return Node::Command::load_extra_arguments;
	}

	Node::Command on_reset_symbol(Cursor& /*cursor*/, const Symbol& symbol) {
		_stream.get() << to_debug_string("RESET_SYMBOL");
		_stream.get() << " " << symbol.str();
		return Node::Command::reset_symbol;
	}

	Node::Command on_reset_fast(Cursor& /*cursor*/, const Symbol& symbol, std::size_t index) {
		_stream.get() << to_debug_string("RESET_FAST");
		_stream.get() << " " << symbol.str();
		_stream.get() << " " << index;
		return Node::Command::reset_fast;
	}

	Node::Command on_declare_fast(Cursor& /*cursor*/, const Symbol& symbol, std::size_t index, Reference::Flags flags) {
		_stream.get() << to_debug_string("DECLARE_FAST");
		_stream.get() << " " << symbol.str();
		_stream.get() << " " << index;
		_stream.get() << " " << to_debug_string(flags);
		return Node::Command::declare_fast;
	}

	Node::Command on_declare_symbol(Cursor& /*cursor*/, const Symbol& symbol, Reference::Flags flags) {
		_stream.get() << to_debug_string("DECLARE_SYMBOL");
		_stream.get() << " " << symbol.str();
		_stream.get() << " " << to_debug_string(flags);
		return Node::Command::declare_symbol;
	}

	Node::Command on_declare_function(Cursor& /*cursor*/, const Symbol& symbol, Reference::Flags flags) {
		_stream.get() << to_debug_string("DECLARE_FUNCTION");
		_stream.get() << " " << symbol.str();
		_stream.get() << " " << to_debug_string(flags);
		return Node::Command::declare_function;
	}

	Node::Command on_function_overload(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("FUNCTION_OVERLOAD");
		return Node::Command::function_overload;
	}

	Node::Command on_alloc_iterator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("ALLOC_ITERATOR");
		return Node::Command::alloc_iterator;
	}

	Node::Command on_init_iterator(Cursor& /*cursor*/, std::size_t length) {
		_stream.get() << to_debug_string("INIT_ITERATOR");
		_stream.get() << " " << length;
		return Node::Command::init_iterator;
	}

	Node::Command on_alloc_array(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("ALLOC_ARRAY");
		return Node::Command::alloc_array;
	}

	Node::Command on_init_array(Cursor& /*cursor*/, std::size_t length) {
		_stream.get() << to_debug_string("INIT_ARRAY");
		_stream.get() << " " << length;
		return Node::Command::init_array;
	}

	Node::Command on_alloc_hash(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("ALLOC_HASH");
		return Node::Command::alloc_hash;
	}

	Node::Command on_init_hash(Cursor& /*cursor*/, std::size_t length) {
		_stream.get() << to_debug_string("INIT_HASH");
		_stream.get() << " " << length;
		return Node::Command::init_hash;
	}

	Node::Command on_create_lib(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("CREATE_LIB");
		return Node::Command::create_lib;
	}

	Node::Command on_regex_match(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("REGEX_MATCH");
		return Node::Command::regex_match;
	}

	Node::Command on_regex_unmatch(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("REGEX_UNMATCH");
		return Node::Command::regex_unmatch;
	}

	Node::Command on_strict_eq_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("STRICT_EQ_OPERATOR");
		return Node::Command::strict_eq_operator;
	}

	Node::Command on_strict_ne_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("STRICT_NE_OPERATOR");
		return Node::Command::strict_ne_operator;
	}

	Node::Command on_open_package(Cursor& /*cursor*/, Package& package) {
		_stream.get() << to_debug_string("OPEN_PACKAGE");
		_stream.get() << " " << to_debug_string(package);
		return Node::Command::open_package;
	}

	Node::Command on_close_package(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("CLOSE_PACKAGE");
		return Node::Command::close_package;
	}

	Node::Command on_register_class(Cursor& /*cursor*/, ClassRegister::Id id) {
		_stream.get() << to_debug_string("REGISTER_CLASS");
		_stream.get() << " " << id;
		return Node::Command::register_class;
	}

	Node::Command on_move_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("MOVE_OPERATOR");
		return Node::Command::move_operator;
	}

	Node::Command on_copy_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("COPY_OPERATOR");
		return Node::Command::copy_operator;
	}

	Node::Command on_add_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("ADD_OPERATOR");
		return Node::Command::add_operator;
	}

	Node::Command on_sub_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("SUB_OPERATOR");
		return Node::Command::sub_operator;
	}

	Node::Command on_mod_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("MOD_OPERATOR");
		return Node::Command::mod_operator;
	}

	Node::Command on_mul_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("MUL_OPERATOR");
		return Node::Command::mul_operator;
	}

	Node::Command on_div_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("DIV_OPERATOR");
		return Node::Command::div_operator;
	}

	Node::Command on_pow_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("POW_OPERATOR");
		return Node::Command::pow_operator;
	}

	Node::Command on_is_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("IS_OPERATOR");
		return Node::Command::is_operator;
	}

	Node::Command on_eq_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("EQ_OPERATOR");
		return Node::Command::eq_operator;
	}

	Node::Command on_ne_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("NE_OPERATOR");
		return Node::Command::ne_operator;
	}

	Node::Command on_lt_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("LT_OPERATOR");
		return Node::Command::lt_operator;
	}

	Node::Command on_gt_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("GT_OPERATOR");
		return Node::Command::gt_operator;
	}

	Node::Command on_le_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("LE_OPERATOR");
		return Node::Command::le_operator;
	}

	Node::Command on_ge_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("GE_OPERATOR");
		return Node::Command::ge_operator;
	}

	Node::Command on_inc_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("INC_OPERATOR");
		return Node::Command::inc_operator;
	}

	Node::Command on_dec_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("DEC_OPERATOR");
		return Node::Command::dec_operator;
	}

	Node::Command on_not_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("NOT_OPERATOR");
		return Node::Command::not_operator;
	}

	Node::Command on_and_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("AND_OPERATOR");
		return Node::Command::and_operator;
	}

	Node::Command on_or_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("OR_OPERATOR");
		return Node::Command::or_operator;
	}

	Node::Command on_band_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("BAND_OPERATOR");
		return Node::Command::band_operator;
	}

	Node::Command on_bor_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("BOR_OPERATOR");
		return Node::Command::bor_operator;
	}

	Node::Command on_xor_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("XOR_OPERATOR");
		return Node::Command::xor_operator;
	}

	Node::Command on_compl_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("COMPL_OPERATOR");
		return Node::Command::compl_operator;
	}

	Node::Command on_pos_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("POS_OPERATOR");
		return Node::Command::pos_operator;
	}

	Node::Command on_neg_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("NEG_OPERATOR");
		return Node::Command::neg_operator;
	}

	Node::Command on_shift_left_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("SHIFT_LEFT_OPERATOR");
		return Node::Command::shift_left_operator;
	}

	Node::Command on_shift_right_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("SHIFT_RIGHT_OPERATOR");
		return Node::Command::shift_right_operator;
	}

	Node::Command on_inclusive_range_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("INCLUSIVE_RANGE_OPERATOR");
		return Node::Command::inclusive_range_operator;
	}

	Node::Command on_exclusive_range_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("EXCLUSIVE_RANGE_OPERATOR");
		return Node::Command::exclusive_range_operator;
	}

	Node::Command on_subscript_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("SUBSCRIPT_OPERATOR");
		return Node::Command::subscript_operator;
	}

	Node::Command on_subscript_move_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("SUBSCRIPT_MOVE_OPERATOR");
		return Node::Command::subscript_move_operator;
	}

	Node::Command on_typeof_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("TYPEOF_OPERATOR");
		return Node::Command::typeof_operator;
	}

	Node::Command on_membersof_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("MEMBERSOF_OPERATOR");
		return Node::Command::membersof_operator;
	}

	Node::Command on_find_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("FIND_OPERATOR");
		return Node::Command::find_operator;
	}

	Node::Command on_in_operator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("IN_OPERATOR");
		return Node::Command::in_operator;
	}

	Node::Command on_find_defined_symbol(Cursor& /*cursor*/, const Symbol& symbol) {
		_stream.get() << to_debug_string("FIND_DEFINED_SYMBOL");
		_stream.get() << " " << symbol.str();
		return Node::Command::find_defined_symbol;
	}

	Node::Command on_find_defined_member(Cursor& /*cursor*/, const Symbol& symbol) {
		_stream.get() << to_debug_string("FIND_DEFINED_MEMBER");
		_stream.get() << " " << symbol.str();
		return Node::Command::find_defined_member;
	}

	Node::Command on_find_defined_var_symbol(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("FIND_DEFINED_VAR_SYMBOL");
		return Node::Command::find_defined_var_symbol;
	}

	Node::Command on_find_defined_var_member(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("FIND_DEFINED_VAR_MEMBER");
		return Node::Command::find_defined_var_member;
	}

	Node::Command on_check_defined(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("CHECK_DEFINED");
		return Node::Command::check_defined;
	}

	Node::Command on_find_init(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("FIND_INIT");
		return Node::Command::find_init;
	}

	Node::Command on_find_next(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("FIND_NEXT");
		return Node::Command::find_next;
	}

	Node::Command on_find_check(Cursor& /*cursor*/, std::size_t offset) {
		_stream.get() << to_debug_string("FIND_CHECK");
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::find_check;
	}

	Node::Command on_range_init(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("RANGE_INIT");
		return Node::Command::range_init;
	}

	Node::Command on_range_next(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("RANGE_NEXT");
		return Node::Command::range_next;
	}

	Node::Command on_range_check(Cursor& /*cursor*/, std::size_t offset) {
		_stream.get() << to_debug_string("RANGE_CHECK");
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::range_check;
	}

	Node::Command on_range_iterator_check(Cursor& /*cursor*/, std::size_t offset) {
		_stream.get() << to_debug_string("RANGE_ITERATOR_CHECK");
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::range_iterator_check;
	}

	Node::Command on_begin_generator_expression(Cursor& /*cursor*/, std::size_t offset) {
		_stream.get() << to_debug_string("BEGIN_GENERATOR_EXPRESSION");
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::begin_generator_expression;
	}

	Node::Command on_end_generator_expression(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("END_GENERATOR_EXPRESSION");
		return Node::Command::end_generator_expression;
	}

	Node::Command on_open_printer(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("OPEN_PRINTER");
		return Node::Command::open_printer;
	}

	Node::Command on_close_printer(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("CLOSE_PRINTER");
		return Node::Command::close_printer;
	}

	Node::Command on_print(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("PRINT");
		return Node::Command::print;
	}

	Node::Command on_or_pre_check(Cursor& /*cursor*/, std::size_t offset) {
		_stream.get() << to_debug_string("OR_PRE_CHECK");
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::or_pre_check;
	}

	Node::Command on_and_pre_check(Cursor& /*cursor*/, std::size_t offset) {
		_stream.get() << to_debug_string("AND_PRE_CHECK");
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::and_pre_check;
	}

	Node::Command on_case_jump(Cursor& /*cursor*/, std::size_t offset) {
		_stream.get() << to_debug_string("CASE_JUMP");
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::case_jump;
	}

	Node::Command on_zero_jump(Cursor& /*cursor*/, std::size_t offset) {
		_stream.get() << to_debug_string("ZERO_JUMP");
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::zero_jump;
	}

	Node::Command on_jump(Cursor& /*cursor*/, std::size_t offset) {
		_stream.get() << to_debug_string("JUMP");
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::jump;
	}

	Node::Command on_set_retrieve_point(Cursor& /*cursor*/, std::size_t offset) {
		_stream.get() << to_debug_string("SET_RETRIEVE_POINT");
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::set_retrieve_point;
	}

	Node::Command on_unset_retrieve_point(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("UNSET_RETRIEVE_POINT");
		return Node::Command::unset_retrieve_point;
	}

	Node::Command on_raise(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("RAISE");
		return Node::Command::raise;
	}

	Node::Command on_yield(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("YIELD");
		return Node::Command::yield;
	}

	Node::Command on_exit_generator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("EXIT_GENERATOR");
		return Node::Command::exit_generator;
	}

	Node::Command on_yield_exit_generator(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("YIELD_EXIT_GENERATOR");
		return Node::Command::yield_exit_generator;
	}

	Node::Command on_init_capture(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("INIT_CAPTURE");
		return Node::Command::init_capture;
	}

	Node::Command on_capture_symbol(Cursor& /*cursor*/, const Symbol& symbol) {
		_stream.get() << to_debug_string("CAPTURE_SYMBOL");
		_stream.get() << " " << symbol.str();
		return Node::Command::capture_symbol;
	}

	Node::Command on_capture_as(Cursor& /*cursor*/, const Symbol& symbol) {
		_stream.get() << to_debug_string("CAPTURE_AS");
		_stream.get() << " " << symbol.str();
		return Node::Command::capture_as;
	}

	Node::Command on_capture_all(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("CAPTURE_ALL");
		return Node::Command::capture_all;
	}

	Node::Command on_call(Cursor& /*cursor*/, int signature) {
		_stream.get() << to_debug_string("CALL");
		_stream.get() << " " << signature;
		return Node::Command::call;
	}

	Node::Command on_call_member(Cursor& /*cursor*/, int signature) {
		_stream.get() << to_debug_string("CALL_MEMBER");
		_stream.get() << " " << signature;
		return Node::Command::call_member;
	}

	Node::Command on_call_builtin(Cursor& /*cursor*/, std::size_t index) {
		_stream.get() << to_debug_string("CALL_BUILTIN");
		_stream.get() << " " << index;
		return Node::Command::call_builtin;
	}

	Node::Command on_init_call(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("INIT_CALL");
		return Node::Command::init_call;
	}

	Node::Command on_init_member_call(Cursor& /*cursor*/, const Symbol& symbol) {
		_stream.get() << to_debug_string("INIT_MEMBER_CALL");
		_stream.get() << " " << symbol.str();
		return Node::Command::init_member_call;
	}

	Node::Command on_init_operator_call(Cursor& /*cursor*/, Class::Operator op) {
		_stream.get() << to_debug_string("INIT_OPERATOR_CALL");
		_stream.get() << " " << get_operator_symbol(op).str();
		return Node::Command::init_operator_call;
	}

	Node::Command on_init_var_member_call(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("INIT_VAR_MEMBER_CALL");
		return Node::Command::init_var_member_call;
	}

	Node::Command on_init_defined_member_call(Cursor& /*cursor*/, const Symbol& symbol, std::size_t offset) {
		_stream.get() << to_debug_string("INIT_DEFINED_MEMBER_CALL");
		_stream.get() << " " << symbol.str();
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::init_defined_member_call;
	}

	Node::Command on_init_defined_operator_call(Cursor& /*cursor*/, Class::Operator op, std::size_t offset) {
		_stream.get() << to_debug_string("INIT_DEFINED_OPERATOR_CALL");
		_stream.get() << " " << get_operator_symbol(op).str();
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::init_defined_operator_call;
	}

	Node::Command on_init_defined_var_member_call(Cursor& /*cursor*/, std::size_t offset) {
		_stream.get() << to_debug_string("INIT_DEFINED_VAR_MEMBER_CALL");
		_stream.get() << " " << to_debug_string(offset);
		return Node::Command::init_defined_var_member_call;
	}

	Node::Command on_init_exception(Cursor& /*cursor*/, const Symbol& symbol) {
		_stream.get() << to_debug_string("INIT_EXCEPTION");
		_stream.get() << " " << symbol.str();
		return Node::Command::init_exception;
	}

	Node::Command on_reset_exception(Cursor& /*cursor*/, const Symbol& symbol) {
		_stream.get() << to_debug_string("RESET_EXCEPTION");
		_stream.get() << " " << symbol.str();
		return Node::Command::reset_exception;
	}

	Node::Command on_init_parameter(Cursor& /*cursor*/, const Symbol& symbol, Reference::Flags flags,
	    std::size_t index) {
		_stream.get() << to_debug_string("INIT_PARAMETER");
		_stream.get() << " " << symbol.str();
		_stream.get() << " " << to_debug_string(flags);
		_stream.get() << " " << index;
		return Node::Command::init_parameter;
	}

	Node::Command on_exit_call(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("EXIT_CALL");
		return Node::Command::exit_call;
	}

	Node::Command on_exit_thread(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("EXIT_THREAD");
		return Node::Command::exit_thread;
	}

	Node::Command on_exit_exec(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("EXIT_EXEC");
		return Node::Command::exit_exec;
	}

	Node::Command on_exit_module(Cursor& /*cursor*/) {
		_stream.get() << to_debug_string("EXIT_MODULE");
		return Node::Command::exit_module;
	}
};

}

bool mint::is_module_file(const std::filesystem::path& file_path) {
	return file_path.extension() == ".mn";
}

std::filesystem::path mint::to_system_path(const std::string& module) {
	if (module == Module::main_name) {
		return std::filesystem::absolute(FileSystem::instance().get_main_module_path());
	}
	return FileSystem::instance().get_module_path(module);
}

std::string mint::to_module_path(const std::filesystem::path& file_path) {
	if (const std::filesystem::path main_module_path = FileSystem::instance().get_main_module_path();
	    !main_module_path.empty() && std::filesystem::equivalent(file_path, main_module_path)) {
		return Module::main_name;
	}
	if (const std::filesystem::path root_path = std::filesystem::current_path();
	    FileSystem::is_subpath(file_path, root_path)) {
		return FileSystem::to_module_path(root_path, file_path);
	}
	for (const std::filesystem::path& path : FileSystem::instance().library_path()) {
		if (const std::filesystem::path root_path = std::filesystem::absolute(path);
		    FileSystem::is_subpath(file_path, root_path)) {
			return FileSystem::to_module_path(root_path, file_path);
		}
	}
	return {};
}

std::ifstream mint::get_module_stream(const std::string& module) {
	return std::ifstream(to_system_path(module));
}

std::string mint::get_module_line(const std::string& module, std::size_t line) {

	std::string line_content;
	std::ifstream stream = get_module_stream(module);

	for (std::size_t i = 0; i < line; ++i) {
		getline(stream, line_content, '\n');
	}

	return line_content;
}

Node::Command mint::dump_command(Cursor& cursor, std::ostream& stream) {
	auto dumper = DumpCommand(stream);
	return dumper.walk(cursor);
}

std::string mint::to_debug_string(std::size_t offset) {
	return std::format("{:08x}", offset);
}

std::string mint::to_debug_string(std::string_view command) {
	return std::format("{:32s}", command);
}

std::string mint::to_debug_string(Reference::Flags flags) {
	std::string buffer = "(";
	if (flags & Reference::private_visibility) {
		buffer += "-";
	}
	if (flags & Reference::protected_visibility) {
		buffer += "#";
	}
	if (flags & Reference::package_visibility) {
		buffer += "~";
	}
	if (flags & Reference::global) {
		buffer += "@";
	}
	if (flags & Reference::const_value) {
		buffer += "%";
	}
	if (flags & Reference::const_address) {
		buffer += "$";
	}
	buffer += ")";
	return buffer;
}

std::string mint::to_debug_string(const Number& number) {
	double intpart = 0.;
	const auto fracpart = std::modf(number.value, &intpart);
	if (fracpart != 0.) {
		return std::to_string(intpart + fracpart);
	}
	return std::to_string(to_signed_integer(intpart));
}

std::string mint::to_debug_string(const Boolean& boolean) {
	return boolean.value ? "true" : "false";
}

std::string mint::to_debug_string(const String& string) {
	std::string escaped;
	for (const auto& code_point : views::utf8(string.str)) {
		if (!utf8_is_print(code_point)) {
			escaped += "\\";
			escaped += escape_sequence(code_point);
		}
		else if (code_point == "\\" || code_point == "'") {
			escaped += "\\";
			escaped += code_point;
		}
		else {
			escaped += code_point;
		}
	}
	return std::format("'{}'", escaped);
}

std::string mint::to_debug_string(Cursor& cursor, const Array& array) {
	return std::format("[{}]", std::views::transform(array.values,
	                               [&cursor](const auto& item) {
		                               return to_debug_string(cursor, item);
	                               })
	                               | std::views::join_with(std::string(", ")) | std::ranges::to<std::string>());
}

std::string mint::to_debug_string(Cursor& cursor, const Hash& hash) {
	return std::format("{{{}}}", std::views::transform(hash.values,
	                                 [&cursor](const auto& item) {
		                                 return to_debug_string(cursor, item.first) + " : "
		                                        + to_debug_string(cursor, item.second);
	                                 })
	                                 | std::views::join_with(std::string(", ")) | std::ranges::to<std::string>());
}

std::string mint::to_debug_string(Cursor& cursor, const Iterator& iterator) {
	return std::format("({})", std::views::transform(iterator.ctx.view(),
	                               [&cursor](const auto& item) {
		                               return to_debug_string(cursor, item);
	                               })
	                               | std::views::join_with(std::string(", ")) | std::ranges::to<std::string>());
}

std::string mint::to_debug_string(const Package& package) {
	return std::format("(package: {})", package.data.full_name());
}

std::string mint::to_debug_string(Cursor& cursor, const Function& function) {
	return std::format("(function: {})", std::views::transform(function.mapping,
	                                         [&ast = cursor.ast()](const auto& item) {
		                                         const auto& module = item.second.handle().module;
		                                         return std::to_string(item.first) + "@" + ast.get_module_name(module)
		                                                + to_debug_string(item.second.handle().offset);
	                                         })
	                                         | std::views::join_with(std::string(", "))
	                                         | std::ranges::to<std::string>());
}

std::string mint::to_debug_string(Cursor& cursor, const Reference& constant) {
	switch (constant.data().format()) {
	case Data::none_format:
		return "none";
	case Data::null_format:
		return "null";
	case Data::number_format:
		return to_debug_string(constant.data<Number>());
	case Data::boolean_format:
		return to_debug_string(constant.data<Boolean>());
	case Data::object_format:
		switch (constant.data<Object>().metadata.metatype()) {
		case Class::string:
			return to_debug_string(constant.data<String>());
		case Class::regex:
			return constant.data<Regex>().initializer;
		case Class::array:
			return to_debug_string(cursor, constant.data<Array>());
		case Class::hash:
			return to_debug_string(cursor, constant.data<Hash>());
		case Class::iterator:
			return to_debug_string(cursor, constant.data<Iterator>());
		default:
			return mint::to_string(&constant.data());
		}
	case Data::package_format:
		return to_debug_string(constant.data<Package>());
	case Data::function_format:
		return to_debug_string(cursor, constant.data<Function>());
	}
	return {};
}
