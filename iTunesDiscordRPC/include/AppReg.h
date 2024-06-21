//------------------------------------------------------------------------
// 
// AppReg.h
// Set/Get Registry Values for Default Checked Values in Application
// 
//------------------------------------------------------------------------

#pragma once

#define APP_REG_VALUE_ENABLED		L"Enabled"
#define APP_REG_VALUE_STARTUPAPP	L"Startup"
#define APP_REG_VALUE_MINIMIZETRAY	L"MinimizeToTray"

LRESULT GetApplicationRegValue( LPCWSTR lpszValue, LPBOOL lpbStatus );

LRESULT SetApplicationRegValue( LPCWSTR lpszValue, BOOL bStatus );

LRESULT SetApplicationStartupProgram( BOOL bAdd );
