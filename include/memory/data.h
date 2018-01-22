#ifndef DATA_H
#define DATA_H

#include "config.h"

namespace mint {

struct MINT_EXPORT Data {
	enum Format {
		fmt_none,
		fmt_null,
		fmt_number,
		fmt_boolean,
		fmt_object,
		fmt_function
	};
	Format format;
	virtual ~Data() = default;

protected:
	friend class Reference;
	Data() { format = fmt_none; }
};

}

#endif // DATA_H
