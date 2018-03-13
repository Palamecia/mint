#ifndef FILE_PRINTER_H
#define FILE_PRINTER_H

#include "system/printer.h"

#include <cstdio>

namespace mint {

class MINT_EXPORT FilePrinter : public Printer {
public:
	FilePrinter(int fd);
	FilePrinter(const char *path);
	~FilePrinter();

	void print(SpecialValue value) override;
	void print(const char *value) override;
	void print(double value) override;
	void print(void *value) override;

private:
	FILE *m_output;
	bool m_closable;
};

}

#endif // FILE_PRINTER_H
