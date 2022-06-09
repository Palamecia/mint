#ifndef DEBUG_PRINTER_H
#define DEBUG_PRINTER_H

#include <ast/printer.h>
#include <string>

namespace mint {
class Reference;
struct Iterator;
struct Array;
struct Hash;
struct Function;
}

class DebugPrinter : public mint::Printer {
public:
	DebugPrinter();
	~DebugPrinter() override;

	void print(mint::Reference &reference) override;
};

std::string reference_value(const mint::Reference &reference);
std::string iterator_value(mint::Iterator *iterator);
std::string array_value(mint::Array *array);
std::string hash_value(mint::Hash *hash);
std::string function_value(mint::Function *function);

void print_debug_trace(const char *format, ...) __attribute__((format(printf, 1, 2)));

#endif // DEBUG_PRINTER_H
