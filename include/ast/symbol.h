#ifndef SYMBOL_H
#define SYMBOL_H

#include <config.h>

#include <string>
#include <cstring>

namespace mint {

class MINT_EXPORT Symbol {
public:
	using hash_t = std::size_t;

#if defined (__GNUC__) && !defined (__clang__)
	constexpr
#endif
	Symbol(const char *symbol) :
		m_size(strlen(symbol)),
		m_hash(make_symbol_hash_constexpr(symbol, m_size)),
		m_symbol(strdup(symbol)) {

	}

	explicit Symbol(const std::string& symbol);
	Symbol(const Symbol &other);
	Symbol(Symbol &&other) noexcept;
	~Symbol();

	Symbol &operator =(const Symbol &other);
	Symbol &operator =(Symbol &&other) noexcept;

	inline bool operator ==(const Symbol &other) const;
	inline bool operator !=(const Symbol &other) const;

	inline hash_t hash() const;
	inline std::string str() const;

	static const Symbol MoveOperator;
	static const Symbol CopyOperator;
	static const Symbol CallOperator;
	static const Symbol AddOperator;
	static const Symbol SubOperator;
	static const Symbol MulOperator;
	static const Symbol DivOperator;
	static const Symbol PowOperator;
	static const Symbol ModOperator;
	static const Symbol InOperator;
	static const Symbol IsOperator;
	static const Symbol EqOperator;
	static const Symbol NeOperator;
	static const Symbol LtOperator;
	static const Symbol GtOperator;
	static const Symbol LeOperator;
	static const Symbol GeOperator;
	static const Symbol AndOperator;
	static const Symbol OrOperator;
	static const Symbol BandOperator;
	static const Symbol BorOperator;
	static const Symbol XorOperator;
	static const Symbol IncOperator;
	static const Symbol DecOperator;
	static const Symbol NotOperator;
	static const Symbol ComplOperator;
	static const Symbol ShiftLeftOperator;
	static const Symbol ShiftRightOperator;
	static const Symbol InclusiveRangeOperator;
	static const Symbol ExclusiveRangeOperator;
	static const Symbol TypeofOperator;
	static const Symbol MembersofOperator;
	static const Symbol SubscriptOperator;
	static const Symbol SubscriptMoveOperator;
	static const Symbol RegexMatchOperator;
	static const Symbol RegexUnmatchOperator;
	static const Symbol New;
	static const Symbol Delete;
	static const Symbol Write;
	static const Symbol Show;

private:
#if !defined (__x86_64__) && !defined (_WIN64)
	static constexpr const hash_t fnv_prime = 16777619u;
	static constexpr const hash_t offset_basis = 2166136261u;
#else
	static constexpr const hash_t fnv_prime = 1099511628211u;
	static constexpr const hash_t offset_basis = 14695981039346656037u;
#endif

	static constexpr hash_t make_symbol_hash_constexpr(const char *symbol, std::size_t length) {
		return make_symbol_hash_next(symbol, length, offset_basis, 0);
	}

	static constexpr hash_t make_symbol_hash_next(const char *symbol, std::size_t length, hash_t hash, std::size_t i) {
		return (i < length) ? make_symbol_hash_next(symbol, length, (hash * fnv_prime) ^ static_cast<hash_t>(symbol[i]), i + 1) : hash;
	}

	static hash_t make_symbol_hash(const char *symbol, std::size_t length);

	const size_t m_size;
	const hash_t m_hash;
	const char *m_symbol;
};

Symbol::hash_t Symbol::hash() const {
	return m_hash;
}

std::string Symbol::str() const {
	return std::string(m_symbol, m_size);
}

bool Symbol::operator ==(const Symbol &other) const {
	return LIKELY((m_size == other.m_size) && !memcmp(m_symbol, other.m_symbol, m_size));
}

bool Symbol::operator !=(const Symbol &other) const {
	return UNLIKELY((m_size != other.m_size) || memcmp(m_symbol, other.m_symbol, m_size));
}

}

template <>
struct std::hash<mint::Symbol> {
	std::size_t operator()(const mint::Symbol &k) const {
		return k.hash();
	}
};

#endif // SYMBOL_H
