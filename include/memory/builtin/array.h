#ifndef ARRAY_H
#define ARRAY_H

#include "memory/class.h"
#include "memory/object.h"

namespace mint {

class MINT_EXPORT ArrayClass : public Class {
public:
	static ArrayClass *instance();

private:
	ArrayClass();
};

struct MINT_EXPORT Array : public Object {
	Array();
	Array(const Array &other) = delete;
	~Array();

	Array &operator =(const Array &other) = delete;

	void mark() override;

	using values_type = std::vector<SharedReference>;
	values_type values;
};

}

#endif // ARRAY_H
