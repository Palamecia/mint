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

#include "mint/debug/debugtool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/system/filesystem.h"
#include "mint/system/string.h"

#include <sstream>
#include <fstream>
#include <iomanip>
#include <cmath>

using namespace std;
using namespace mint;

static string g_main_module_path;

string mint::get_main_module_path() {
	return g_main_module_path;
}

void mint::set_main_module_path(const string &path) {

	g_main_module_path = FileSystem::clean_path(path);

	string load_path = g_main_module_path;
	if (auto pos = load_path.rfind(FileSystem::separator); pos != string::npos) {
		FileSystem::instance().add_to_path(FileSystem::instance().absolute_path(load_path.erase(pos)));
	}
}

bool mint::is_module_file(const string &file_path) {
	auto pos = file_path.rfind('.');
	return pos != string::npos && file_path.substr(pos) == ".mn";
}

string mint::to_system_path(const std::string &module) {
	if (module == Module::main_name) {
		return FileSystem::instance().absolute_path(g_main_module_path);
	}
	return FileSystem::instance().get_module_path(module);
}

string mint::to_module_path(const string &file_path) {
	if (FileSystem::is_equal_path(file_path, g_main_module_path)) {
		return Module::main_name;
	}
	if (const string root_path = FileSystem::instance().current_path();
		FileSystem::is_sub_path(file_path, root_path)) {
		string module_path = FileSystem::instance().relative_path(root_path, file_path);
		module_path.resize(module_path.find('.'));
		for_each(module_path.begin(), module_path.end(), [](char &ch) {
			if (ch == FileSystem::separator) {
				ch = '.';
			}
		});
		return module_path;
	}
	for (const string &path : FileSystem::instance().library_path()) {
		const string root_path = FileSystem::instance().absolute_path(path);
		if (FileSystem::is_sub_path(file_path, root_path)) {
			string module_path = FileSystem::instance().relative_path(root_path, file_path);
			module_path.resize(module_path.find('.'));
			for_each(module_path.begin(), module_path.end(), [](char &ch) {
				if (ch == FileSystem::separator) {
					ch = '.';
				}
			});
			return module_path;
		}
	}
	return {};
}

ifstream mint::get_module_stream(const string &module) {
	return ifstream(to_system_path(module));
}

string mint::get_module_line(const string &module, size_t line) {

	string line_content;
	ifstream stream = get_module_stream(module);

	for (size_t i = 0; i < line; ++i) {
		getline(stream, line_content, '\n');
	}

	return line_content;
}

static string escape_sequence(char c) {
	switch (c) {
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
		char buffer[3];
		sprintf(buffer, "%02X", static_cast<int>(c));
		return "x" + string(buffer);
	}
}

static string offset_to_string(int offset) {
	char buffer[11];
	sprintf(buffer, "[%08x]", offset);
	return buffer;
}

static string constant_to_string(Cursor *cursor, const Reference *constant) {

	switch (constant->data()->format) {
	case Data::fmt_none:
		return "none";
	case Data::fmt_null:
		return "null";
	case Data::fmt_number:
	{
		double fracpart, intpart;
		if ((fracpart = modf(constant->data<Number>()->value, &intpart)) != 0.) {
			return std::to_string(intpart + fracpart);
		}
		return std::to_string(to_integer(intpart));
	}
	case Data::fmt_boolean:
		return constant->data<Boolean>()->value ? "true" : "false";
	case Data::fmt_object:
		switch (constant->data<Object>()->metadata->metatype()) {
		case Class::string:
			return "'" + [] (const string &str) {
				string escaped;
				for (auto it = str.begin(); it != str.end(); ++it) {
					if (!isprint(*it)) {
						escaped += "\\";
						escaped += escape_sequence(*it);
					}
					else if (*it == '\\' || *it == '\'') {
						escaped += "\\";
						escaped += *it;
					}
					else {
						escaped += *it;
					}
				}
				return escaped;
			} (constant->data<String>()->str) + "'";
		case Class::regex:
			return constant->data<Regex>()->initializer;
		case Class::array:
			return "[" +  mint::join(constant->data<Array>()->values, ", ", [cursor](auto it) {
					   return constant_to_string(cursor, &(*it));
				   }) + "]";
		case Class::hash:
			return "{" + mint::join(constant->data<Hash>()->values, ", ", [cursor](auto it) {
					   return constant_to_string(cursor, &it->first) + " : " + constant_to_string(cursor, &it->second);
				   }) + "}";
		case Class::iterator:
			return "(" + mint::join(constant->data<Iterator>()->ctx, ", ", [cursor](auto it) {
					   return constant_to_string(cursor, &(*it));
				   }) + ")";
		default:
			return mint::to_string(constant->data());
		}
	case Data::fmt_package:
		return "(package: " + constant->data<Package>()->data->full_name() + ")";
	case Data::fmt_function:
		return "(function: " + mint::join(constant->data<Function>()->mapping, ", ", [ast = cursor->ast()](auto it) {
				   Module *module = ast->get_module(it->second.handle->module);
				   return to_string(it->first)
						  + "@" + ast->get_module_name(module)
						  + offset_to_string(static_cast<int>(it->second.handle->offset));
			   }) + ")";
	}

	return {};
}

