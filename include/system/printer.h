#ifndef PRINTER_H
#define PRINTER_H

#include "config.h"

namespace mint {

class MINT_EXPORT Printer {
public:
	virtual ~Printer() = default;

	enum SpecialValue {
		none,
		null,
		package,
		function
	};

	virtual void print(SpecialValue value) = 0;
	virtual void print(const char *value) = 0;
	virtual void print(double value) = 0;
	virtual void print(void *value) = 0;

	virtual bool global() const;
};

}

#endif // PRINTER_H
