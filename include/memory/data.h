#ifndef DATA_H
#define DATA_H

#include <config.h>

#include <cstddef>

namespace mint {

struct MemoryInfos {
	bool reachable = true;
	bool collected = false;
	size_t refcount = 0;
};

struct MINT_EXPORT Data {
	enum Format {
		fmt_none,
		fmt_null,
		fmt_number,
		fmt_boolean,
		fmt_object,
		fmt_package,
		fmt_function
	};
	const Format format;

	virtual void mark();

protected:
	friend class Reference;
	friend class GarbageCollector;

	Data(Format fmt);
	virtual ~Data() = default;

	bool markedBit() const;

private:
	MemoryInfos infos;
	Data *prev = nullptr;
	Data *next = nullptr;
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
