#include "pch.h"
#include "framework.h"
#include "iTunesDiscordRPC.h"
#include "iTunesDiscordRPCDlg.h"
#include "afxdialogex.h"
#include "include/AppReg.h"
#include "include/iTunesThread.h"
#include "include/iTunesEvents.h"
#include "include/DiscordStatus.h"
#include "include/AppTray.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_ACTIVATEWND		WM_USER + 1
#define ACTIVATE_TIMEOUT	1000


static HANDLE g_hTrayThread;
static HANDLE g_hITunesThread;

static CWnd *GetControlPosition( int nCtlID, LPRECT lpr, CWnd *pWnd)
{
	/* Get dialogue window position */
	if (nCtlID == 0) {
		pWnd->GetClientRect( lpr );
		return NULL;
	}

	CWnd *pCtlWnd = pWnd->GetDlgItem( nCtlID );

	pCtlWnd->GetWindowRect( lpr );
	pWnd->ScreenToClient( lpr );

	return pCtlWnd;
}

static VOID AlignDlgControls( CDialogEx *pWnd )
{
	RECT DlgRect;
	RECT TitleRect;
	RECT VersionRect;
	
	// Get dialogue position
	GetControlPosition( 0, &DlgRect, pWnd );

	// Title and version text positions
	CWnd *pTitleWnd	= GetControlPosition( IDC_STATIC_TITLETEXT, &TitleRect, pWnd );
	CWnd *pVersionWnd =	GetControlPosition( IDC_STATIC_VERSIONTEXT, &VersionRect, pWnd );

	TitleRect.left = 1;

	/* Attach each control to top of dialogue */
	TitleRect.top	= 1;
	VersionRect.top = 1;

	/* Set each text to have same height */
	TitleRect.bottom = VersionRect.bottom;
		
	pTitleWnd->MoveWindow( &TitleRect );
	pVersionWnd->MoveWindow( &VersionRect );

	/* Set title 2 */
	pTitleWnd = GetControlPosition( IDC_STATIC_AUTHORTEXT, &TitleRect, pWnd );
	TitleRect.left = 1;
	pTitleWnd->MoveWindow( &TitleRect );

	RECT EnabledCBRect;
	RECT TrayCBRect;
	RECT StartupCBRect;

	/* Update each check box */
	CWnd *pEnabledCB = GetControlPosition( IDC_CHECKBOX_ENABLED, &EnabledCBRect, pWnd );
	CWnd *pTrayCB = GetControlPosition( IDC_CHECKBOX_TRAYENABLED, &TrayCBRect, pWnd );
	CWnd *pStartupCB = GetControlPosition( IDC_CHECKBOX_STARTUPAPP, &StartupCBRect, pWnd );

	EnabledCBRect.left	= 1;
	StartupCBRect.left	= 1;
	TrayCBRect.bottom	= StartupCBRect.bottom;
	TrayCBRect.top		= StartupCBRect.top;

	pEnabledCB->MoveWindow( &EnabledCBRect );
	pTrayCB->MoveWindow( &TrayCBRect );
	pStartupCB->MoveWindow( &StartupCBRect );

	/* Update button position */
	RECT CloseBtnRect;
	CWnd *pButton = GetControlPosition( IDC_BUTTON_CLOSE, &CloseBtnRect, pWnd);

	CloseBtnRect.right = DlgRect.right;
	CloseBtnRect.left = 0;

	pButton->MoveWindow( &CloseBtnRect );
}

