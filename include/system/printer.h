#ifndef MINT_PRINTER_H
#define MINT_PRINTER_H

#include "config.h"

namespace mint {

class MINT_EXPORT Printer {
public:
	virtual ~Printer() = default;

	enum DataType {
		// Basic types
		none,
		null,
		object,
		package,
		function,

		// Extended types
		regex,
		array,
		hash,
		iterator
	};

	virtual bool print(DataType type, void *data) = 0;
	virtual void print(const char *value) = 0;
	virtual void print(double value) = 0;
	virtual void print(bool value) = 0;

	virtual bool global() const;
};

}

#endif // MINT_PRINTER_H
