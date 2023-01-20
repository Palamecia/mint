#ifndef MINT_COMPIMER_H
#define MINT_COMPIMER_H

#include "system/datastream.h"
#include "ast/module.h"

namespace mint {

class MINT_EXPORT Compiler {
public:
	Compiler();

	bool isPrinting() const;
	void setPrinting(bool enabled);

	bool build(DataStream *stream, Module::Infos node);

	static Data *makeLibrary(const std::string &token);
	static Data *makeData(const std::string &token);
	static Data *makeArray();
	static Data *makeHash();
	static Data *makeNone();

private:
	bool m_printing;
};

}

#endif // MINT_COMPIMER_H
