#ifndef MINT_OUTPUT_H
#define MINT_OUTPUT_H

#include "system/fileprinter.h"

namespace mint {

class MINT_EXPORT Output : public FilePrinter {
public:
	static Output &instance();
	~Output();

	bool print(DataType type, void *value) override;
	void print(const char *value) override;
	void print(double value) override;
	void print(bool value) override;

	bool global() const override;

private:
	Output();
};

}

#endif // MINT_OUTPUT_H
