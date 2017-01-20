#ifndef UTF_8_ITERATOR_H
#define UTF_8_ITERATOR_H

#include <iterator>
#include <string>

bool utf8char_valid(unsigned char c);
size_t utf8char_length(unsigned char c);
size_t utf8length(const std::string &str);

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
		m_data += utf8char_length(*m_data);
		return *this;
	}

	abstract_utf8iterator<iterator_type> &operator --() {

		do {
			m_data--;
		}
		while (!utf8char_valid(*m_data));
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

		std::string str;

		for (size_t i = 0; i < utf8char_length(*m_data); ++i) {
			str += *(m_data + i);
		}

		return str;
	}

protected:
	iterator_type m_data;
};

typedef abstract_utf8iterator<std::string::iterator> utf8iterator;

typedef abstract_utf8iterator<std::string::const_iterator> const_utf8iterator;

#endif // UTF_8_ITERATOR_H
