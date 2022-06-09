#include <memory/functiontool.h>
#include <memory/builtin/string.h>
#include <debug/debugtool.h>
#include <ast/abstractsyntaxtree.h>
#include <ast/cursor.h>
#include <sstream>

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_assembly_from_function, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference object = move(helper.popParameter());
	WeakReference result = create_hash();

	for (const auto &signature : object.data<Function>()->mapping) {

		Module::Handle *handle = signature.second.handle;
		Cursor *dump_cursor = cursor->ast()->createCursor(handle->module);
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

	Module::Infos infos = cursor->ast()->loadModule(object.data<String>()->str);
	Cursor *dump_cursor = cursor->ast()->createCursor(infos.id);
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
