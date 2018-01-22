#include "system/output.h"

using namespace mint;

Output::Output() : Printer(1) {}

Output::~Output() {
	Printer::print("\n");
}

Output &Output::instance() {

	static Output g_instance;

	return g_instance;
}

void Output::print(SpecialValue value) {
	((void)value);
}

void Output::print(const char *value) {

	Printer::print(value);
	Printer::print("\n");
}

void Output::print(const void *value) {
	((void)value);
}

void Output::print(double value) {

	Printer::print(value);
	Printer::print("\n");
}
