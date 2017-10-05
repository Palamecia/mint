#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "memory/builtin/libobject.h"
#include "system/utf8iterator.h"

#include <stdio.h>

using namespace std;

extern "C" {

void mint_file_fopen_2(Cursor *cursor) {

	FunctionHelper helper(cursor, 2);

	string mode = to_string(*helper.popParameter());
	string path = to_string(*helper.popParameter());

	if (FILE *file = fopen(path.c_str(), mode.c_str())) {
		helper.returnValue(create_object(file));
	}
	else {
		helper.returnValue(Reference::create<Null>());
	}
}

void mint_file_fclose_1(Cursor *cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	if (((LibObject<FILE> *)file.data())->impl) {
		fclose(((LibObject<FILE> *)file.data())->impl);
		((LibObject<FILE> *)file.data())->impl = nullptr;
	}
}

void mint_file_fgetc_1(Cursor *cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	int cptr = fgetc(((LibObject<FILE> *)file.data())->impl);

	if (cptr != EOF) {
		string result(1, cptr);
		size_t length = utf8char_length(cptr);
		while (--length) {
			result += fgetc(((LibObject<FILE> *)file.data())->impl);
		}
		helper.returnValue(create_string(result));
	}
}

void mint_file_readline_1(Cursor *cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	int cptr = fgetc(((LibObject<FILE> *)file.data())->impl);

	if (cptr != EOF) {
		string result;
		while ((cptr != '\n') && (cptr != EOF)) {
			result += cptr;
			cptr = fgetc(((LibObject<FILE> *)file.data())->impl);
		}
		helper.returnValue(create_string(result));
	}
}

}
