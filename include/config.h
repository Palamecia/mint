#ifndef MINT_CONFIG_H
#define MINT_CONFIG_H

#ifdef _WIN32

#define DECL_IMPORT __declspec(dllimport)
#define DECL_EXPORT __declspec(dllexport)

#ifdef BUILD_MINT_LIB
#define MINT_EXPORT DECL_EXPORT
#else
#define MINT_EXPORT DECL_IMPORT
#endif
#else
#define MINT_EXPORT
#endif

#endif // MINT_CONFIG_H
