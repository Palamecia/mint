#ifndef NTPROCESSINFO_H
#define NTPROCESSINFO_H

#include <Windows.h>

// Unicode path usually prefix with '\\?\'
#define MAX_UNICODE_PATH	32767L

namespace mint {

typedef struct _PROCESSINFO {
	DWORD	dwPID;
	DWORD	dwParentPID;
	DWORD	dwSessionID;
	DWORD	dwPEBBaseAddress;
	DWORD	dwAffinityMask;
	LONG	dwBasePriority;
	LONG	dwExitStatus;
	BYTE	cBeingDebugged;
	WCHAR	szImgPath[MAX_UNICODE_PATH];
	WCHAR	szCmdLine[MAX_UNICODE_PATH];
	WCHAR	szCurrentDirectoryPath[MAX_UNICODE_PATH];
	WCHAR	szEnvironment[MAX_UNICODE_PATH];
} PROCESSINFO;

BOOL EnableTokenPrivilege(IN LPCTSTR pszPrivilege);
BOOL GetNtProcessInfo(IN const DWORD dwPID, OUT PROCESSINFO *ppi);

LPWSTR GetNtProcessCommandLine(HANDLE hProcess);
DWORD GetNtProcessCurrentDirectory(HANDLE hProcess, LPWSTR lpCurrentDirectory, DWORD nSize);
LPWCH GetNtProcessEnvironmentStrings(HANDLE hProcess);

}

#endif // NTPROCESSINFO_H
