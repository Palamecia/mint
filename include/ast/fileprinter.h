#ifndef MINT_FILEPRINTER_H
#define MINT_FILEPRINTER_H

#include "ast/printer.h"

#include <cstdio>

namespace mint {

class MINT_EXPORT FilePrinter : public Printer {
public:
	FilePrinter(const char *path);
	FilePrinter(int fd);
	~FilePrinter();

	void print(Reference &reference) override;

	FILE *file() const;

protected:
	FilePrinter &operator =(const FilePrinter &other) = delete;
	FilePrinter(const FilePrinter &other) = delete;

	int internalPrint(const char *str);

private:
	int (*m_print)(FILE *stream, const char *str);
	int (*m_close)(FILE *stream);
	FILE *m_stream;
};

}

#endif // MINT_FILEPRINTER_H
