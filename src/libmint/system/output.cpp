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

void Output::print(SpecialValue value) {
	((void)value);
}

void Output::print(const char *value) {

	FilePrinter::print(value);
	FilePrinter::print("\n");
}

void Output::print(double value) {

	FilePrinter::print(value);
	FilePrinter::print("\n");
}

void Output::print(void *value) {
	((void)value);
}

bool Output::global() const {
	return true;
}
