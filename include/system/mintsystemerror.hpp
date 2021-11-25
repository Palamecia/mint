#ifndef MINT_MINTSYSTEMERROR_HPP
#define MINT_MINTSYSTEMERROR_HPP

#include <exception>

namespace mint {

class MintSystemError : public std::exception {
public:
	MintSystemError() = default;

	const char *what() const noexcept override {
		return "MintSystemError";
	}
};

}

#endif // MINT_MINTSYSTEMERROR_HPP
