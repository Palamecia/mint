#ifndef MINT_ITERATOR_H
#define MINT_ITERATOR_H

#include "memory/class.h"
#include "memory/object.h"
#include "system/optional.hpp"

namespace _mint_iterator{
class data;
class data_iterator;
}

namespace mint {

class Cursor;

class MINT_EXPORT IteratorClass : public Class {
public:
	static IteratorClass *instance();

private:
	friend class GlobalData;
	IteratorClass();
};

struct MINT_EXPORT Iterator : public Object {
	Iterator();
	Iterator(Reference &ref);
	Iterator(const Iterator &other);
	Iterator(double begin, double end);
	Iterator(size_t stack_size);

	void mark() override;

	static WeakReference fromInclusiveRange(double begin, double end);
	static WeakReference fromExclusiveRange(double begin, double end);

	class MINT_EXPORT ctx_type {
	public:
		enum type { items, range, generator };
		using value_type = Reference;

		class MINT_EXPORT iterator {
		public:
			iterator(_mint_iterator::data_iterator *data);
			iterator(const iterator &other);
			iterator(iterator &&other);
			~iterator();

			iterator &operator =(const iterator &other);
			iterator &operator =(iterator &&other);

			bool operator ==(const iterator &other) const;
			bool operator !=(const iterator &other) const;

			value_type &operator *();
			value_type *operator ->();

			iterator operator ++(int);
			iterator &operator ++();

		private:
			std::unique_ptr<_mint_iterator::data_iterator> m_data;
		};

		ctx_type(_mint_iterator::data *data);
		ctx_type(const ctx_type &other);
		~ctx_type();

		ctx_type &operator =(const ctx_type &other);

		void mark();

		type getType() const;

		iterator begin() const;
		iterator end() const;

		value_type &front();
		value_type &back();

		void emplace_front(value_type &&value);
		void emplace_back(value_type &&value);

		void pop_front();
		void pop_back();

		void finalize();
		void clear();

		size_t size() const;
		bool empty() const;

	private:
		std::unique_ptr<_mint_iterator::data> m_data;
	};

	ctx_type ctx;

private:
	friend class Reference;
	static LocalPool<Iterator> g_pool;
};

MINT_EXPORT void iterator_init_from_stack(Cursor *cursor, size_t length);
MINT_EXPORT void iterator_finalize(Cursor *cursor, int signature, int limit);
MINT_EXPORT Iterator *iterator_init(Reference &ref);
MINT_EXPORT Iterator *iterator_init(Reference &&ref);
MINT_EXPORT void iterator_insert(Iterator *iterator, Reference &&item);
MINT_EXPORT void iterator_set_next(Iterator *iterator, Reference &&item);
MINT_EXPORT optional<WeakReference> iterator_get(Iterator *iterator);
MINT_EXPORT optional<WeakReference> iterator_next(Iterator *iterator);

}

#endif // MINT_ITERATOR_H
