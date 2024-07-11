#include "pch.h"
#include "include/AppReg.h"

#define HKEY_BASE_PATH		L"SOFTWARE\\Outfled"
#define HKEY_BASE_NAME		L"iTunes Discord RPC"
#define HKEY_STARUP_PATH	L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"
#define HKEY_STARTUP_VALUE	HKEY_BASE_NAME

static LRESULT OpenAppBaseKey(PHKEY phKey);

//--------------------------------------------------
// Read Value from the Application Registry Key
LRESULT GetApplicationRegValue( LPCWSTR lpszValue, LPBOOL lpbStatus )
{
	LRESULT lResult;
	HKEY	hBaseKey;
	DWORD	dwValue;
	DWORD	cbValue;

	/* Open/create HKEY_CURRENT_USER\SOFTWARE\Outfled key */
	lResult = OpenAppBaseKey( &hBaseKey );
	if (FAILED( lResult ))
	{
		return lResult;
	}

	/* Query key value */
	lResult = RegQueryValueEx( hBaseKey, lpszValue, NULL, NULL, (LPBYTE)&dwValue, &(cbValue = sizeof( DWORD )) );
	if (SUCCEEDED( lResult )) {
		*lpbStatus = (dwValue == 1);
	}

	RegCloseKey( hBaseKey );
	return lResult;
}

//--------------------------------------------------
// Set Value in the Application Registry Key
LRESULT SetApplicationRegValue( LPCWSTR lpszValue, BOOL bStatus )
{
	LRESULT lResult;
	HKEY	hBaseKey;

	/* Open/create HKEY_CURRENT_USER\SOFTWARE\Outfled key */
	lResult = OpenAppBaseKey( &hBaseKey );
	if (FAILED( lResult ))
	{
		return lResult;
	}

	/* Set key value */
	lResult = RegSetValueEx( hBaseKey, lpszValue, 0, REG_DWORD, (LPBYTE)&bStatus, sizeof( bStatus ) );

	RegCloseKey( hBaseKey );
	return lResult;
}

//--------------------------------------------------
// Set/Remove Application from the Windows Startup Programs Registry Key
LRESULT SetApplicationStartupProgram( BOOL bAdd )
{
	HKEY	hStartupKey;
	LRESULT lResult;

	lResult = RegOpenKey( HKEY_CURRENT_USER, HKEY_STARUP_PATH, &hStartupKey );
	if (SUCCEEDED( lResult ))
	{
		if (bAdd)
		{
			WCHAR szCurrentPath[1024] = { 0 };

			/* Get full path of current executable */
			GetModuleFileName(NULL, szCurrentPath + 1, ARRAYSIZE(szCurrentPath) - 1);

			/* Wrap the path string in quotes */
			szCurrentPath[0] = L'\"';
			szCurrentPath[wcslen(szCurrentPath)] = L'\"';

			/* Append '--no-wnd' to start the exe without the window */
			StringCchCopy(
				szCurrentPath + wcslen(szCurrentPath),
				ARRAYSIZE(szCurrentPath) - wcslen(szCurrentPath),
				L" --no-wnd"
			);

			lResult = RegSetValueEx(hStartupKey,
				HKEY_STARTUP_VALUE,
				0,
				REG_SZ,
				(LPBYTE)szCurrentPath,
				(DWORD)(wcslen(szCurrentPath) + 1) * sizeof(WCHAR)
			);
		}
		else
		{
			lResult = RegDeleteValue(hStartupKey, HKEY_STARTUP_VALUE);
		}

		RegCloseKey( hStartupKey );
	}

	return lResult;
}

//--------------------------------------------------
// Open/Create Registry Key HKEY_CURRENT_USER\Software\Outfled\iTunesDiscordRPC
LRESULT OpenAppBaseKey(PHKEY phKey)
{
	HKEY	hCreated;
	LRESULT lResult;
	DWORD	dwDisposition;

	*phKey = NULL;

	lResult = RegCreateKeyEx(HKEY_CURRENT_USER, HKEY_BASE_PATH, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hCreated, NULL);
	if (SUCCEEDED(lResult))
	{
		lResult = RegCreateKeyEx(hCreated, HKEY_BASE_NAME, 0, NULL, 0, KEY_ALL_ACCESS, NULL, phKey, &dwDisposition);
		if (lResult == ERROR_SUCCESS && dwDisposition == REG_CREATED_NEW_KEY)
		{
			DWORD dwValue = 1;

			/* Set default values if the registry key was created & not opened */
			RegSetValueEx(*phKey, APP_REG_VALUE_ENABLED, 0, REG_DWORD, (BYTE *)&dwValue, sizeof(DWORD));
			RegSetValueEx(*phKey, APP_REG_VALUE_STARTUPAPP, 0, REG_DWORD, (BYTE *)&dwValue, sizeof(DWORD));
			RegSetValueEx(*phKey, APP_REG_VALUE_MINIMIZETRAY, 0, REG_DWORD, (BYTE *)&dwValue, sizeof(DWORD));
		}
	}

	return lResult;
}