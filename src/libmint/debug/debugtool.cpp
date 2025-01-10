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

using namespace mint;

static std::string g_main_module_path;

std::string mint::get_main_module_path() {
	return g_main_module_path;
}

void mint::set_main_module_path(const std::string &path) {

	g_main_module_path = FileSystem::clean_path(path);

	std::string load_path = g_main_module_path;
	if (auto pos = load_path.rfind(FileSystem::SEPARATOR); pos != std::string::npos) {
		FileSystem::instance().add_to_path(FileSystem::instance().absolute_path(load_path.erase(pos)));
	}
}

bool mint::is_module_file(const std::string &file_path) {
	auto pos = file_path.rfind('.');
	return pos != std::string::npos && file_path.substr(pos) == ".mn";
}

std::string mint::to_system_path(const std::string &module) {
	if (module == Module::MAIN_NAME) {
		return FileSystem::instance().absolute_path(g_main_module_path);
	}
	return FileSystem::instance().get_module_path(module);
}

std::string mint::to_module_path(const std::string &file_path) {
	if (FileSystem::is_equal_path(file_path, g_main_module_path)) {
		return Module::MAIN_NAME;
	}
	if (const std::string root_path = FileSystem::instance().current_path();
		FileSystem::is_sub_path(file_path, root_path)) {
		std::string module_path = FileSystem::instance().relative_path(root_path, file_path);
		module_path.resize(module_path.find('.'));
		for_each(module_path.begin(), module_path.end(), [](char &ch) {
			if (ch == FileSystem::SEPARATOR) {
				ch = '.';
			}
		});
		return module_path;
	}
	for (const std::string &path : FileSystem::instance().library_path()) {
		const std::string root_path = FileSystem::instance().absolute_path(path);
		if (FileSystem::is_sub_path(file_path, root_path)) {
			std::string module_path = FileSystem::instance().relative_path(root_path, file_path);
			module_path.resize(module_path.find('.'));
			for_each(module_path.begin(), module_path.end(), [](char &ch) {
				if (ch == FileSystem::SEPARATOR) {
					ch = '.';
				}
			});
			return module_path;
		}
	}
	return {};
}

std::ifstream mint::get_module_stream(const std::string &module) {
	return std::ifstream(to_system_path(module));
}

std::string mint::get_module_line(const std::string &module, size_t line) {

	std::string line_content;
	std::ifstream stream = get_module_stream(module);

	for (size_t i = 0; i < line; ++i) {
		getline(stream, line_content, '\n');
	}

	return line_content;
}

static std::string escape_sequence(char c) {
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
		return "x" + std::string(buffer);
	}
}

static std::string offset_to_string(int offset) {
	char buffer[11];
	sprintf(buffer, "[%08x]", offset);
	return buffer;
}

