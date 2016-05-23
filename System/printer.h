#ifndef PRINTER_H
#define PRINTER_H

#include <cstdio>

class Printer {
public:
	Printer(int fd);
	Printer(const char *path);
	~Printer();

	virtual void print(const void *value);
	virtual void print(double value);
	virtual void print(const char *value);

	virtual void printNone();
	virtual void printNull();
	virtual void printFunction();

private:
	FILE *m_output;
	bool m_closable;
};

#endif // PRINTER_H
