#ifndef ITERATOR_H
#define ITERATOR_H

#include "memory/class.h"
#include "memory/object.h"

namespace mint {

class Cursor;

class MINT_EXPORT IteratorClass : public Class {
public:
	static IteratorClass *instance();

private:
	IteratorClass();
};

struct MINT_EXPORT Iterator : public Object {
	Iterator();
	Iterator(double begin, double end);
	Iterator(Cursor *cursor, size_t stack_size);

	void mark() override;

	static Reference *fromInclusiveRange(double begin, double end);
	static Reference *fromExclusiveRange(double begin, double end);

	class MINT_EXPORT ctx_type {
		ctx_type(const ctx_type &) = delete;
		ctx_type &operator =(const ctx_type &) = delete;
	public:
		enum type { items, range, generator };
		using value_type = SharedReference;

		class MINT_EXPORT iterator {
		public:
			class MINT_EXPORT data {
			public:
				virtual ~data() = default;

				virtual bool compare(data *other) const = 0;
				virtual value_type &get() = 0;
				virtual data *next() = 0;
			};

			iterator(data *data);

			bool operator !=(const iterator &other) const;
			value_type &operator *();
			iterator operator ++();

		private:
			std::shared_ptr<data> m_data;
		};

		class MINT_EXPORT data {
		public:
			virtual ~data() = default;

			virtual void mark() = 0;

			virtual ctx_type::type getType() = 0;

			virtual iterator::data *begin() = 0;
			virtual iterator::data *end() = 0;

			virtual value_type &front() = 0;
			virtual value_type &back() = 0;

			virtual void emplace_front(value_type &value) = 0;
			virtual void emplace_back(value_type &value) = 0;

			virtual void pop_front() = 0;
			virtual void pop_back() = 0;

			virtual void finalize() {}
			virtual void clear() = 0;

			virtual size_t size() const = 0;
			virtual bool empty() const = 0;
		};

		ctx_type();
		ctx_type(double begin, double end);
		ctx_type(Cursor *cursor, size_t stack_size);
		~ctx_type();

		void mark();

		type getType() const;

		iterator begin() const;
		iterator end() const;

		value_type &front();
		value_type &back();

		void emplace_front(value_type &value);
		void emplace_back(value_type &value);

		void pop_front();
		void pop_back();

		void finalize();
		void clear();

		size_t size() const;
		bool empty() const;

	private:
		data *m_data;
	};

	ctx_type ctx;
};

}

#endif // ITERATOR_H
