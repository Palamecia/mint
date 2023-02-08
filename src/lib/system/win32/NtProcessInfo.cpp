/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "NtProcessInfo.h"

#include <ntstatus.h>
#include <winternl.h>
#include <psapi.h>

#define STRSAFE_LIB
#include <strsafe.h>

#pragma comment(lib, "strsafe.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "psapi.lib")

#ifndef NTSTATUS
#define LONG NTSTATUS
#endif

namespace mint {

// Used in PEB struct
typedef ULONG PPS_POST_PROCESS_INIT_ROUTINE;

// Used in PEB struct
typedef struct _PEB_LDR_DATA {
	ULONG          Length;
	BOOLEAN        Initialized;
	HANDLE         SsHandle;
	LIST_ENTRY     LoadOrder;
	LIST_ENTRY     MemoryOrder;
	LIST_ENTRY     InitializationOrder;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

// Used in PEB struct
typedef struct _CURDIR {
	UNICODE_STRING Path;
	HANDLE Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	ULONG               MaximumLength;
	ULONG               Length;
	ULONG               Flags;
	ULONG               DebugFlags;
	HANDLE              ConsoleHandle;
	ULONG               ConsoleFlags;
	HANDLE              StdInputHandle;
	HANDLE              StdOuputHandle;
	HANDLE              StdErrorHandle;
	CURDIR              CurrentDirectory;
	UNICODE_STRING      DllPath;
	UNICODE_STRING      ImagePathName;
	UNICODE_STRING      CommandLine;
	PWSTR               Environment;
	ULONG               dwX;
	ULONG               dwY;
	ULONG               dwXSize;
	ULONG               dwYSize;
	ULONG               dwXCountChars;
	ULONG               dwYCountChars;
	ULONG               dwFillAttribute;
	ULONG               dwFlags;
	ULONG               wShowWindow;
	UNICODE_STRING      WindowTitle;
	UNICODE_STRING      Desktop;
	UNICODE_STRING      ShellInfo;
	UNICODE_STRING      RuntimeInfo;
	/// \todo RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

// Used in PROCESS_BASIC_INFORMATION struct
#define RTL_CONDITION_VARIABLE_INIT {0}
#define RTL_CONDITION_VARIABLE_LOCKMODE_SHARED 0x1

typedef struct _RTL_CONDITION_VARIABLE {
	PVOID Ptr;
} RTL_CONDITION_VARIABLE, *PRTL_CONDITION_VARIABLE;

typedef struct _RTL_CRITICAL_SECTION_DEBUG {
	WORD Type;
	WORD CreatorBackTraceIndex;
	struct _RTL_CRITICAL_SECTION *CriticalSection;
	LIST_ENTRY ProcessLocksList;
	DWORD EntryCount;
	DWORD ContentionCount;
	DWORD Flags;
	WORD CreatorBackTraceIndexHigh;
	WORD SpareWORD;
} RTL_CRITICAL_SECTION_DEBUG, *PRTL_CRITICAL_SECTION_DEBUG, RTL_RESOURCE_DEBUG, *PRTL_RESOURCE_DEBUG;

typedef struct _RTL_CRITICAL_SECTION {
	PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
	LONG LockCount;
	LONG RecursionCount;
	HANDLE OwningThread;
	HANDLE LockSemaphore;
	ULONG_PTR SpinCount;
} RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION;

typedef PVOID PPEBLOCKROUTINE;

typedef struct _PEB_FREE_BLOCK {
	struct _PEB_FREE_BLOCK *Next;
	ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

typedef struct tagRTL_BITMAP {
	ULONG  SizeOfBitMap; /* Number of bits in the bitmap */
	PULONG Buffer; /* Bitmap data, assumed sized to a DWORD boundary */
} RTL_BITMAP, *PRTL_BITMAP;

/* Doubly Linked Lists */
typedef struct _LIST_ENTRY {
	struct _LIST_ENTRY *Flink;
	struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY, *RESTRICTED_POINTER PRLIST_ENTRY;

/* win32/win64 */
typedef struct _PEB {
	BOOLEAN                      InheritedAddressSpace;             /* 000/000 */
	BOOLEAN                      ReadImageFileExecOptions;          /* 001/001 */
	BOOLEAN                      BeingDebugged;                     /* 002/002 */
	BOOLEAN                      SpareBool;                         /* 003/003 */
	HANDLE                       Mutant;                            /* 004/008 */
	HMODULE                      ImageBaseAddress;                  /* 008/010 */
	PPEB_LDR_DATA                LdrData;                           /* 00c/018 */
	RTL_USER_PROCESS_PARAMETERS* ProcessParameters;                 /* 010/020 */
	PVOID                        SubSystemData;                     /* 014/028 */
	HANDLE                       ProcessHeap;                       /* 018/030 */
	PRTL_CRITICAL_SECTION        FastPebLock;                       /* 01c/038 */
	PPEBLOCKROUTINE              FastPebLockRoutine;                /* 020/040 */
	PPEBLOCKROUTINE              FastPebUnlockRoutine;              /* 024/048 */
	ULONG                        EnvironmentUpdateCount;            /* 028/050 */
	PVOID                        KernelCallbackTable;               /* 02c/058 */
	ULONG                        Reserved[2];                       /* 030/060 */
	PPEB_FREE_BLOCK              FreeList;                          /* 038/068 */
	ULONG                        TlsExpansionCounter;               /* 03c/070 */
	PRTL_BITMAP                  TlsBitmap;                         /* 040/078 */
	ULONG                        TlsBitmapBits[2];                  /* 044/080 */
	PVOID                        ReadOnlySharedMemoryBase;          /* 04c/088 */
	PVOID                        ReadOnlySharedMemoryHeap;          /* 050/090 */
	PVOID*                       ReadOnlyStaticServerData;          /* 054/098 */
	PVOID                        AnsiCodePageData;                  /* 058/0a0 */
	PVOID                        OemCodePageData;                   /* 05c/0a8 */
	PVOID                        UnicodeCaseTableData;              /* 060/0b0 */
	ULONG                        NumberOfProcessors;                /* 064/0b8 */
	ULONG                        NtGlobalFlag;                      /* 068/0bc */
	LARGE_INTEGER                CriticalSectionTimeout;            /* 070/0c0 */
	SIZE_T                       HeapSegmentReserve;                /* 078/0c8 */
	SIZE_T                       HeapSegmentCommit;                 /* 07c/0d0 */
	SIZE_T                       HeapDeCommitTotalFreeThreshold;    /* 080/0d8 */
	SIZE_T                       HeapDeCommitFreeBlockThreshold;    /* 084/0e0 */
	ULONG                        NumberOfHeaps;                     /* 088/0e8 */
	ULONG                        MaximumNumberOfHeaps;              /* 08c/0ec */
	PVOID*                       ProcessHeaps;                      /* 090/0f0 */
	PVOID                        GdiSharedHandleTable;              /* 094/0f8 */
	PVOID                        ProcessStarterHelper;              /* 098/100 */
	PVOID                        GdiDCAttributeList;                /* 09c/108 */
	PVOID                        LoaderLock;                        /* 0a0/110 */
	ULONG                        OSMajorVersion;                    /* 0a4/118 */
	ULONG                        OSMinorVersion;                    /* 0a8/11c */
	ULONG                        OSBuildNumber;                     /* 0ac/120 */
	ULONG                        OSPlatformId;                      /* 0b0/124 */
	ULONG                        ImageSubSystem;                    /* 0b4/128 */
	ULONG                        ImageSubSystemMajorVersion;        /* 0b8/12c */
	ULONG                        ImageSubSystemMinorVersion;        /* 0bc/130 */
	ULONG                        ImageProcessAffinityMask;          /* 0c0/134 */
	HANDLE                       GdiHandleBuffer[28];               /* 0c4/138 */
	ULONG                        unknown[6];                        /* 134/218 */
	PVOID                        PostProcessInitRoutine;            /* 14c/230 */
	PRTL_BITMAP                  TlsExpansionBitmap;                /* 150/238 */
	ULONG                        TlsExpansionBitmapBits[32];        /* 154/240 */
	ULONG                        SessionId;                         /* 1d4/2c0 */
	ULARGE_INTEGER               AppCompatFlags;                    /* 1d8/2c8 */
	ULARGE_INTEGER               AppCompatFlagsUser;                /* 1e0/2d0 */
	PVOID                        ShimData;                          /* 1e8/2d8 */
	PVOID                        AppCompatInfo;                     /* 1ec/2e0 */
	UNICODE_STRING               CSDVersion;                        /* 1f0/2e8 */
	PVOID                        ActivationContextData;             /* 1f8/2f8 */
	PVOID                        ProcessAssemblyStorageMap;         /* 1fc/300 */
	PVOID                        SystemDefaultActivationData;       /* 200/308 */
	PVOID                        SystemAssemblyStorageMap;          /* 204/310 */
	SIZE_T                       MinimumStackCommit;                /* 208/318 */
	PVOID*                       FlsCallback;                       /* 20c/320 */
	LIST_ENTRY                   FlsListHead;                       /* 210/328 */
	PRTL_BITMAP                  FlsBitmap;                         /* 218/338 */
	ULONG                        FlsBitmapBits[4];                  /* 21c/340 */
} PEB, *PPEB;

// Used with NtQueryInformationProcess
typedef struct _PROCESS_BASIC_INFORMATION {
	LONG ExitStatus;
	PPEB PebBaseAddress;
	ULONG_PTR AffinityMask;
	LONG BasePriority;
	ULONG_PTR UniqueProcessId;
	ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

// NtQueryInformationProcess in NTDLL.DLL
typedef NTSTATUS(NTAPI *pfnNtQueryInformationProcess)(
	IN	HANDLE ProcessHandle,
	IN	PROCESSINFOCLASS ProcessInformationClass,
	OUT	PVOID ProcessInformation,
	IN	ULONG ProcessInformationLength,
	OUT	PULONG ReturnLength	OPTIONAL
	);

struct NtDllInfo {
	NtDllInfo(const char *name) {
		handle = LoadLibraryA(name);
	}

	~NtDllInfo() {
		if (handle) {
			FreeLibrary(handle);
		}
	}

	FARPROC operator()(const char *name) {
		if (handle) {
			return GetProcAddress(handle, name);
		}
		return NULL;
	}

protected:
	NtDllInfo(const NtDllInfo &other) = delete;
	NtDllInfo &operator =(const NtDllInfo &other) = delete;

private:
	HMODULE handle;
};

static NtDllInfo ntdll("ntdll.dll");
static pfnNtQueryInformationProcess NtQueryInformationProcess = (pfnNtQueryInformationProcess)ntdll("NtQueryInformationProcess");

}

BOOL mint::EnableTokenPrivilege(IN LPCTSTR pszPrivilege) {

	HANDLE hToken = 0;
	TOKEN_PRIVILEGES tkp = {0};
	bool bResult = false;

	// Get a token for this process.
	if (!OpenProcessToken(GetCurrentProcess(),
	                      TOKEN_ADJUST_PRIVILEGES |
	                      TOKEN_QUERY, &hToken)) {
		return FALSE;
	}

	// Get the LUID for the privilege.
	if (LookupPrivilegeValue(NULL, pszPrivilege,
	                         &tkp.Privileges[0].Luid)) {
		tkp.PrivilegeCount = 1;  // one privilege to set
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		// Set the privilege for this process.
		AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

		if (GetLastError() == ERROR_SUCCESS) {
			bResult = true;
		}

	}

	CloseHandle(hToken);
	return bResult;
}

static BOOL HasReadAccess(HANDLE hProcess, LPVOID pAddress, USHORT &nSize) {

	MEMORY_BASIC_INFORMATION memInfo;

	VirtualQueryEx(hProcess, pAddress, &memInfo, sizeof(memInfo));

	if (PAGE_NOACCESS == memInfo.Protect || PAGE_EXECUTE == memInfo.Protect) {
		nSize = 0;
		return FALSE;
	}

	nSize = memInfo.RegionSize;
	return TRUE;
}

NTSTATUS LoadNtProcessBasicInformation(HANDLE hProcess, mint::PPROCESS_BASIC_INFORMATION *pbi, DWORD *dwSize) {

	DWORD dwSizeNeeded = 0;
	HANDLE hHeap = 0;

	// Try to allocate buffer
	hHeap = GetProcessHeap();
	*dwSize = sizeof(mint::PROCESS_BASIC_INFORMATION);
	*pbi = (mint::PPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, *dwSize);

	// Did we successfully allocate memory
	if (!*pbi) {
		CloseHandle(hProcess);
		return STATUS_NO_MEMORY;
	}

	// Attempt to get basic info on process
	NTSTATUS dwStatus = mint::NtQueryInformationProcess(hProcess, ProcessBasicInformation, *pbi, *dwSize, &dwSizeNeeded);

	// If we had error and buffer was too all, try again
	// with larger buffer size (dwSizeNeeded)
	if (dwStatus >= 0 && *dwSize < dwSizeNeeded) {
		if (*pbi) {
			HeapFree(hHeap, 0, *pbi);
		}

		*dwSize = dwSizeNeeded;
		*pbi = (mint::PPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, *dwSize);

		if (!*pbi) {
			CloseHandle(hProcess);
			return STATUS_NO_MEMORY;
		}

		dwStatus = mint::NtQueryInformationProcess(hProcess, ProcessBasicInformation, *pbi, *dwSize, &dwSizeNeeded);
	}

	return dwStatus;
}

BOOL mint::GetNtProcessInfo(IN const DWORD dwPID, OUT PROCESSINFO *ppi) {

	BOOL bReturnStatus = TRUE;
	DWORD dwSize = 0;
	DWORD dwSizeNeeded = 0;
	SIZE_T dwBytesRead = 0;
	HANDLE hHeap = 0;
	WCHAR *pwszBuffer = NULL;

	PROCESSINFO spi = {0};
	PPROCESS_BASIC_INFORMATION pbi = NULL;

	PEB peb = {0};
	PEB_LDR_DATA peb_ldr = {0};
	RTL_USER_PROCESS_PARAMETERS peb_upp = {0};

	ZeroMemory(&spi, sizeof(spi));
	ZeroMemory(&peb, sizeof(peb));
	ZeroMemory(&peb_ldr, sizeof(peb_ldr));
	ZeroMemory(&peb_upp, sizeof(peb_upp));

	spi.dwPID = dwPID;

	// Attempt to access process
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPID);

	if (hProcess == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	// Try to allocate buffer
	hHeap = GetProcessHeap();
	NTSTATUS dwStatus = LoadNtProcessBasicInformation(hProcess, &pbi, &dwSize);

	// Did we successfully get basic info on process
	if (dwStatus >= STATUS_SUCCESS) {
		// Basic Info
		//        spi.dwPID = (DWORD)pbi->UniqueProcessId;
		spi.dwParentPID = (DWORD)pbi->InheritedFromUniqueProcessId;
		spi.dwBasePriority = (LONG)pbi->BasePriority;
		spi.dwExitStatus = (NTSTATUS)pbi->ExitStatus;
		spi.dwPEBBaseAddress = (DWORD)pbi->PebBaseAddress;
		spi.dwAffinityMask = (DWORD)pbi->AffinityMask;

		// Read Process Environment Block (PEB)
		if (pbi->PebBaseAddress) {
			if (ReadProcessMemory(hProcess, pbi->PebBaseAddress, &peb, sizeof(peb), &dwBytesRead)) {
				spi.dwSessionID = (DWORD)peb.SessionId;
				spi.cBeingDebugged = (BYTE)peb.BeingDebugged;

				// Here we could access PEB_LDR_DATA, i.e., module list for process
				//				dwBytesRead = 0;
				//				if(ReadProcessMemory(hProcess,
				//									 pbi->PebBaseAddress->Ldr,
				//									 &peb_ldr,
				//									 sizeof(peb_ldr),
				//									 &dwBytesRead))
				//				{
				// get ldr
				//				}

				// if PEB read, try to read Process Parameters
				dwBytesRead = 0;
				if (ReadProcessMemory(hProcess, peb.ProcessParameters, &peb_upp, sizeof(RTL_USER_PROCESS_PARAMETERS), &dwBytesRead)) {
					// We got Process Parameters, is CommandLine filled in
					if (peb_upp.CommandLine.Length > 0) {
						// Yes, try to read CommandLine
						pwszBuffer = (WCHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, peb_upp.CommandLine.Length);

						// If memory was allocated, continue
						if (pwszBuffer) {
							if (ReadProcessMemory(hProcess, peb_upp.CommandLine.Buffer, pwszBuffer, peb_upp.CommandLine.Length, &dwBytesRead)) {
								// Copy CommandLine to our structure variable
								// Since core NT functions operate in Unicode
								// there is no conversion if application is
								// compiled for Unicode
								wcsncpy(spi.szCmdLine, pwszBuffer, MAX_UNICODE_PATH - 1);
							}
							if (!HeapFree(hHeap, 0, pwszBuffer)) {
								// failed to free memory
								bReturnStatus = FALSE;
								goto gnpiFreeMemFailed;
							}
						}
					}	// Read CommandLine in Process Parameters

					// We got Process Parameters, is ImagePath filled in
					if (peb_upp.ImagePathName.Length > 0) {
						// Yes, try to read ImagePath
						dwBytesRead = 0;
						pwszBuffer = (WCHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, peb_upp.ImagePathName.Length);
						if (pwszBuffer) {
							if (ReadProcessMemory(hProcess, peb_upp.ImagePathName.Buffer, pwszBuffer, peb_upp.ImagePathName.Length, &dwBytesRead)) {
								// Copy ImagePath to our structure
								wcsncpy(spi.szImgPath, pwszBuffer, MAX_UNICODE_PATH - 1);
							}
							if (!HeapFree(hHeap, 0, pwszBuffer)) {
								// failed to free memory
								bReturnStatus = FALSE;
								goto gnpiFreeMemFailed;
							}
						}
					}	// Read ImagePath in Process Parameters

					if (peb_upp.CurrentDirectory.Path.Length > 0) {
						// Yes, try to read CurrentDirectoryPath
						pwszBuffer = (WCHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, peb_upp.CurrentDirectory.Path.Length);

						// If memory was allocated, continue
						if (pwszBuffer) {
							if (ReadProcessMemory(hProcess, peb_upp.CurrentDirectory.Path.Buffer, pwszBuffer, peb_upp.CurrentDirectory.Path.Length, &dwBytesRead)) {
								// Copy CurrentDirectoryPath to our structure variable
								// Since core NT functions operate in Unicode
								// there is no conversion if application is
								// compiled for Unicode
								wcsncpy(spi.szCurrentDirectoryPath, pwszBuffer, MAX_UNICODE_PATH - 1);
							}
							if (!HeapFree(hHeap, 0, pwszBuffer)) {
								// failed to free memory
								bReturnStatus = FALSE;
								goto gnpiFreeMemFailed;
							}
						}
					}	// Read CurrentDirectoryPath in Process Parameters
				}	// Read Process Parameters

				USHORT usEnvStrBlockLength = 0;
				PBYTE pAddrEnvStrBlock = (PBYTE)(peb.ProcessParameters->Environment);

				if (HasReadAccess(hProcess, pAddrEnvStrBlock, usEnvStrBlockLength)) {

					// Allocate buffer for to copy the block
					pwszBuffer = (WCHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, usEnvStrBlockLength);

					// Read the environment block
					ReadProcessMemory(hProcess, (LPCVOID)pAddrEnvStrBlock, pwszBuffer, usEnvStrBlockLength, &dwBytesRead);
					// Cleanup existing data if any

					if (dwBytesRead) {
						// Copy Environment to our structure variable
						memcpy(spi.szEnvironment, pwszBuffer, dwBytesRead);
					}
					if (!HeapFree(hHeap, 0, pwszBuffer)) {
						// failed to free memory
						bReturnStatus = FALSE;
						goto gnpiFreeMemFailed;
					}
				} // Read the environment block
			}	// Read PEB
		}	// Check for PEB

		// System process for WinXP and later is PID 4 and we cannot access
		// PEB, but we know it is aka ntoskrnl.exe so we will manually define it.
		// ntkrnlpa.exe if Physical Address Extension (PAE)
		// ntkrnlmp.exe if Symmetric MultiProcessing (P)
		// Actual filename is ntoskrnl.exe, but other name will be in
		// Original Filename field of version block.
		if (spi.dwPID == 4) {
			ExpandEnvironmentStringsW(L"%SystemRoot%\\System32\\ntoskrnl.exe", spi.szImgPath, sizeof(spi.szImgPath));
		}
	}	// Read Basic Info

gnpiFreeMemFailed:

	// Free memory if allocated
	if (pbi != NULL) {
		if (!HeapFree(hHeap, 0, pbi)) {
			// failed to free memory
		}
	}

	CloseHandle(hProcess);

	// Return filled in structure to caller
	*ppi = spi;

	return bReturnStatus;
}

LPWSTR mint::GetNtProcessCommandLine(HANDLE hProcess) {

	DWORD dwSize = 0;
	PPROCESS_BASIC_INFORMATION pbi = NULL;

	HANDLE hHeap = GetProcessHeap();
	DWORD dwPID = GetProcessId(hProcess);

	NTSTATUS dwStatus = LoadNtProcessBasicInformation(hProcess, &pbi, &dwSize);

	// Did we successfully get basic info on process
	if (dwStatus >= STATUS_SUCCESS) {

		// Read Process Environment Block (PEB)
		if (pbi->PebBaseAddress) {

			SIZE_T dwBytesRead = 0;

			PEB peb = { 0 };
			RTL_USER_PROCESS_PARAMETERS peb_upp = { 0 };

			ZeroMemory(&peb, sizeof (peb));
			ZeroMemory(&peb_upp, sizeof (peb_upp));

			if (ReadProcessMemory(hProcess, pbi->PebBaseAddress, &peb, sizeof(peb), &dwBytesRead)) {
				if (ReadProcessMemory(hProcess, peb.ProcessParameters, &peb_upp, sizeof(RTL_USER_PROCESS_PARAMETERS), &dwBytesRead)) {
					if (peb_upp.CommandLine.Length > 0) {
						LPWSTR lpCommandLine = (LPWSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, peb_upp.CommandLine.Length);
						if (ReadProcessMemory(hProcess, peb_upp.CommandLine.Buffer, lpCommandLine, peb_upp.CommandLine.Length, &dwBytesRead)) {
							lpCommandLine[dwBytesRead / sizeof (WCHAR)] = '\0';
							HeapFree(hHeap, 0, pbi);
							return lpCommandLine;
						}
					}
				}
			}
		}
		
		HeapFree(hHeap, 0, pbi);
	}

	// System process for WinXP and later is PID 4 and we cannot access
	// PEB, but we know it is aka ntoskrnl.exe so we will manually define it.
	// ntkrnlpa.exe if Physical Address Extension (PAE)
	// ntkrnlmp.exe if Symmetric MultiProcessing (P)
	// Actual filename is ntoskrnl.exe, but other name will be in
	// Original Filename field of version block.
	if (dwPID == 4) {
		LPWSTR lpCommandLine = (LPWSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_UNICODE_PATH);
		ExpandEnvironmentStringsW(L"%SystemRoot%\\System32\\ntoskrnl.exe", lpCommandLine, MAX_UNICODE_PATH);
		return lpCommandLine;
	}

	return NULL;
}

DWORD mint::GetNtProcessCurrentDirectory(HANDLE hProcess, LPWSTR lpCurrentDirectory, DWORD nSize) {

	DWORD dwSize = 0;
	PPROCESS_BASIC_INFORMATION pbi = NULL;

	HANDLE hHeap = GetProcessHeap();

	NTSTATUS dwStatus = LoadNtProcessBasicInformation(hProcess, &pbi, &dwSize);

	// Did we successfully get basic info on process
	if (dwStatus >= STATUS_SUCCESS) {

		// Read Process Environment Block (PEB)
		if (pbi->PebBaseAddress) {

			SIZE_T dwBytesRead = 0;

			PEB peb = { 0 };
			RTL_USER_PROCESS_PARAMETERS peb_upp = { 0 };

			ZeroMemory(&peb, sizeof (peb));
			ZeroMemory(&peb_upp, sizeof (peb_upp));

			if (ReadProcessMemory(hProcess, pbi->PebBaseAddress, &peb, sizeof(peb), &dwBytesRead)) {
				if (ReadProcessMemory(hProcess, peb.ProcessParameters, &peb_upp, sizeof(RTL_USER_PROCESS_PARAMETERS), &dwBytesRead)) {
					if ((peb_upp.CurrentDirectory.Path.Length > 0) && lpCurrentDirectory) {
						if (ReadProcessMemory(hProcess, peb_upp.CurrentDirectory.Path.Buffer, lpCurrentDirectory, nSize, &dwBytesRead)) {
							lpCurrentDirectory[dwBytesRead / sizeof (WCHAR)] = '\0';
						}
					}

					DWORD dwLength = peb_upp.CurrentDirectory.Path.Length;
					HeapFree(hHeap, 0, pbi);
					return dwLength;
				}
			}
		}

		HeapFree(hHeap, 0, pbi);
	}
	
	return 0;
}

LPWCH mint::GetNtProcessEnvironmentStrings(HANDLE hProcess) {

	DWORD dwSize = 0;
	PPROCESS_BASIC_INFORMATION pbi = NULL;

	HANDLE hHeap = GetProcessHeap();

	NTSTATUS dwStatus = LoadNtProcessBasicInformation(hProcess, &pbi, &dwSize);

	// Did we successfully get basic info on process
	if (dwStatus >= STATUS_SUCCESS) {

		// Read Process Environment Block (PEB)
		if (pbi->PebBaseAddress) {

			SIZE_T dwBytesRead = 0;
			
			PEB peb = { 0 };
			RTL_USER_PROCESS_PARAMETERS peb_upp = { 0 };

			ZeroMemory(&peb, sizeof (peb));
			ZeroMemory(&peb_upp, sizeof (peb_upp));

			if (ReadProcessMemory(hProcess, pbi->PebBaseAddress, &peb, sizeof(peb), &dwBytesRead)) {
				if (ReadProcessMemory(hProcess, peb.ProcessParameters, &peb_upp, sizeof(RTL_USER_PROCESS_PARAMETERS), &dwBytesRead)) {
					USHORT usEnvStrBlockLength = 0;
					if (HasReadAccess(hProcess, peb_upp.Environment, usEnvStrBlockLength)) {
						LPWCH lpEnvironmentStrings = (LPWCH)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, usEnvStrBlockLength);
						if (ReadProcessMemory(hProcess, peb_upp.Environment, lpEnvironmentStrings, usEnvStrBlockLength, &dwBytesRead)) {
							HeapFree(hHeap, 0, pbi);
							return lpEnvironmentStrings;
						}
					}
				}
			}
		}

		HeapFree(hHeap, 0, pbi);
	}

	return NULL;
}
