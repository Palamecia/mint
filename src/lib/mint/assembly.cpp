#include <memory/casttool.h>
#include <memory/globaldata.h>
#include <memory/functiontool.h>
#include <memory/builtin/string.h>
#include <memory/builtin/regex.h>
#include <ast/abstractsyntaxtree.h>
#include <ast/cursor.h>
#include <sstream>
#include <iomanip>
#include <cmath>

using namespace mint;
using namespace std;

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

static string constant_to_string(const Reference *constant) {

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
			return "[" + [] (Array::values_type &values) {
				string join;
				for (auto it = values.begin(); it != values.end(); ++it) {
					if (it != values.begin()) {
						join += ", ";
					}
					join += constant_to_string(&(*it));
				}
				return join;
			} (constant->data<Array>()->values) + "]";
		case Class::hash:
			return "{" + [] (Hash::values_type &values) {
				string join;
				for (auto it = values.begin(); it != values.end(); ++it) {
					if (it != values.begin()) {
						join += ", ";
					}
					join += constant_to_string(&it->first);
					join += " : ";
					join += constant_to_string(&it->second);
				}
				return join;
			} (constant->data<Hash>()->values) + "}";
		case Class::iterator:
			return "(" + [] (Iterator::ctx_type &ctx) {
				string join;
				for (auto it = ctx.begin(); it != ctx.end(); ++it) {
					if (it != ctx.begin()) {
						join += ", ";
					}
					join += constant_to_string(&(*it));
				}
				return join;
			} (constant->data<Iterator>()->ctx) + ")";
		default:
			char buffer[(sizeof(void *) * 2) + 3];
			sprintf(buffer, "0x%0*lX",
					static_cast<int>(sizeof(void *) * 2),
					reinterpret_cast<uintptr_t>(constant->data()));
			return buffer;
		}
	case Data::fmt_package:
		return "(package: " + constant->data<Package>()->data->fullName() + ")";
	case Data::fmt_function:
		return "(function: " + [] (Function::mapping_type &mapping) {
			string join;
			for (auto it = mapping.begin(); it != mapping.end(); ++it) {
				if (it != mapping.begin()) {
					join += ", ";
				}
				join += to_string(it->first);
				join += "@";
				Module *module = AbstractSyntaxTree::instance().getModule(it->second.handle->module);
				join += AbstractSyntaxTree::instance().getModuleName(module);
				join += offset_to_string(static_cast<int>(it->second.handle->offset));
			}
			return join;
		} (constant->data<Function>()->mapping) + ")";
	}

	return string();
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

static void dump_command(size_t offset, Node::Command command, Cursor *cursor, stringstream &stream) {

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
		stream << " " << constant_to_string(cursor->next().constant);
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
	case Node::create_symbol:
		stream << setiosflags(stringstream::left) << setw(32) << "CREATE_SYMBOL";
		stream << " " << cursor->next().symbol->str();
		stream << " " << flags_to_string(cursor->next().parameter);
		break;
	case Node::create_iterator:
		stream << setiosflags(stringstream::left) << setw(32) << "CREATE_ITERATOR";
		stream << " " << cursor->next().parameter;
		break;
	case Node::create_array:
		stream << setiosflags(stringstream::left) << setw(32) << "CREATE_ARRAY";
		break;
	case Node::create_hash:
		stream << setiosflags(stringstream::left) << setw(32) << "CREATE_HASH";
		break;
	case Node::create_lib:
		stream << setiosflags(stringstream::left) << setw(32) << "CREATE_LIB";
		break;
	case Node::array_insert:
		stream << setiosflags(stringstream::left) << setw(32) << "ARRAY_INSERT";
		break;
	case Node::hash_insert:
		stream << setiosflags(stringstream::left) << setw(32) << "HASH_INSERT";
		break;
	case Node::regex_match:
		stream << setiosflags(stringstream::left) << setw(32) << "REGEX_MATCH";
		break;
	case Node::regex_unmatch:
		stream << setiosflags(stringstream::left) << setw(32) << "REGEX_UNMATCH";
		break;
	case Node::open_package:
		stream << setiosflags(stringstream::left) << setw(32) << "OPEN_PACKAGE";
		stream << " " << constant_to_string(cursor->next().constant);
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
	case Node::range_iterator_finalize:
		stream << setiosflags(stringstream::left) << setw(32) << "RANGE_ITERATOR_FINALIZE";
		break;
	case Node::range_iterator_check:
		stream << setiosflags(stringstream::left) << setw(32) << "RANGE_ITERATOR_CHECK";
		stream << " " << offset_to_string(cursor->next().parameter);
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
	case Node::load_current_result:
		stream << setiosflags(stringstream::left) << setw(32) << "LOAD_CURRENT_RESULT";
		break;
	case Node::load_generator_result:
		stream << setiosflags(stringstream::left) << setw(32) << "LOAD_GENERATOR_RESULT";
		break;
	case Node::finalize_iterator:
		stream << setiosflags(stringstream::left) << setw(32) << "FINALIZE_ITERATOR";
		break;
	case Node::capture_symbol:
		stream << setiosflags(stringstream::left) << setw(32) << "CAPTURE_SYMBOL";
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
	case Node::init_param:
		stream << setiosflags(stringstream::left) << setw(32) << "INIT_PARAM";
		stream << " " << cursor->next().symbol->str();
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

MINT_FUNCTION(mint_assembly_from_function, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference object = move(helper.popParameter());
	WeakReference result = create_hash();

	for (const auto &signature : object.data<Function>()->mapping) {

		Module::Handle *handle = signature.second.handle;
		Cursor *dump_cursor = AbstractSyntaxTree::instance().createCursor(handle->module);
		dump_cursor->jmp(handle->offset - 1);

		size_t end_offset = static_cast<size_t>(dump_cursor->next().parameter);
		stringstream stream;

		for (size_t offset = dump_cursor->offset(); offset < end_offset; offset = dump_cursor->offset()) {
			dump_command(offset, dump_cursor->next().command, dump_cursor, stream);
		}

		hash_insert(result.data<Hash>(), create_number(signature.first), create_string(stream.str()));
	}

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_assembly_from_module, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference object = move(helper.popParameter());

	Module::Infos infos = AbstractSyntaxTree::instance().loadModule(object.data<String>()->str);
	Cursor *dump_cursor = AbstractSyntaxTree::instance().createCursor(infos.id);
	bool has_next = true;
	stringstream stream;

	while (has_next) {
		const size_t offset = dump_cursor->offset();
		switch (Node::Command command = dump_cursor->next().command) {
		case Node::module_end:
			dump_command(offset, command, dump_cursor, stream);
			has_next = false;
			break;
		default:
			dump_command(offset, command, dump_cursor, stream);
		}
	}

	helper.returnValue(create_string(stream.str()));
}