static std::string constant_to_string(Cursor *cursor, const Reference *constant) {

	switch (constant->data()->format) {
	case Data::FMT_NONE:
		return "none";
	case Data::FMT_NULL:
		return "null";
	case Data::FMT_NUMBER:
	{
		double fracpart, intpart;
		if ((fracpart = modf(constant->data<Number>()->value, &intpart)) != 0.) {
			return std::to_string(intpart + fracpart);
		}
		return std::to_string(to_integer(intpart));
	}
	case Data::FMT_BOOLEAN:
		return constant->data<Boolean>()->value ? "true" : "false";
	case Data::FMT_OBJECT:
		switch (constant->data<Object>()->metadata->metatype()) {
		case Class::STRING:
			return "'" + [] (const std::string &str) {
				std::string escaped;
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
		case Class::REGEX:
			return constant->data<Regex>()->initializer;
		case Class::ARRAY:
			return "[" +  mint::join(constant->data<Array>()->values, ", ", [cursor](auto it) {
					   return constant_to_string(cursor, &(*it));
				   }) + "]";
		case Class::HASH:
			return "{" + mint::join(constant->data<Hash>()->values, ", ", [cursor](auto it) {
					   return constant_to_string(cursor, &it->first) + " : " + constant_to_string(cursor, &it->second);
				   }) + "}";
		case Class::ITERATOR:
			return "(" + mint::join(constant->data<Iterator>()->ctx, ", ", [cursor](auto it) {
					   return constant_to_string(cursor, &(*it));
				   }) + ")";
		default:
			return mint::to_string(constant->data());
		}
	case Data::FMT_PACKAGE:
		return "(package: " + constant->data<Package>()->data->full_name() + ")";
	case Data::FMT_FUNCTION:
		return "(function: " + mint::join(constant->data<Function>()->mapping, ", ", [ast = cursor->ast()](auto it) {
				   Module *module = ast->get_module(it->second.handle->module);
				   return std::to_string(it->first)
						  + "@" + ast->get_module_name(module)
						  + offset_to_string(static_cast<int>(it->second.handle->offset));
			   }) + ")";
	}

	return {};
}

static std::string flags_to_string(int flags) {

	std::stringstream stream;
	stream << "(";

	if (flags & Reference::PRIVATE_VISIBILITY) {
		stream << "-";
	}

	if (flags & Reference::PROTECTED_VISIBILITY) {
		stream << "#";
	}

	if (flags & Reference::PACKAGE_VISIBILITY) {
		stream << "~";
	}

	if (flags & Reference::GLOBAL) {
		stream << "@";
	}

	if (flags & Reference::CONST_VALUE) {
		stream << "%";
	}

	if (flags & Reference::CONST_ADDRESS) {
		stream << "$";
	}

	stream << ")";
	return stream.str();
}

void mint::dump_command(size_t offset, Node::Command command, Cursor *cursor, std::ostream &stream) {

	stream << offset_to_string(static_cast<int>(offset)) << " ";

	switch (command) {
	case Node::LOAD_MODULE:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "LOAD_MODULE";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::LOAD_FAST:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "LOAD_FAST";
		stream << " " << cursor->next().symbol->str();
		stream << " " << cursor->next().parameter;
		break;
	case Node::LOAD_SYMBOL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "LOAD_SYMBOL";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::LOAD_MEMBER:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "LOAD_MEMBER";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::LOAD_OPERATOR:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "LOAD_OPERATOR";
		stream << " " << get_operator_symbol(static_cast<Class::Operator>(cursor->next().parameter)).str();
		break;
	case Node::LOAD_CONSTANT:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "LOAD_CONSTANT";
		stream << " " << constant_to_string(cursor, cursor->next().constant);
		break;
	case Node::LOAD_VAR_SYMBOL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "LOAD_VAR_SYMBOL";
		break;
	case Node::LOAD_VAR_MEMBER:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "LOAD_VAR_MEMBER";
		break;
	case Node::CLONE_REFERENCE:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CLONE_REFERENCE";
		break;
	case Node::RELOAD_REFERENCE:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "RELOAD_REFERENCE";
		break;
	case Node::UNLOAD_REFERENCE:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "UNLOAD_REFERENCE";
		break;
	case Node::LOAD_EXTRA_ARGUMENTS:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "LOAD_EXTRA_ARGUMENTS";
		break;
	case Node::RESET_SYMBOL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "RESET_SYMBOL";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::RESET_FAST:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "RESET_FAST";
		stream << " " << cursor->next().symbol->str();
		stream << " " << cursor->next().parameter;
		break;
	case Node::CREATE_FAST:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CREATE_FAST";
		stream << " " << cursor->next().symbol->str();
		stream << " " << cursor->next().parameter;
		stream << " " << flags_to_string(cursor->next().parameter);
		break;
	case Node::CREATE_SYMBOL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CREATE_SYMBOL";
		stream << " " << cursor->next().symbol->str();
		stream << " " << flags_to_string(cursor->next().parameter);
		break;
	case Node::CREATE_FUNCTION:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CREATE_FUNCTION";
		stream << " " << cursor->next().symbol->str();
		stream << " " << flags_to_string(cursor->next().parameter);
		break;
	case Node::FUNCTION_OVERLOAD:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "FUNCTION_OVERLOAD";
		break;
	case Node::ALLOC_ITERATOR:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "ALLOC_ITERATOR";
		break;
	case Node::CREATE_ITERATOR:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CREATE_ITERATOR";
		stream << " " << cursor->next().parameter;
		break;
	case Node::ALLOC_ARRAY:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "ALLOC_ARRAY";
		break;
	case Node::CREATE_ARRAY:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CREATE_ARRAY";
		stream << " " << cursor->next().parameter;
		break;
	case Node::ALLOC_HASH:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "ALLOC_HASH";
		break;
	case Node::CREATE_HASH:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CREATE_HASH";
		stream << " " << cursor->next().parameter;
		break;
	case Node::CREATE_LIB:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CREATE_LIB";
		break;
	case Node::REGEX_MATCH:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "REGEX_MATCH";
		break;
	case Node::REGEX_UNMATCH:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "REGEX_UNMATCH";
		break;
	case Node::STRICT_EQ_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "STRICT_EQ_OP";
		break;
	case Node::STRICT_NE_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "STRICT_NE_OP";
		break;
	case Node::OPEN_PACKAGE:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "OPEN_PACKAGE";
		stream << " " << constant_to_string(cursor, cursor->next().constant);
		break;
	case Node::CLOSE_PACKAGE:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CLOSE_PACKAGE";
		break;
	case Node::REGISTER_CLASS:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "REGISTER_CLASS";
		stream << " " << cursor->next().parameter;
		break;
	case Node::MOVE_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "MOVE_OP";
		break;
	case Node::COPY_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "COPY_OP";
		break;
	case Node::ADD_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "ADD_OP";
		break;
	case Node::SUB_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "SUB_OP";
		break;
	case Node::MOD_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "MOD_OP";
		break;
	case Node::MUL_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "MUL_OP";
		break;
	case Node::DIV_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "DIV_OP";
		break;
	case Node::POW_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "POW_OP";
		break;
	case Node::IS_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "IS_OP";
		break;
	case Node::EQ_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "EQ_OP";
		break;
	case Node::NE_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "NE_OP";
		break;
	case Node::LT_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "LT_OP";
		break;
	case Node::GT_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "GT_OP";
		break;
	case Node::LE_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "LE_OP";
		break;
	case Node::GE_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "GE_OP";
		break;
	case Node::INC_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "INC_OP";
		break;
	case Node::DEC_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "DEC_OP";
		break;
	case Node::NOT_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "NOT_OP";
		break;
	case Node::AND_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "AND_OP";
		break;
	case Node::OR_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "OR_OP";
		break;
	case Node::BAND_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "BAND_OP";
		break;
	case Node::BOR_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "BOR_OP";
		break;
	case Node::XOR_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "XOR_OP";
		break;
	case Node::COMPL_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "COMPL_OP";
		break;
	case Node::POS_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "POS_OP";
		break;
	case Node::NEG_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "NEG_OP";
		break;
	case Node::SHIFT_LEFT_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "SHIFT_LEFT_OP";
		break;
	case Node::SHIFT_RIGHT_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "SHIFT_RIGHT_OP";
		break;
	case Node::INCLUSIVE_RANGE_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "INCLUSIVE_RANGE_OP";
		break;
	case Node::EXCLUSIVE_RANGE_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "EXCLUSIVE_RANGE_OP";
		break;
	case Node::SUBSCRIPT_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "SUBSCRIPT_OP";
		break;
	case Node::SUBSCRIPT_MOVE_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "SUBSCRIPT_MOVE_OP";
		break;
	case Node::TYPEOF_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "TYPEOF_OP";
		break;
	case Node::MEMBERSOF_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "MEMBERSOF_OP";
		break;
	case Node::FIND_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "FIND_OP";
		break;
	case Node::IN_OP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "IN_OP";
		break;
	case Node::FIND_DEFINED_SYMBOL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "FIND_DEFINED_SYMBOL";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::FIND_DEFINED_MEMBER:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "FIND_DEFINED_MEMBER";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::FIND_DEFINED_VAR_SYMBOL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "FIND_DEFINED_VAR_MEMBER";
		break;
	case Node::FIND_DEFINED_VAR_MEMBER:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "FIND_DEFINED_VAR_MEMBER";
		break;
	case Node::CHECK_DEFINED:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CHECK_DEFINED";
		break;
	case Node::FIND_INIT:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "FIND_INIT";
		break;
	case Node::FIND_NEXT:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "FIND_NEXT";
		break;
	case Node::FIND_CHECK:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "FIND_CHECK";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::RANGE_INIT:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "RANGE_INIT";
		break;
	case Node::RANGE_NEXT:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "RANGE_NEXT";
		break;
	case Node::RANGE_CHECK:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "RANGE_CHECK";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::RANGE_ITERATOR_CHECK:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "RANGE_ITERATOR_CHECK";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::BEGIN_GENERATOR_EXPRESSION:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "BEGIN_GENERATOR_EXPRESSION";
		break;
	case Node::END_GENERATOR_EXPRESSION:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "END_GENERATOR_EXPRESSION";
		break;
	case Node::YIELD_EXPRESSION:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "YIELD_EXPRESSION";
		break;
	case Node::OPEN_PRINTER:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "OPEN_PRINTER";
		break;
	case Node::CLOSE_PRINTER:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CLOSE_PRINTER";
		break;
	case Node::PRINT:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "PRINT";
		break;
	case Node::OR_PRE_CHECK:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "OR_PRE_CHECK";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::AND_PRE_CHECK:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "AND_PRE_CHECK";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::CASE_JUMP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CASE_JUMP";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::JUMP_ZERO:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "JUMP_ZERO";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::JUMP:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "JUMP";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::SET_RETRIEVE_POINT:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "SET_RETRIEVE_POINT";
		stream << " " << offset_to_string(cursor->next().parameter);
		break;
	case Node::UNSET_RETRIEVE_POINT:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "UNSET_RETRIEVE_POINT";
		break;
	case Node::RAISE:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "RAISE";
		break;
	case Node::YIELD:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "YIELD";
		break;
	case Node::EXIT_GENERATOR:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "EXIT_GENERATOR";
		break;
	case Node::YIELD_EXIT_GENERATOR:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "YIELD_EXIT_GENERATOR";
		break;
	case Node::INIT_CAPTURE:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "INIT_CAPTURE";
		break;
	case Node::CAPTURE_SYMBOL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CAPTURE_SYMBOL";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::CAPTURE_AS:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CAPTURE_AS";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::CAPTURE_ALL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CAPTURE_ALL";
		break;
	case Node::CALL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CALL";
		stream << " " << cursor->next().parameter;
		break;
	case Node::CALL_MEMBER:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CALL_MEMBER";
		stream << " " << cursor->next().parameter;
		break;
	case Node::CALL_BUILTIN:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "CALL_BUILTIN";
		stream << " " << cursor->next().parameter;
		break;
	case Node::INIT_CALL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "INIT_CALL";
		break;
	case Node::INIT_MEMBER_CALL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "INIT_MEMBER_CALL";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::INIT_OPERATOR_CALL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "INIT_OPERATOR_CALL";
		stream << " " << get_operator_symbol(static_cast<Class::Operator>(cursor->next().parameter)).str();
		break;
	case Node::INIT_VAR_MEMBER_CALL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "INIT_VAR_MEMBER_CALL";
		break;
	case Node::INIT_EXCEPTION:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "INIT_EXCEPTION";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::RESET_EXCEPTION:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "RESET_EXCEPTION";
		stream << " " << cursor->next().symbol->str();
		break;
	case Node::INIT_PARAM:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "INIT_PARAM";
		stream << " " << cursor->next().symbol->str();
		stream << " " << flags_to_string(cursor->next().parameter);
		stream << " " << cursor->next().parameter;
		break;
	case Node::EXIT_CALL:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "EXIT_CALL";
		break;
	case Node::EXIT_THREAD:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "EXIT_THREAD";
		break;
	case Node::EXIT_EXEC:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "EXIT_EXEC";
		break;
	case Node::EXIT_MODULE:
		stream << std::setiosflags(std::stringstream::left) << std::setw(32) << "EXIT_MODULE";
		break;
	}

	stream << std::endl;
}
