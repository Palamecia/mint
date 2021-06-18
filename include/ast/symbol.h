#ifndef SYMBOL_H
#define SYMBOL_H

#include <config.h>

#include <string>
#include <cstring>

namespace mint {

class MINT_EXPORT Symbol {
	friend struct std::hash<mint::Symbol>;
public:
#ifdef OS_UNIX
	constexpr
#endif
	Symbol(const char *symbol) :
		m_hash(make_symbol_hash_constexpr(symbol, strlen(symbol))),
		m_symbol(strdup(symbol)) {

	}

	explicit Symbol(const std::string& symbol);
	Symbol(const Symbol &other);
	Symbol(Symbol &&other);
	~Symbol();

	Symbol &operator =(const Symbol &other) = delete;
	Symbol &operator =(Symbol &&other) = delete;

	bool operator ==(const Symbol &other) const;
	bool operator !=(const Symbol &other) const;
	bool operator <(const Symbol &other) const;

	std::string str() const;

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
	static constexpr const uint64_t fnv_prime = 1099511628211;
	static constexpr const uint64_t offset_basis = 14695981039346656037u;

	static constexpr uint64_t make_symbol_hash_constexpr(const char *symbol, size_t length) {
		return make_symbol_hash_next(symbol, length, offset_basis, 0);
	}

	static constexpr uint64_t make_symbol_hash_next(const char *symbol, size_t length, uint64_t hash, size_t i) {
		return (i < length) ? make_symbol_hash_next(symbol, length, (hash * fnv_prime) ^ symbol[i], i + 1) : hash;
	}

	static uint64_t make_symbol_hash(const char *symbol, size_t length);

	const uint64_t m_hash;
	const char *m_symbol;
};

}

template <>
struct std::hash<mint::Symbol> {
	std::size_t operator()(const mint::Symbol &k) const {
		return k.m_hash;
	}
};

#endif // SYMBOL_H
