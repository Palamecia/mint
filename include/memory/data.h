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
	const Format format;

protected:
	friend class Reference;
	Data(Format fmt = fmt_none) : format(fmt) {}

	friend class GarbadgeCollector;
	virtual ~Data() = default;
};

}

#endif // DATA_H
