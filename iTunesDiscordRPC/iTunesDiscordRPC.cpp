
// iTunesDiscordRPC.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "iTunesDiscordRPC.h"
#include "iTunesDiscordRPCDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CiTunesDiscordRPCApp

BEGIN_MESSAGE_MAP(CiTunesDiscordRPCApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CiTunesDiscordRPCApp construction

CiTunesDiscordRPCApp::CiTunesDiscordRPCApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CiTunesDiscordRPCApp object

CiTunesDiscordRPCApp theApp;


// CiTunesDiscordRPCApp initialization

BOOL CiTunesDiscordRPCApp::InitInstance()
{
	LPWSTR *pCommandLineArgs;
	INT		nArgs;

	pCommandLineArgs = ::CommandLineToArgvW( ::GetCommandLine(), &nArgs );

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("iTunesDiscordRPC"));

	CiTunesDiscordRPCDlg	dlg;
	INT_PTR					nResponse;

	m_pMainWnd	= &dlg;
	nResponse	= -2;

	// Check command line arguments
	if ( nArgs > 1 )
	{
		// Dont display the dlg window. Automatically add to tray
		if ( 0 == _wcsicmp( pCommandLineArgs[1], L"--no-wnd" ) )
		{
			dlg.Create( IDD_ITUNES_DISCORD_RPC_DIALOG );
			dlg.ShowWindow( SW_HIDE );
			nResponse = dlg.RunModalLoop();
		}
	}
	if ( nResponse == -2 )
	{
		nResponse = dlg.DoModal();
	}

	// Check the dlg response
	if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

	// Delete the shell manager created above.
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

