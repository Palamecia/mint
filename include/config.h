#ifndef MINT_CONFIG_H
#define MINT_CONFIG_H

#ifndef MINT_NO_BYTE_TYPE
typedef unsigned char byte;
#endif

#define MINT_TO_STR(__str) #__str
#define MINT_MACRO_TO_STR(__str) MINT_TO_STR(__str)

#ifdef _WIN32
#define OS_WINDOWS
#ifdef _WIN64
#define OS_WIN_64
#else
#define OS_WIN_32
#endif
#else
#define OS_UNIX
#endif

#ifdef OS_WINDOWS

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
