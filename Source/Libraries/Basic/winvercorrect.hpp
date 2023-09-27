#pragma once

typedef LONG NTSTATUS, * PNTSTATUS;
#define STATUS_SUCCESS ((NTSTATUS)0x00000000)
#define STATUS_REVISION_MISMATCH ((NTSTATUS)0xC0000059)
typedef LONG(WINAPI* PFN_RtlVerifyVersionInfo)(OSVERSIONINFOEXW*, ULONG, ULONGLONG);

static BOOL IsWindowsVersionOrGreaterNew(WORD major, WORD minor);

BOOL IsWindowsVersionOrGreaterNew(WORD major, WORD minor)
{
	static PFN_RtlVerifyVersionInfo RtlVerifyVersionInfoFn = NULL;
	if (!RtlVerifyVersionInfoFn)
	{
		HMODULE ntdllModule = GetModuleHandleW(L"ntdll.dll");
		if (ntdllModule)
		{
			RtlVerifyVersionInfoFn = (PFN_RtlVerifyVersionInfo)GetProcAddress(ntdllModule, "RtlVerifyVersionInfo");
		}
	}
	RTL_OSVERSIONINFOEXW versionInfo = { 0 };
	NTSTATUS status;
	ULONGLONG conditionMask = 0;
	versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
	versionInfo.dwMajorVersion = major;
	versionInfo.dwMinorVersion = minor;

	VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

	status = RtlVerifyVersionInfoFn(&versionInfo, VER_MAJORVERSION | VER_MINORVERSION, conditionMask);

	if (status == STATUS_SUCCESS)
		return TRUE;

	return FALSE;
}

#define IsWindowsVistaOrGreater()   IsWindowsVersionOrGreaterNew(HIBYTE(0x0600), LOBYTE(0x0600)) // _WIN32_WINNT_VISTA
#define IsWindows7SP1OrGreater()   IsWindowsVersionOrGreaterNew(HIBYTE(0x0601), LOBYTE(0x0601)) // _WIN32_WINNT_WIN7
#define IsWindows8OrGreater()       IsWindowsVersionOrGreaterNew(HIBYTE(0x0602), LOBYTE(0x0602)) // _WIN32_WINNT_WIN8
#define IsWindows8Point1OrGreater() IsWindowsVersionOrGreaterNew(HIBYTE(0x0603), LOBYTE(0x0603)) // _WIN32_WINNT_WINBLUE