static string flags_to_string(int flags) {

	stringstream stream;
	stream << "(";

	if (flags & Reference::private_visibility) {
		stream << "-";
	}

	if (flags & Reference::protected_visibility) {
		stream << "#";
	}

	if (flags & Reference::package_visibility) {
		stream << "~";
	}

	if (flags & Reference::global) {
		stream << "@";
	}

	if (flags & Reference::const_value) {
		stream << "%";
	}

	if (flags & Reference::const_address) {
		stream << "$";
	}

	stream << ")";
	return stream.str();
}

void mint::dump_command(size_t offset, Node::Command command, Cursor *cursor, ostream &stream) {

	stream << offset_to_string(static_cast<int>(offset)) << " ";

	switch (command) {
	case Node::load_module:
		stream << setiosflags(stringstream::left) << setw(32) << "LOAD_MODULE";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::load_fast:
		stream << setiosflags(stringstream::left) << setw(32) << "LOAD_FAST";
		stream << " " << cursor->next().symbol->str();
		stream << " " << cursor->next().parameter;
		break;
	case Node::load_symbol:
		stream << setiosflags(stringstream::left) << setw(32) << "LOAD_SYMBOL";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::load_member:
		stream << setiosflags(stringstream::left) << setw(32) << "LOAD_MEMBER";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::load_operator:
		stream << setiosflags(stringstream::left) << setw(32) << "LOAD_OPERATOR";
		stream << " " << get_operator_symbol(static_cast<Class::Operator>(cursor->next().parameter)).str();
		break;
	case Node::load_constant:
		stream << setiosflags(stringstream::left) << setw(32) << "LOAD_CONSTANT";
		stream << " " << constant_to_string(cursor, cursor->next().constant);
		break;
	case Node::load_var_symbol:
		stream << setiosflags(stringstream::left) << setw(32) << "LOAD_VAR_SYMBOL";
		break;
	case Node::load_var_member:
		stream << setiosflags(stringstream::left) << setw(32) << "LOAD_VAR_MEMBER";
		break;
	case Node::store_reference:
		stream << setiosflags(stringstream::left) << setw(32) << "STORE_REFERENCE";
		break;
	case Node::reload_reference:
		stream << setiosflags(stringstream::left) << setw(32) << "RELOAD_REFERENCE";
		break;
	case Node::unload_reference:
		stream << setiosflags(stringstream::left) << setw(32) << "UNLOAD_REFERENCE";
		break;
	case Node::load_extra_arguments:
		stream << setiosflags(stringstream::left) << setw(32) << "LOAD_EXTRA_ARGUMENTS";
		break;
	case Node::reset_symbol:
		stream << setiosflags(stringstream::left) << setw(32) << "RESET_SYMBOL";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::reset_fast:
		stream << setiosflags(stringstream::left) << setw(32) << "RESET_FAST";
		stream << " " << cursor->next().symbol->str();
		stream << " " << cursor->next().parameter;
		break;
	case Node::create_fast:
		stream << setiosflags(stringstream::left) << setw(32) << "CREATE_FAST";
		stream << " " << cursor->next().symbol->str();
		stream << " " << cursor->next().parameter;
		stream << " " << flags_to_string(cursor->next().parameter);
		break;
	case Node::create_symbol:
		stream << setiosflags(stringstream::left) << setw(32) << "CREATE_SYMBOL";
		stream << " " << cursor->next().symbol->str();
		stream << " " << flags_to_string(cursor->next().parameter);
		break;
	case Node::create_function:
		stream << setiosflags(stringstream::left) << setw(32) << "CREATE_FUNCTION";
		stream << " " << cursor->next().symbol->str();
		stream << " " << flags_to_string(cursor->next().parameter);
		break;
	case Node::alloc_iterator:
		stream << setiosflags(stringstream::left) << setw(32) << "ALLOC_ITERATOR";
		break;
	case Node::create_iterator:
		stream << setiosflags(stringstream::left) << setw(32) << "CREATE_ITERATOR";
		stream << " " << cursor->next().parameter;
		break;
	case Node::alloc_array:
		stream << setiosflags(stringstream::left) << setw(32) << "ALLOC_ARRAY";
		break;
	case Node::create_array:
		stream << setiosflags(stringstream::left) << setw(32) << "CREATE_ARRAY";
		stream << " " << cursor->next().parameter;
		break;
	case Node::alloc_hash:
		stream << setiosflags(stringstream::left) << setw(32) << "ALLOC_HASH";
		break;
	case Node::create_hash:
		stream << setiosflags(stringstream::left) << setw(32) << "CREATE_HASH";
		stream << " " << cursor->next().parameter;
		break;
	case Node::create_lib:
		stream << setiosflags(stringstream::left) << setw(32) << "CREATE_LIB";
		break;
	case Node::function_overload:
		stream << setiosflags(stringstream::left) << setw(32) << "FUNCTION_OVERLOAD";
		break;
	case Node::regex_match:
		stream << setiosflags(stringstream::left) << setw(32) << "REGEX_MATCH";
		break;
	case Node::regex_unmatch:
		stream << setiosflags(stringstream::left) << setw(32) << "REGEX_UNMATCH";
		break;
	case Node::strict_eq_op:
		stream << setiosflags(stringstream::left) << setw(32) << "STRICT_EQ_OP";
		break;
	case Node::strict_ne_op:
		stream << setiosflags(stringstream::left) << setw(32) << "STRICT_NE_OP";
		break;
	case Node::open_package:
		stream << setiosflags(stringstream::left) << setw(32) << "OPEN_PACKAGE";
		stream << " " << constant_to_string(cursor, cursor->next().constant);
		break;
	case Node::close_package:
		stream << setiosflags(stringstream::left) << setw(32) << "CLOSE_PACKAGE";
		break;
	case Node::register_class:
		stream << setiosflags(stringstream::left) << setw(32) << "REGISTER_CLASS";
		stream << " " << cursor->next().parameter;
		break;
	case Node::move_op:
		stream << setiosflags(stringstream::left) << setw(32) << "MOVE_OP";
		break;
	case Node::copy_op:
		stream << setiosflags(stringstream::left) << setw(32) << "COPY_OP";
		break;
	case Node::add_op:
		stream << setiosflags(stringstream::left) << setw(32) << "ADD_OP";
		break;
	case Node::sub_op:
		stream << setiosflags(stringstream::left) << setw(32) << "SUB_OP";
		break;
	case Node::mod_op:
		stream << setiosflags(stringstream::left) << setw(32) << "MOD_OP";
		break;
	case Node::mul_op:
		stream << setiosflags(stringstream::left) << setw(32) << "MUL_OP";
		break;
	case Node::div_op:
		stream << setiosflags(stringstream::left) << setw(32) << "DIV_OP";
		break;
	case Node::pow_op:
		stream << setiosflags(stringstream::left) << setw(32) << "POW_OP";
		break;
	case Node::is_op:
		stream << setiosflags(stringstream::left) << setw(32) << "IS_OP";
		break;
	case Node::eq_op:
		stream << setiosflags(stringstream::left) << setw(32) << "EQ_OP";
		break;
	case Node::ne_op:
		stream << setiosflags(stringstream::left) << setw(32) << "NE_OP";
		break;
	case Node::lt_op:
		stream << setiosflags(stringstream::left) << setw(32) << "LT_OP";
		break;
	case Node::gt_op:
		stream << setiosflags(stringstream::left) << setw(32) << "GT_OP";
		break;
	case Node::le_op:
		stream << setiosflags(stringstream::left) << setw(32) << "LE_OP";
		break;
	case Node::ge_op:
		stream << setiosflags(stringstream::left) << setw(32) << "GE_OP";
		break;
	case Node::inc_op:
		stream << setiosflags(stringstream::left) << setw(32) << "INC_OP";
		break;
	case Node::dec_op:
		stream << setiosflags(stringstream::left) << setw(32) << "DEC_OP";
		break;
	case Node::not_op:
		stream << setiosflags(stringstream::left) << setw(32) << "NOT_OP";
		break;
	case Node::and_op:
		stream << setiosflags(stringstream::left) << setw(32) << "AND_OP";
		break;
	case Node::or_op:
		stream << setiosflags(stringstream::left) << setw(32) << "OR_OP";
		break;
	case Node::band_op:
		stream << setiosflags(stringstream::left) << setw(32) << "BAND_OP";
		break;
	case Node::bor_op:
		stream << setiosflags(stringstream::left) << setw(32) << "BOR_OP";
		break;
	case Node::xor_op:
		stream << setiosflags(stringstream::left) << setw(32) << "XOR_OP";
		break;
	case Node::compl_op:
		stream << setiosflags(stringstream::left) << setw(32) << "COMPL_OP";
		break;
	case Node::pos_op:
		stream << setiosflags(stringstream::left) << setw(32) << "POS_OP";
		break;
	case Node::neg_op:
		stream << setiosflags(stringstream::left) << setw(32) << "NEG_OP";
		break;
	case Node::shift_left_op:
		stream << setiosflags(stringstream::left) << setw(32) << "SHIFT_LEFT_OP";
		break;
	case Node::shift_right_op:
		stream << setiosflags(stringstream::left) << setw(32) << "SHIFT_RIGHT_OP";
		break;
	case Node::inclusive_range_op:
		stream << setiosflags(stringstream::left) << setw(32) << "INCLUSIVE_RANGE_OP";
		break;
	case Node::exclusive_range_op:
		stream << setiosflags(stringstream::left) << setw(32) << "EXCLUSIVE_RANGE_OP";
		break;
	case Node::subscript_op:
		stream << setiosflags(stringstream::left) << setw(32) << "SUBSCRIPT_OP";
		break;
	case Node::subscript_move_op:
		stream << setiosflags(stringstream::left) << setw(32) << "SUBSCRIPT_MOVE_OP";
		break;
	case Node::typeof_op:
		stream << setiosflags(stringstream::left) << setw(32) << "TYPEOF_OP";
		break;
	case Node::membersof_op:
		stream << setiosflags(stringstream::left) << setw(32) << "MEMBERSOF_OP";
		break;
	case Node::find_op:
		stream << setiosflags(stringstream::left) << setw(32) << "FIND_OP";
		break;
	case Node::in_op:
		stream << setiosflags(stringstream::left) << setw(32) << "IN_OP";
		break;
	case Node::find_defined_symbol:
		stream << setiosflags(stringstream::left) << setw(32) << "FIND_DEFINED_SYMBOL";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::find_defined_member:
		stream << setiosflags(stringstream::left) << setw(32) << "FIND_DEFINED_MEMBER";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::find_defined_var_symbol:
		stream << setiosflags(stringstream::left) << setw(32) << "FIND_DEFINED_VAR_MEMBER";
		break;
	case Node::find_defined_var_member:
		stream << setiosflags(stringstream::left) << setw(32) << "FIND_DEFINED_VAR_MEMBER";
		break;
	case Node::check_defined:
		stream << setiosflags(stringstream::left) << setw(32) << "CHECK_DEFINED";
		break;
	case Node::find_init:
		stream << setiosflags(stringstream::left) << setw(32) << "FIND_INIT";
		break;
	case Node::find_next:
		stream << setiosflags(stringstream::left) << setw(32) << "FIND_NEXT";
		break;
	case Node::find_check:
		stream << setiosflags(stringstream::left) << setw(32) << "FIND_CHECK";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::range_init:
		stream << setiosflags(stringstream::left) << setw(32) << "RANGE_INIT";
		break;
	case Node::range_next:
		stream << setiosflags(stringstream::left) << setw(32) << "RANGE_NEXT";
		break;
	case Node::range_check:
		stream << setiosflags(stringstream::left) << setw(32) << "RANGE_CHECK";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::range_iterator_check:
		stream << setiosflags(stringstream::left) << setw(32) << "RANGE_ITERATOR_CHECK";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::begin_generator_expression:
		stream << setiosflags(stringstream::left) << setw(32) << "BEGIN_GENERATOR_EXPRESSION";
		break;
	case Node::end_generator_expression:
		stream << setiosflags(stringstream::left) << setw(32) << "END_GENERATOR_EXPRESSION";
		break;
	case Node::yield_expression:
		stream << setiosflags(stringstream::left) << setw(32) << "YIELD_EXPRESSION";
		break;
	case Node::open_printer:
		stream << setiosflags(stringstream::left) << setw(32) << "OPEN_PRINTER";
		break;
	case Node::close_printer:
		stream << setiosflags(stringstream::left) << setw(32) << "CLOSE_PRINTER";
		break;
	case Node::print:
		stream << setiosflags(stringstream::left) << setw(32) << "PRINT";
		break;
	case Node::or_pre_check:
		stream << setiosflags(stringstream::left) << setw(32) << "OR_PRE_CHECK";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::and_pre_check:
		stream << setiosflags(stringstream::left) << setw(32) << "AND_PRE_CHECK";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::case_jump:
		stream << setiosflags(stringstream::left) << setw(32) << "CASE_JUMP";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::jump_zero:
		stream << setiosflags(stringstream::left) << setw(32) << "JUMP_ZERO";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::jump:
		stream << setiosflags(stringstream::left) << setw(32) << "JUMP";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::set_retrieve_point:
		stream << setiosflags(stringstream::left) << setw(32) << "SET_RETRIEVE_POINT";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::unset_retrieve_point:
		stream << setiosflags(stringstream::left) << setw(32) << "UNSET_RETRIEVE_POINT";
		break;
	case Node::raise:
		stream << setiosflags(stringstream::left) << setw(32) << "RAISE";
		break;
	case Node::yield:
		stream << setiosflags(stringstream::left) << setw(32) << "YIELD";
		break;
	case Node::exit_generator:
		stream << setiosflags(stringstream::left) << setw(32) << "EXIT_GENERATOR";
		break;
	case Node::yield_exit_generator:
		stream << setiosflags(stringstream::left) << setw(32) << "YIELD_EXIT_GENERATOR";
		break;
	case Node::init_capture:
		stream << setiosflags(stringstream::left) << setw(32) << "INIT_CAPTURE";
		break;
	case Node::capture_symbol:
		stream << setiosflags(stringstream::left) << setw(32) << "CAPTURE_SYMBOL";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::capture_as:
		stream << setiosflags(stringstream::left) << setw(32) << "CAPTURE_AS";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::capture_all:
		stream << setiosflags(stringstream::left) << setw(32) << "CAPTURE_ALL";
		break;
	case Node::call:
		stream << setiosflags(stringstream::left) << setw(32) << "CALL";
		stream << " " << cursor->next().parameter;
		break;
	case Node::call_member:
		stream << setiosflags(stringstream::left) << setw(32) << "CALL_MEMBER";
		stream << " " << cursor->next().parameter;
		break;
	case Node::call_builtin:
		stream << setiosflags(stringstream::left) << setw(32) << "CALL_BUILTIN";
		stream << " " << cursor->next().parameter;
		break;
	case Node::init_call:
		stream << setiosflags(stringstream::left) << setw(32) << "INIT_CALL";
		break;
	case Node::init_member_call:
		stream << setiosflags(stringstream::left) << setw(32) << "INIT_MEMBER_CALL";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::init_operator_call:
		stream << setiosflags(stringstream::left) << setw(32) << "INIT_OPERATOR_CALL";
		stream << " " << get_operator_symbol(static_cast<Class::Operator>(cursor->next().parameter)).str();
		break;
	case Node::init_var_member_call:
		stream << setiosflags(stringstream::left) << setw(32) << "INIT_VAR_MEMBER_CALL";
		break;
	case Node::init_exception:
		stream << setiosflags(stringstream::left) << setw(32) << "INIT_EXCEPTION";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::reset_exception:
		stream << setiosflags(stringstream::left) << setw(32) << "RESET_EXCEPTION";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::init_param:
		stream << setiosflags(stringstream::left) << setw(32) << "INIT_PARAM";
		stream << " " << cursor->next().symbol->str();
		stream << " " << flags_to_string(cursor->next().parameter);
		stream << " " << cursor->next().parameter;
		break;
	case Node::exit_call:
		stream << setiosflags(stringstream::left) << setw(32) << "EXIT_CALL";
		break;
	case Node::exit_thread:
		stream << setiosflags(stringstream::left) << setw(32) << "EXIT_THREAD";
		break;
	case Node::exit_exec:
		stream << setiosflags(stringstream::left) << setw(32) << "EXIT_EXEC";
		break;
	case Node::module_end:
		stream << setiosflags(stringstream::left) << setw(32) << "MODULE_END";
		break;
	}

	stream << endl;
}
