#include "ast/symbol.h"

using namespace mint;
using namespace std;

const Symbol Symbol::MoveOperator = Symbol("=");
const Symbol Symbol::CopyOperator = Symbol(":=");
const Symbol Symbol::CallOperator = Symbol("()");
const Symbol Symbol::AddOperator = Symbol("+");
const Symbol Symbol::SubOperator = Symbol("-");
const Symbol Symbol::MulOperator = Symbol("*");
const Symbol Symbol::DivOperator = Symbol("/");
const Symbol Symbol::PowOperator = Symbol("**");
const Symbol Symbol::ModOperator = Symbol("%");
const Symbol Symbol::InOperator = Symbol("in");
const Symbol Symbol::IsOperator = Symbol("is");
const Symbol Symbol::EqOperator = Symbol("==");
const Symbol Symbol::NeOperator = Symbol("!=");
const Symbol Symbol::LtOperator = Symbol("<");
const Symbol Symbol::GtOperator = Symbol(">");
const Symbol Symbol::LeOperator = Symbol("<=");
const Symbol Symbol::GeOperator = Symbol(">=");
const Symbol Symbol::AndOperator = Symbol("&&");
const Symbol Symbol::OrOperator = Symbol("||");
const Symbol Symbol::BandOperator = Symbol("&");
const Symbol Symbol::BorOperator = Symbol("|");
const Symbol Symbol::XorOperator = Symbol("^");
const Symbol Symbol::IncOperator = Symbol("++");
const Symbol Symbol::DecOperator = Symbol("--");
const Symbol Symbol::NotOperator = Symbol("!");
const Symbol Symbol::ComplOperator = Symbol("~");
const Symbol Symbol::ShiftLeftOperator = Symbol("<<");
const Symbol Symbol::ShiftRightOperator = Symbol(">>");
const Symbol Symbol::InclusiveRangeOperator = Symbol("..");
const Symbol Symbol::ExclusiveRangeOperator = Symbol("...");
const Symbol Symbol::TypeofOperator = Symbol("typeof");
const Symbol Symbol::MembersofOperator = Symbol("membersof");
const Symbol Symbol::SubscriptOperator = Symbol("[]");
const Symbol Symbol::SubscriptMoveOperator = Symbol("[]=");
const Symbol Symbol::RegexMatchOperator = Symbol("=~");
const Symbol Symbol::RegexUnmatchOperator = Symbol("!~");
const Symbol Symbol::New = Symbol("new");
const Symbol Symbol::Delete = Symbol("delete");
const Symbol Symbol::Write = Symbol("write");
const Symbol Symbol::Show = Symbol("show");

Symbol::Symbol(const string& symbol) :
	m_size(symbol.size()),
	m_hash(make_symbol_hash(symbol.data(), m_size)),
	m_symbol(strdup(symbol.data())) {

}

Symbol::Symbol(const Symbol &other) :
	m_size(other.m_size),
	m_hash(other.m_hash),
	m_symbol(strdup(other.m_symbol)) {

}

Symbol::Symbol(Symbol &&other) noexcept :
	m_size(other.m_size),
	m_hash(other.m_hash),
	m_symbol(other.m_symbol) {
	other.m_symbol = nullptr;
}

Symbol::~Symbol() {
	free(const_cast<char *>(m_symbol));
}

Symbol &Symbol::operator =(const Symbol &other) {
	m_symbol = static_cast<const char *>(realloc(const_cast<char *>(m_symbol), other.m_size + 1));
	memcpy(const_cast<char *>(m_symbol), other.m_symbol, other.m_size + 1);
	const_cast<size_t &>(m_size) = other.m_size;
	const_cast<hash_t &>(m_hash) = other.m_hash;
	return *this;
}

Symbol &Symbol::operator =(Symbol &&other) noexcept {
	const_cast<size_t &>(m_size) = other.m_size;
	std::swap(m_symbol, other.m_symbol);
	const_cast<hash_t &>(m_hash) = other.m_hash;
	return *this;
}

Symbol::hash_t Symbol::make_symbol_hash(const char *symbol, size_t length) {

	size_t hash = offset_basis;

	for (size_t i = 0; i < length; ++i) {
		hash = hash * fnv_prime;
		hash = hash ^ static_cast<hash_t>(symbol[i]);
	}

	return hash;
}
