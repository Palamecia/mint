#include "ast/output.h"
#include "system/terminal.h"
#include "memory/reference.h"
#include "memory/class.h"

using namespace mint;

Output::Output() : FilePrinter(stdout_fileno) {

}

Output::~Output() {
	internalPrint("\n");
}

Output &Output::instance() {

	static Output g_instance;

	return g_instance;
}

void Output::print(Reference &reference) {

	switch (reference.data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
	case Data::fmt_package:
	case Data::fmt_function:
		break;
	case Data::fmt_object:
		switch (reference.data<Object>()->metadata->metatype()) {
		case Class::object:
			break;
		default:
			FilePrinter::print(reference);
			internalPrint("\n");
		}
		break;
	default:
		FilePrinter::print(reference);
		internalPrint("\n");
	}
}

bool Output::global() const {
	return true;
}
