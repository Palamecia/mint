#ifndef MINT_OBJECTPRINTER_H
#define MINT_OBJECTPRINTER_H

#include "ast/printer.h"
#include "memory/reference.h"

namespace mint {

class Cursor;
struct Object;

class MINT_EXPORT ObjectPrinter : public Printer {
public:
	ObjectPrinter(Cursor *cursor, Reference::Flags flags, Object *object);

	void print(Reference &reference) override;

private:
	StrongReference m_object;
	Cursor *m_cursor;
};

}

#endif // MINT_OBJECTPRINTER_H
