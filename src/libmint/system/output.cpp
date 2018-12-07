#include "system/output.h"

using namespace mint;

Output::Output() : FilePrinter(1) {

}

Output::~Output() {
	FilePrinter::print("\n");
}

Output &Output::instance() {

	static Output g_instance;

	return g_instance;
}

bool Output::print(DataType type, void *value) {

	switch (type) {
	case none:
	case null:
	case object:
	case package:
	case function:
		((void)value);
		break;

	default:
		return false;
	}

	return true;
}

void Output::print(const char *value) {

	FilePrinter::print(value);
	FilePrinter::print("\n");
}

void Output::print(double value) {

	FilePrinter::print(value);
	FilePrinter::print("\n");
}

void Output::print(bool value) {

	FilePrinter::print(value);
	FilePrinter::print("\n");
}

bool Output::global() const {
	return true;
}
