#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "system/utf8iterator.h"

#include <stdio.h>

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_file_fopen, 2,cursor) {

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

MINT_FUNCTION(mint_file_fclose, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	if (file.data<LibObject<FILE>>()->impl) {
		fclose(file.data<LibObject<FILE>>()->impl);
		file.data<LibObject<FILE>>()->impl = nullptr;
	}
}

MINT_FUNCTION(mint_file_fgetc, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	int cptr = fgetc(file.data<LibObject<FILE>>()->impl);

	if (cptr != EOF) {
		string result(1, cptr);
		size_t length = utf8char_length(cptr);
		while (--length) {
			result += fgetc(file.data<LibObject<FILE>>()->impl);
		}
		helper.returnValue(create_string(result));
	}
}

MINT_FUNCTION(mint_file_readline, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	int cptr = fgetc(file.data<LibObject<FILE>>()->impl);

	if (cptr != EOF) {
		string result;
		while ((cptr != '\n') && (cptr != EOF)) {
			result += cptr;
			cptr = fgetc(file.data<LibObject<FILE>>()->impl);
		}
		helper.returnValue(create_string(result));
	}
}
