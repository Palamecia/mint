#ifndef OUTPUT_H
#define OUTPUT_H

#include "system/printer.h"

class Output : public Printer {
public:
	static Output &instance();
	~Output();

	void print(SpecialValue value) override;
	void print(const char *value) override;
	void print(const void *value) override;
	void print(double value) override;

private:
	Output();
};

#endif // OUTPUT_H
