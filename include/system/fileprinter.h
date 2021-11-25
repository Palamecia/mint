#ifndef MINT_FILEPRINTER_H
#define MINT_FILEPRINTER_H

#include "system/printer.h"

#include <cstdio>

namespace mint {

class MINT_EXPORT FilePrinter : public Printer {
public:
	FilePrinter(int fd);
	FilePrinter(const char *path);
	~FilePrinter();

	bool print(DataType type, void *data) override;
	void print(const char *value) override;
	void print(double value) override;
	void print(bool value) override;

	FILE *file() const;

private:
	FILE *m_output;
	bool m_closable;
};

}

#endif // MINT_FILEPRINTER_H
