#ifndef OBJECT_PRINTER_H
#define OBJECT_PRINTER_H

#include "system/printer.h"
#include "memory/reference.h"

namespace mint {

class Cursor;
struct Object;

class MINT_EXPORT ObjectPrinter : public Printer {
public:
	ObjectPrinter(Cursor *cursor, Reference::Flags flags, Object *object);

	bool print(DataType type, void *value) override;
	void print(const char *value) override;
	void print(double value) override;
	void print(bool value) override;

private:
	StrongReference m_object;
	Cursor *m_cursor;
};

}

#endif // OBJECT_PRINTER_H
