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
	friend class GarbadgeCollector;
	friend class Reference;
	Data(Format fmt) : format(fmt) {}
	virtual ~Data() = default;
};

struct MINT_EXPORT None : public Data {
protected:
	friend class Reference;
	None();
};

struct MINT_EXPORT Null : public Data {
protected:
	friend class Reference;
	Null();
};

}

#endif // DATA_H
