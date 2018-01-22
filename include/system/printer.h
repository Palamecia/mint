#ifndef PRINTER_H
#define PRINTER_H

#include "config.h"

#include <cstdio>

namespace mint {

class MINT_EXPORT Printer {
public:
	Printer(int fd);
	Printer(const char *path);
	virtual ~Printer();

	enum SpecialValue {
		none,
		null,
		function
	};

	virtual void print(SpecialValue value);
	virtual void print(const char *value);
	virtual void print(const void *value);
	virtual void print(double value);

private:
	FILE *m_output;
	bool m_closable;
};

}

#endif // PRINTER_H
