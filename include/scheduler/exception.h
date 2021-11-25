#ifndef MINT_EXCEPTION_H
#define MINT_EXCEPTION_H

#include "scheduler/process.h"

namespace mint {

class MINT_EXPORT Exception : public Process {
public:
	Exception(Reference &&reference, Process *process);
	~Exception() override;

	void setup() override;
	void cleanup() override;

private:
	StrongReference m_reference;
	bool m_handled;
};

MINT_EXPORT bool is_exception(Process *process);

class MINT_EXPORT MintException : public std::exception {
public:
	MintException(Cursor *cursor, Reference &&reference);

	Cursor *cursor();

	Reference &&takeException() noexcept;

	const char *what() const noexcept override;

private:
	Cursor *m_cursor;
	StrongReference m_reference;
};

}

#endif // MINT_EXCEPTION_H
