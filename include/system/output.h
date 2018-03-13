#ifndef OUTPUT_H
#define OUTPUT_H

#include "system/fileprinter.h"

namespace mint {

class MINT_EXPORT Output : public FilePrinter {
public:
	static Output &instance();
	~Output();

	void print(SpecialValue value) override;
	void print(const char *value) override;
	void print(double value) override;
	void print(void *value) override;

	bool global() const override;

private:
	Output();
};

}

#endif // OUTPUT_H
