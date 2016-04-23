#ifndef PRINTER_H
#define PRINTER_H

#include <cstdio>

class Printer {
public:
	Printer(int fd);
	Printer(const char *path);
	~Printer();

	void print(const void *value);
	void print(double value);
	void print(const char *value);

	void printNull();

private:
	FILE *m_output;
	bool m_closable;
};

#endif // PRINTER_H
