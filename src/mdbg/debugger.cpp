#include "debugger.h"

#include <debug/cursordebugger.h>
#include <debug/debugtool.h>
#include <memory/builtin/library.h>
#include <memory/builtin/string.h>
#include <memory/builtin/regex.h>
#include <memory/memorytool.h>
#include <memory/casttool.h>
#include <system/terminal.h>
#include <system/output.h>
#include <system/plugin.h>
#include <sstream>
#include <cstring>

using namespace std;
using namespace mint;

string reference_value(const Reference &reference);
string array_value(const Array *array);
string hash_value(const Hash *hash);
string iterator_value(const Iterator *iterator);

string reference_value(const Reference &reference) {

	char address[2 * sizeof(void *) + 3];

	switch (reference.data()->format) {
	case Data::fmt_none:
		return "none";

	case Data::fmt_null:
		return "null";

	case Data::fmt_number:
	case Data::fmt_boolean:
		return to_string(reference);

	case Data::fmt_object:
		switch (reference.data<Object>()->metadata->metatype()) {
		case Class::string:
			return "\"" + reference.data<String>()->str + "\"";

		case Class::regex:
			return reference.data<Regex>()->initializer;

		case Class::array:
			return array_value(reference.data<Array>());

		case Class::hash:
			return hash_value(reference.data<Hash>());

		case Class::iterator:
			return iterator_value(reference.data<Iterator>());

		case Class::library:
			return reference.data<Library>()->plugin->getPath();

		case Class::object:
		case Class::libobject:
			sprintf(address, "0x%p", reference.data());
		}
		break;

	case Data::fmt_package:
		return reference.data<Package>()->data->fullName();

	case Data::fmt_function:
		return "function";
	}

	return "unknown";
}

string array_value(const Array *array) {

	string join;

	for (auto it = array->values.begin(); it != array->values.end(); ++it) {
		if (it != array->values.begin()) {
			join += ", ";
		}
		join += reference_value(**it);
	}

	return "[" + join + "]";
}

string hash_value(const Hash *hash) {

	string join;

	for (auto it = hash->values.begin(); it != hash->values.end(); ++it) {
		if (it != hash->values.begin()) {
			join += ", ";
		}
		join += reference_value(*it->first);
		join += " : ";
		join += reference_value(*it->second);
	}

	return "{" + join + "}";
}

string iterator_value(const Iterator *iterator) {

	string join;

	for (auto it = iterator->ctx.begin(); it != iterator->ctx.end(); ++it) {
		if (it != iterator->ctx.begin()) {
			join += ", ";
		}
		join += reference_value(**it);
	}

	return "(" + join + ")";
}

class DebugPrinter : public Printer {
public:
	DebugPrinter() {

	}

	~DebugPrinter() {

	}

	bool print(DataType type, void *value) override {
		switch (type) {
		case none:
			printf("none\n");
			break;

		case null:
			printf("null\n");
			break;

		case regex:
			printf("%s\n", static_cast<Regex *>(value)->initializer.c_str());
			break;

		case array:
			printf("%s\n", array_value(static_cast<Array *>(value)).c_str());
			break;

		case hash:
			printf("%s\n", hash_value(static_cast<Hash *>(value)).c_str());
			break;

		case iterator:
			printf("%s\n", iterator_value(static_cast<Iterator *>(value)).c_str());
			break;

		case object:
			if (Object *object = static_cast<Object *>(value)) {

				printf("(%s) {\n", object->metadata->name().c_str());

				if (mint::is_object(object)) {
					for (auto member : object->metadata->members()) {
						printf("\t%s : %s\n", member.first.c_str(), reference_value(object->data[member.second->offset]).c_str());
					}
				}
				else {
					for (auto member : object->metadata->members()) {
						printf("\t%s : %s\n", member.first.c_str(), reference_value(member.second->value).c_str());
					}
				}

				printf("}\n");
			}
			break;

		case package:
			printf("package: %s\n", static_cast<Package *>(value)->data->fullName().c_str());
			break;

		case function:
			printf("function\n");
			break;
		}

		return true;
	}

	void print(const char *value) override {
		printf("\"%s\"\n", value);
	}

	void print(double value) override {
		printf("%g\n", value);
	}

	void print(bool value) override {
		printf("%s\n", value ? "true" : "false");
	}
};

Debugger::Debugger(int argc, char **argv) {

	vector<char *> args;

	if (parseArguments(argc, argv, args)) {
		m_scheduler.reset(new Scheduler(args.size(), args.data()));
		m_scheduler->setDebugInterface(this);
	}
}

