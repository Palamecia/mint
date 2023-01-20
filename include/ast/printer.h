#ifndef MINT_PRINTER_H
#define MINT_PRINTER_H

#include "config.h"

namespace mint {

class Reference;

class MINT_EXPORT Printer {
public:
	Printer() = default;
	Printer(const Printer &other) = delete;
	virtual ~Printer() = default;

	Printer &operator =(const Printer &other) = delete;

	virtual void print(Reference &reference) = 0;

	virtual bool global() const;
};

}

#endif // MINT_PRINTER_H
