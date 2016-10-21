#include "system/output.h"

Output::Output() : Printer(1) {}

Output::~Output() {
	print("\n");
}

Output &Output::instance() {

	static Output g_instance;

	return g_instance;
}

void Output::print(const void *value) {
	((void)value);
}

void Output::printNone() {}

void Output::printNull() {}

void Output::printFunction() {}
