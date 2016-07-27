#ifndef UTF_8_ITERATOR_H
#define UTF_8_ITERATOR_H

#include <iterator>
#include <string>

template<class iterator_type>
class abstract_utf8iterator : public std::iterator<std::random_access_iterator_tag, std::string> {
public:
	abstract_utf8iterator(const iterator_type &it) : m_data(it) {}

	virtual ~abstract_utf8iterator() {}

	abstract_utf8iterator<iterator_type> &operator =(const iterator_type &it) {
		m_data = it;
		return *this;
	}

	abstract_utf8iterator<iterator_type> &operator ++() {

		if ((*m_data & (1 << 7))) {
			if ((*m_data & (1 << 5))) {
				if ((*m_data & (1 << 4))) {
					m_data += 4;
				}
				else {
					m_data += 3;
				}
			}
			else {
				m_data += 2;
			}
		}
		else {
			m_data += 1;
		}

		return *this;
	}

	abstract_utf8iterator<iterator_type> &operator --() {

		do {
			m_data--;
		}
		while ((*m_data & (1 << 7)) && !(*m_data & (1 << 6)));
	}

	auto operator ++(int) -> decltype(*this) {

		decltype(*this) other(*this);
		operator ++();
		return other;
	}

	auto operator --(int) -> decltype(*this) {

		decltype(*this) other(*this);
		operator --();
		return other;
	}

	auto operator +(size_t offset) -> decltype(*this) {

		decltype(*this) other(*this);
		for (size_t i = 0; i < offset; ++i) {
			other++;
		}
		return other;
	}

	auto operator -(size_t offset) -> decltype(*this) {

		decltype(*this) other(*this);
		for (size_t i = 0; i < offset; ++i) {
			other--;
		}
		return other;
	}

	bool operator !=(const abstract_utf8iterator<iterator_type> &other) {
		return m_data != other.m_data;
	}

	std::string operator *() {

		std::string str(1, *m_data);

		if ((*m_data & (1 << 7))) {
			str += *(m_data + 1);
			if ((*m_data & (1 << 5))) {
				str += *(m_data + 2);
				if ((*m_data & (1 << 4))) {
					str += *(m_data + 3);
				}
			}
		}

		return str;
	}

protected:
	iterator_type m_data;
};

typedef abstract_utf8iterator<std::string::iterator> utf8iterator;

typedef abstract_utf8iterator<std::string::const_iterator> const_utf8iterator;

size_t utf8length(const std::string &str);

#endif // UTF_8_ITERATOR_H
