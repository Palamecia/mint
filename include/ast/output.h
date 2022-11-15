#ifndef MINT_OUTPUT_H
#define MINT_OUTPUT_H

#include "ast/fileprinter.h"

namespace mint {

class MINT_EXPORT Output : public FilePrinter {
public:
	static Output &instance();
	~Output();

	void print(Reference &reference) override;

	bool global() const override;

private:
	Output();
};

}

#endif // MINT_OUTPUT_H