#ifndef UTF_8_ITERATOR_H
#define UTF_8_ITERATOR_H

#include "config.h"

#include <iterator>
#include <string>

namespace mint {

MINT_EXPORT bool utf8char_valid(byte b);
MINT_EXPORT size_t utf8char_length(byte b);
MINT_EXPORT size_t utf8length(const std::string &str);

MINT_EXPORT std::string::size_type utf8_byte_index_to_pos(const std::string &str, std::string::difference_type index);
MINT_EXPORT std::string::size_type utf8_byte_index_to_pos(const std::string &str, std::string::size_type index);
MINT_EXPORT size_t utf8_pos_to_byte_index(const std::string &str, std::string::size_type pos);

template<class iterator_type>
class basic_utf8iterator : public std::iterator<std::random_access_iterator_tag, std::string> {
public:
	basic_utf8iterator(const iterator_type &it) : m_data(it) {

	}

	virtual ~basic_utf8iterator() = default;

	basic_utf8iterator<iterator_type> &operator =(const iterator_type &it) {
		m_data = it;
		return *this;
	}

	basic_utf8iterator<iterator_type> &operator ++() {
		m_data += static_cast<typename iterator_type::difference_type>(utf8char_length(static_cast<byte>(*m_data)));
		return *this;
	}

	basic_utf8iterator<iterator_type> &operator --() {

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

	bool operator !=(const basic_utf8iterator<iterator_type> &other) const {
		return m_data != other.m_data;
	}

	std::string operator *() {

		std::string str;

		for (size_t i = 0; i < utf8char_length(static_cast<byte>(*m_data)); ++i) {
			str += static_cast<typename iterator_type::reference>(*std::next(m_data, static_cast<typename iterator_type::difference_type>(i)));
		}

		return str;
	}

protected:
	iterator_type m_data;
};

typedef basic_utf8iterator<std::string::iterator> utf8iterator;
typedef basic_utf8iterator<std::string::const_iterator> const_utf8iterator;

}

#endif // UTF_8_ITERATOR_H