CiTunesDiscordRPCDlg::CiTunesDiscordRPCDlg(CWnd* pParent) : CDialogEx(IDD_ITUNES_DISCORD_RPC_DIALOG, pParent)
{
	m_hIcon	= AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CiTunesDiscordRPCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP( CiTunesDiscordRPCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED( IDC_CHECKBOX_STARTUPAPP, &CiTunesDiscordRPCDlg::OnBnClickedCheckboxStartupapp )
	ON_BN_CLICKED( IDC_CHECKBOX_ENABLED, &CiTunesDiscordRPCDlg::OnBnClickedCheckboxEnabled )
	ON_BN_CLICKED( IDC_CHECKBOX_TRAYENABLED, &CiTunesDiscordRPCDlg::OnBnClickedCheckboxTrayenabled )
	ON_BN_CLICKED( IDC_BUTTON_CLOSE, &CiTunesDiscordRPCDlg::OnBnClickedButtonClose )
END_MESSAGE_MAP()


#pragma warning( push )
#pragma warning( disable : 6001 ) /* Using uninitialized memory */
BOOL CiTunesDiscordRPCDlg::OnInitDialog()
{
	BOOL	bIsEnabled;
	BOOL	bMinimizeToTray;
	BOOL	bStartupApp;
	LRESULT lStatus;

	CDialogEx::OnInitDialog();

	/* Initialize events for threads */
	g_hITunesClosingEvent	= CreateEvent( NULL, TRUE, FALSE, NULL );
	g_hAppDisabledEvent		= CreateEvent( NULL, TRUE, FALSE, NULL );

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	/* Set the big & small icons */
	SetIcon(m_hIcon, TRUE);	
	SetIcon(m_hIcon, FALSE);
	
	/* Align the controls */
	AlignDlgControls( this );

	//
	// Get the previous checkbox values from registry
	//
	lStatus = GetApplicationRegValue( APP_REG_VALUE_ENABLED, &bIsEnabled );
	if (SUCCEEDED( lStatus )) {
		lStatus = GetApplicationRegValue( APP_REG_VALUE_MINIMIZETRAY, &bMinimizeToTray );
	}
	if (SUCCEEDED( lStatus )) {
		lStatus = GetApplicationRegValue( APP_REG_VALUE_STARTUPAPP, &bStartupApp );
	}
	
	if (FAILED( lStatus ))
	{
		TerminateApplication( this, L"An unknown error occurred", lStatus );
	}

	//
	// Set the checkbox values
	//
	((CButton *)GetDlgItem( IDC_CHECKBOX_ENABLED ))->SetCheck( bIsEnabled );
	((CButton *)GetDlgItem( IDC_CHECKBOX_TRAYENABLED ))->SetCheck( bMinimizeToTray );
	((CButton *)GetDlgItem( IDC_CHECKBOX_STARTUPAPP ))->SetCheck( bStartupApp );

	/* Check if disabled */
	if (bIsEnabled == 0)
	{
		SetEvent( g_hAppDisabledEvent );
	}

	InitializeCriticalSection( &g_CritSection );

	/* Start threads */
	g_hITunesThread = CreateThread( NULL, 0, iTunesHandlerThread, this, 0, NULL );
	g_hTrayThread	= CreateThread( NULL, 0, ApplicationTrayHandler, this, 0, NULL );

	return TRUE;  // return TRUE  unless you set the focus to a control
}
#pragma warning( pop )

void CiTunesDiscordRPCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	/* Window is closing */
	if ( nID == SC_CLOSE )
	{
		this->OnBnClickedButtonClose();
		return;
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CiTunesDiscordRPCDlg::OnPaint()
{
	if (IsIconic())
	{
		/* Device context for painting */
		CPaintDC dc(this); 

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);

		CRect rect;
		GetClientRect(&rect);

		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//--------------------------------------------------
// Called by System to Obtain the Cursor to Display While the User Drags the Minimized Window
HCURSOR CiTunesDiscordRPCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//--------------------------------------------------
// Set the Window Background Color
HBRUSH CiTunesDiscordRPCDlg::OnCtlColor( CDC *pDC, CWnd *pWnd, UINT uCtlColor )
{
	return (HBRUSH)GetStockObject( WHITE_BRUSH );
}


void CiTunesDiscordRPCDlg::OnBnClickedCheckboxEnabled()
{
	CButton *pButton = (CButton *)GetDlgItem( IDC_CHECKBOX_ENABLED );
	SetApplicationRegValue( APP_REG_VALUE_ENABLED, pButton->GetCheck() );

	if (pButton->GetCheck())
	{
		ResetEvent( g_hAppDisabledEvent );	
	}
	else
	{
		SetEvent( g_hAppDisabledEvent );
	}

	Sleep( 10 );

	/* Send event to iTunes to trigger status display */
	LONG cVolume;
	if (g_ItunesConnection && SUCCEEDED(CoInitializeEx( NULL, COINITBASE_MULTITHREADED )))
	{
		g_ItunesConnection->get_SoundVolume( &cVolume );
		g_ItunesConnection->put_SoundVolume( ++cVolume );
		g_ItunesConnection->put_SoundVolume( --cVolume );
		CoUninitialize();
	}
}

void CiTunesDiscordRPCDlg::OnBnClickedCheckboxStartupapp()
{
	CButton *pButton = (CButton *)GetDlgItem( IDC_CHECKBOX_STARTUPAPP );

	SetApplicationRegValue( APP_REG_VALUE_STARTUPAPP, pButton->GetCheck() );
	SetApplicationStartupProgram( pButton->GetCheck() );
}

void CiTunesDiscordRPCDlg::OnBnClickedCheckboxTrayenabled()
{
	CButton *MinimizeTrayBtn = (CButton *)GetDlgItem( IDC_CHECKBOX_TRAYENABLED );
	SetApplicationRegValue( APP_REG_VALUE_MINIMIZETRAY, MinimizeTrayBtn->GetCheck() );
	if (MinimizeTrayBtn->GetCheck() == 0)
	{
		((CButton *)GetDlgItem( IDC_CHECKBOX_ENABLED ))->EnableWindow( FALSE );
	}
	else {
		( (CButton *)GetDlgItem( IDC_CHECKBOX_ENABLED ) )->EnableWindow( TRUE );
	}
}

void CiTunesDiscordRPCDlg::OnBnClickedButtonClose()
{
	CButton *MinimizeTrayBtn = (CButton *)GetDlgItem( IDC_CHECKBOX_TRAYENABLED );
	if (MinimizeTrayBtn->GetCheck() == 0)
	{
		/* Minimize to tray is disabled -- exit the app */
		UpdateDiscordStatus( NULL ); // Clear Discord status

		/* Delete tray icon */
		Shell_NotifyIcon( NIM_DELETE, &g_IconData );
		::DestroyWindow( g_hTrayWnd );
		UnregisterClassW( TRAY_WNDCLASS_NAME, GetModuleHandle( NULL ) );

		/* Cleanup iTunes connection */
		SafeRelease( &g_ItunesConnection );
		WaitForSingleObject( g_hITunesThread, 1000 );

		ExitProcess( 0 );
	}

	AfxGetMainWnd()->ShowWindow( SW_HIDE );
}
