#ifndef ITERATOR_H
#define ITERATOR_H

#include "memory/class.h"
#include "memory/object.h"

namespace mint {

class MINT_EXPORT IteratorClass : public Class {
public:
	static IteratorClass *instance();

private:
	IteratorClass();
};

struct MINT_EXPORT Iterator : public Object {
	Iterator();
	Iterator(double begin, double end);

	static Reference *fromInclusiveRange(double begin, double end);
	static Reference *fromExclusiveRange(double begin, double end);

	class ctx_type {
		ctx_type(const ctx_type &) = delete;
		ctx_type &operator =(const ctx_type &) = delete;
	public:
		typedef SharedReference value_type;

		class iterator {
		public:
			class data {
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

		class data {
		public:
			virtual ~data() = default;

			virtual iterator::data *begin() = 0;
			virtual iterator::data *end() = 0;

			virtual const value_type &front() const = 0;
			virtual const value_type &back() const = 0;

			virtual void push_front(const value_type &value) = 0;
			virtual void push_back(const value_type &value) = 0;

			virtual void pop_front() = 0;
			virtual void pop_back() = 0;

			virtual void clear() = 0;

			virtual size_t size() const = 0;
			virtual bool empty() const = 0;
		};

		ctx_type();
		ctx_type(double begin, double end);
		~ctx_type();

		iterator begin() const;
		iterator end() const;

		const value_type &front() const;
		const value_type &back() const;

		void push_front(const value_type &value);
		void push_back(const value_type &value);

		void pop_front();
		void pop_back();

		void clear();

		size_t size() const;
		bool empty() const;

	private:
		data *m_data;
	};

	ctx_type ctx;
	Reference ref;
};

}

#endif // ITERATOR_H
