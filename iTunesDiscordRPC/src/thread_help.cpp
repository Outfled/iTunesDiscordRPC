#include "pch.h"
#include <TlHelp32.h>

//--------------------------------------------------
// Check if a Process is Running Using it's Exe Name
DWORD FindProcessByExeName(LPCWSTR lpszProcessExe)
{
	HANDLE			hSnapshot;
	PROCESSENTRY32	EntryData;
	DWORD			dwProcessId;

	dwProcessId = 0;

	/* Create process snapshot */
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE)
	{
		EntryData.dwSize = sizeof(EntryData);

		//
		// Enumerate the snapped processes and find the exe
		//
		if (Process32First(hSnapshot, &EntryData))
		{
			do
			{
				if (0 == _wcsicmp(EntryData.szExeFile, lpszProcessExe))
				{
					dwProcessId = EntryData.th32ProcessID;
					break;
				}
			} while (Process32Next(hSnapshot, &EntryData));
		}

		CloseHandle(hSnapshot);
	}

	return dwProcessId;
}