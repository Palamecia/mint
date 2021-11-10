#ifndef ARRAY_H
#define ARRAY_H

#include "memory/class.h"
#include "memory/object.h"

namespace mint {

class Cursor;

class MINT_EXPORT ArrayClass : public Class {
public:
	static ArrayClass *instance();

private:
	ArrayClass();
};

struct MINT_EXPORT Array : public Object {
	Array();
	Array(const Array &other);

	Array &operator =(const Array &other) = delete;

	void mark() override;

	using values_type = std::vector<WeakReference>;
	values_type values;

private:
	friend class Reference;
	static LocalPool<Array> g_pool;
};

MINT_EXPORT void array_append_from_stack(Cursor *cursor);
MINT_EXPORT void array_append(Array *array, Reference &item);
MINT_EXPORT void array_append(Array *array, Reference &&item);
MINT_EXPORT WeakReference array_get_item(Array *array, intmax_t index);
MINT_EXPORT WeakReference array_get_item(Array::values_type::iterator &it);
MINT_EXPORT WeakReference array_get_item(Array::values_type::value_type &value);
MINT_EXPORT size_t array_index(const Array *array, intmax_t index);
MINT_EXPORT WeakReference array_item(const Reference &item);

}

#endif // ARRAY_H