int Debugger::run() {

	if (m_scheduler) {
		return m_scheduler->run();
	}

	return EXIT_FAILURE;
}

bool Debugger::parseArguments(int argc, char **argv, vector<char *> &args) {

	args.push_back(argv[0]);

	for (int argn = 1; argn < argc; ++argn) {
		if (!parseArgument(argc, argn, argv, args)) {
			return false;
		}
	}

	return true;
}

bool Debugger::parseArgument(int argc, int &argn, char **argv, vector<char *> &args) {

	if (!strcmp(argv[argn], "-b") || !strcmp(argv[argn], "--breackpoint")) {

		if (++argn < argc) {
			string module = argv[argn];
			if (++argn < argc) {
				size_t line = atoi(argv[argn]);
				createBreackpoint(module, line);
				return true;
			}
		}

		return false;
	}

	if (!strcmp(argv[argn], "--version")) {
		printVersion();
		return false;
	}

	if (!strcmp(argv[argn], "--help")) {
		printHelp();
		return false;
	}

	args.push_back(argv[argn]);
	return true;

	return false;
}

void Debugger::printCommands() {
	printf("c | continue :\n");
	printf("\tExecute until next breackpoint\n");
	printf("n | next :\n");
	printf("\tExecute next line\n");
	printf("e | enter :\n");
	printf("\tEnter function\n");
	printf("r | return :\n");
	printf("\tExit function\n");
	printf("bt | backtrace :\n");
	printf("\tPrint backtrace\n");
	printf("bp | breackpoint :\n");
	printf("\tAdd breakcpoint\n");
	printf("p | print :\n");
	printf("\tPrint current line\n");
	printf("l | list :\n");
	printf("\tPrint defined symbols\n");
	printf("s | show :\n");
	printf("\tShow symbol value\n");
	printf("q | quit :\n");
	printf("\tExit program\n");
}

void Debugger::printVersion() {

}

void Debugger::printHelp() {

}

bool Debugger::check(CursorDebugger *cursor) {

	string prompt = cursor->moduleName() + ":" + to_string(cursor->lineNumber()) + " >>> ";

	bool running = true;
	char *command_line = term_read_line(prompt.c_str());
	term_add_history(command_line);

	istringstream stream(command_line);

	/// \todo use a command parser

	string command;
	stream >> command;

	if (command == "c" || command == "continue") {
		doRun();
	}
	else if (command == "n" || command == "next") {
		doNext();
	}
	else if (command == "e" || command == "enter") {
		doEnter();
	}
	else if (command == "r" || command == "return") {
		doReturn();
	}
	else if (command == "q" || command == "quit") {
		running = false;
	}
	else if (command == "p" || command == "print") {

		int count = 0;
		stream >> count;

		string moduleName = cursor->moduleName();
		size_t lineNumber = cursor->lineNumber();

		if (count < 0) {
			count = abs(count);
			for (size_t i = lineNumber - min(static_cast<size_t>(count), lineNumber - 1); i < lineNumber; ++i) {
				printf("    |%s\n", get_module_line(moduleName, i).c_str());
			}
		}

		printf("   >|%s\n", get_module_line(moduleName, lineNumber).c_str());

		for (size_t i = lineNumber + 1; i < lineNumber + count; ++i) {
			printf("    |%s\n", get_module_line(moduleName, i).c_str());
		}
	}
	else if (command == "bt" || command == "backtrace") {
		for (const string &line : cursor->cursor()->dump()) {
			printf("\t%s\n", line.c_str());
		}
	}
	else if (command == "bp" || command == "breakpoint") {
		string module, line;
		stream >> module;
		stream >> line;
		createBreackpoint(module, atoi(line.c_str()));
		printf("\tNew breackpoint at %s:%d\n", module.c_str(), atoi(line.c_str()));
	}
	else if (command == "l" || command == "list") {
		for (const auto &symbol : cursor->cursor()->symbols()) {
			printf("\t%s (%s) : %s\n",
				   symbol.first.c_str(),
				   type_name(symbol.second).c_str(),
				   reference_value(symbol.second).c_str());
		}
	}
	else if (command == "s" || command == "show") {

		DebugPrinter *printer = new DebugPrinter;
		unique_ptr<Process> process(Process::fromBuffer(stream.str().substr(stream.tellg())));

		doRun();
		process->setup();
		process->cursor()->symbols() = cursor->cursor()->symbols();
		process->cursor()->openPrinter(printer);

		do {
			process->debug(Scheduler::quantum, this);
		}
		while (process->cursor()->callInProgress());

		process->cleanup();
		doPause();
	}
	else {
		printCommands();
	}

	free(command_line);
	return running;
}
