#ifndef OUTPUT_H
#define OUTPUT_H

#include "System/printer.h"

class Output : public Printer {
public:
	static Output &instance();

	void print(const void *value) override;
	void printNone() override;
	void printNull() override;
	void printFunction() override;

private:
	Output();
};

#endif // OUTPUT_H
