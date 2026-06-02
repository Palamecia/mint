#ifndef MINT_CONFIG_H
#define MINT_CONFIG_H

#define MINT_TO_STR(__str) #__str
#define MINT_MACRO_TO_STR(__str) MINT_TO_STR(__str)

#ifdef _WIN32
#define MINT_OS_WINDOWS
#ifdef _WIN64
#define MINT_OS_WIN_64
#else
#define MINT_OS_WIN_32
#endif
#elifdef __APPLE__
#define MINT_OS_MAC
#define MINT_OS_UNIX
#elifdef __linux__
#define MINT_OS_LINUX
#define MINT_OS_UNIX
#elifdef __unix__
#define MINT_OS_UNIX
#endif

#if !defined(NDEBUG) || defined(_DEBUG)
#define MINT_BUILD_TYPE_DEBUG
#else
#define MINT_BUILD_TYPE_RELEASE
#endif

#ifdef MINT_OS_WINDOWS

#define MINT_DECL_IMPORT __declspec(dllimport)
#define MINT_DECL_EXPORT __declspec(dllexport)

#pragma warning(disable: 4251)
#define _UNICODE
#define UNICODE

#ifdef BUILD_MINT_LIB
#define MINT_EXPORT MINT_DECL_EXPORT
#else
#define MINT_EXPORT MINT_DECL_IMPORT
#endif
#else
#define MINT_DECL_IMPORT
#define MINT_DECL_EXPORT
#define MINT_EXPORT
#endif

namespace mint {

template<class... Ts>
struct Overloaded : Ts... {
	using Ts::operator()...;
};

}

#endif // MINT_CONFIG_H
