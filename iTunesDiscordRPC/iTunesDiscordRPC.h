#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


class CiTunesDiscordRPCApp : public CWinApp
{
public:
	CiTunesDiscordRPCApp();

//
// Overrides
//
public:
	virtual BOOL InitInstance();

	//
	// Implementation
	//

	DECLARE_MESSAGE_MAP()
};

extern CiTunesDiscordRPCApp theApp;
