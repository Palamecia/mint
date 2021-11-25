#ifndef MINT_OPTIONAL_HPP
#define MINT_OPTIONAL_HPP

#include <type_traits>
#include <memory>

namespace mint {

struct nullopt_t {
	enum class _Construct { _Token };
	explicit constexpr nullopt_t(_Construct) {}
};

constexpr nullopt_t nullopt(nullopt_t::_Construct::_Token);

template<class Type>
class optional {
public:
	using value_type = Type;

#if !defined(_MSC_VER) || _MSC_VER > 1900
	constexpr
#endif
	optional(nullopt_t) noexcept :
	    m_empty() {

	}

	template<typename... Args>
	optional(Args&&... args) {
		construct(std::forward<Args>(args)...);
	}

	template<class Other>
	optional(const optional<Other> &other) {
		if (other.m_engaged) {
			construct(std::forward<Other>(other.m_payload));
		}
	}

	template<class Other>
	optional(optional<Other> &&other) noexcept {
		if (other.m_engaged) {
			construct(std::move(other.m_payload));
		}
	}

	~optional() {
		reset();
	}

	optional &operator =(nullopt_t) noexcept {
		reset();
		return *this;
	}

	template<class Other = Type>
	optional &operator =(Other &&value) {
		reset();
		construct(std::forward<Other>(value));
		return *this;
	}

	template<class Other = Type>
	optional &operator =(const optional<Other> &other) {
		reset();
		if (other.m_engaged) {
			construct(std::forward<Other>(other.m_payload));
		}
		return *this;
	}

	template<class Other = Type>
	optional &operator =(optional<Other> &&other) noexcept {
		if (other.m_engaged) {
			if (m_engaged) {
				m_payload = std::move(other.m_payload);
			}
			else {
				construct(std::move(other.m_payload));
			}
		}
		else {
			reset();
		}
		return *this;
	}

	constexpr const value_type *operator ->() const noexcept {
		return &m_payload;
	}

	value_type *operator ->() noexcept {
		return &m_payload;
	}

	constexpr const value_type &operator *() const &noexcept {
		return m_payload;
	}

	value_type &operator *() & noexcept {
		return m_payload;
	}

	constexpr const value_type &&operator *() const &&noexcept {
		return std::move(m_payload);
	}

	value_type &&operator *() &&noexcept {
		return std::move(m_payload);
	}

	constexpr explicit operator bool() const noexcept {
		return m_engaged;
	}

	constexpr bool has_value() const noexcept {
		return m_engaged;
	}

private:
	using stored_type_t = typename std::remove_const<value_type>::type;
	struct empty_byte_t {};

	template<typename... Args>
	void construct(Args&&... args) noexcept {
		new (&m_payload) stored_type_t(std::forward<Args>(args)...);
		m_engaged = true;
	}

	void destruct() {
		m_engaged = false;
		m_payload.~stored_type_t();
	}

	void reset() {
		if (m_engaged) {
			destruct();
		}
	}

	union {
		stored_type_t m_payload;
		empty_byte_t m_empty;
	};

	bool m_engaged = false;
};

}

#endif // MINT_OPTIONAL_HPP
