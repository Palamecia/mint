#ifndef MINT_CONFIG_H
#define MINT_CONFIG_H

#ifndef MINT_NO_BYTE_TYPE
using byte_t = unsigned char;
#endif

#define MINT_TO_STR(__str) #__str
#define MINT_MACRO_TO_STR(__str) MINT_TO_STR(__str)

#if defined(_WIN32)
#define OS_WINDOWS
#ifdef _WIN64
#define OS_WIN_64
#else
#define OS_WIN_32
#endif
#elif defined(__APPLE__)
#define OS_MAC
#define OS_UNIX
#elif defined(__linux__)
#define OS_LINUX
#define OS_UNIX
#elif defined(__unix__)
#define OS_UNIX
#endif

#if !defined(NDEBUG) || defined(_DEBUG)
#define BUILD_TYPE_DEBUG
#else
#define BUILD_TYPE_RELEASE
#endif

#ifdef OS_WINDOWS

#define DECL_IMPORT __declspec(dllimport)
#define DECL_EXPORT __declspec(dllexport)

#pragma warning(disable: 4251)
#define _UNICODE
#define UNICODE

#define __attribute__(ignore)

#define LIKELY(expr) (expr)
#define UNLIKELY(expr) (expr)

#ifdef BUILD_MINT_LIB
#define MINT_EXPORT DECL_EXPORT
#else
#define MINT_EXPORT DECL_IMPORT
#endif
#else
#define DECL_IMPORT
#define DECL_EXPORT
#define MINT_EXPORT

#define LIKELY(expr) __builtin_expect(!!(expr), true)
#define UNLIKELY(expr) __builtin_expect(!!(expr), false)
#endif

#endif // MINT_CONFIG_H
