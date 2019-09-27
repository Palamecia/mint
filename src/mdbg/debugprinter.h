#ifndef DEBUG_PRINTER_H
#define DEBUG_PRINTER_H

#include <system/printer.h>
#include <string>

namespace mint {
class SharedReference;
class Iterator;
class Array;
class Hash;
}

class DebugPrinter : public mint::Printer {
public:
	DebugPrinter();
	~DebugPrinter() override;

	bool print(DataType type, void *value) override;
	void print(const char *value) override;
	void print(double value) override;
	void print(bool value) override;
};

std::string reference_value(const mint::SharedReference &reference);
std::string iterator_value(mint::Iterator *iterator);
std::string array_value(mint::Array *array);
std::string hash_value(mint::Hash *hash);

void print_debug_trace(const char *format, ...) __attribute__((format(printf, 1, 2)));

#endif // DEBUG_PRINTER_H
