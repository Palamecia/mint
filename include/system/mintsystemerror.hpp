#ifndef MINT_SYSTEM_ERROR_HPP
#define MINT_SYSTEM_ERROR_HPP

#include <exception>

namespace mint {

class MintSystemError : public std::exception {
public:
    MintSystemError() = default;
};

}

#endif // MINT_SYSTEM_ERROR_HPP
