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

//--------------------------------------------------
// Dialog Constructor -- Loads the Application Icon
CiTunesDiscordRPCDlg::CiTunesDiscordRPCDlg(CWnd* pParent) : CDialogEx(IDD_ITUNES_DISCORD_RPC_DIALOG, pParent)
{
	m_hIcon	= AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

//--------------------------------------------------
// Dialog Data Exchange (DDX) and Dialog Data Validation (DDV) Support
void CiTunesDiscordRPCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


//--------------------------------------------------
// Dialog Class Message Map
BEGIN_MESSAGE_MAP( CiTunesDiscordRPCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED( IDC_CHECKBOX_STARTUPAPP, &CiTunesDiscordRPCDlg::OnBnClickedCheckboxStartupApp)
	ON_BN_CLICKED( IDC_CHECKBOX_ENABLED, &CiTunesDiscordRPCDlg::OnBnClickedCheckboxEnabled )
	ON_BN_CLICKED( IDC_CHECKBOX_TRAYENABLED, &CiTunesDiscordRPCDlg::OnBnClickedCheckboxMinimizeToTray)
	ON_BN_CLICKED( IDC_BUTTON_CLOSE, &CiTunesDiscordRPCDlg::OnBnClickedButtonClose )
END_MESSAGE_MAP()


#pragma warning( push )
#pragma warning( disable : 6001 ) /* Using uninitialized memory */
#pragma warning( disable : 6387 ) /* Variable (g_hAppDisabledEvent) could be NULL */

//--------------------------------------------------
// Called After Dialog Window & All Controls Are Created; Right Before Displaying the Window
BOOL CiTunesDiscordRPCDlg::OnInitDialog()
{
	BOOL	bIsEnabled;
	BOOL	bMinimizeToTray;
	BOOL	bStartupApp;
	LRESULT lStatus;

	CDialogEx::OnInitDialog();

	//
	// Initialize global thread events
	// 
	g_hITunesClosingEvent	= CreateEvent( NULL, TRUE, FALSE, NULL );
	g_hAppDisabledEvent		= CreateEvent( NULL, TRUE, FALSE, NULL );

	ASSERT(g_hITunesClosingEvent != NULL && g_hAppDisabledEvent != NULL);

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
		TerminateApplication( this, L"An unknown error occurred", (DWORD)lStatus );
	}

	/* Set/Remove startup application */
	SetApplicationStartupProgram(bStartupApp);

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

	//
	// Start global threads
	//
	g_hITunesThread = CreateThread( NULL, 0, iTunesHandlerThread, this, 0, NULL );
	g_hTrayThread	= CreateThread( NULL, 0, ApplicationTrayHandler, this, 0, NULL );

	ASSERT(g_hITunesThread != NULL && g_hTrayThread != NULL);

	/* return TRUE since we are not setting the focus to a control */
	return TRUE;
}

#pragma warning( pop )

//--------------------------------------------------
// Called When the User Sends a Command to the Dialog Window, Typically the Close Button
void CiTunesDiscordRPCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	switch (nID)
	{
		//
		// Window is closing
		//
	case SC_CLOSE:
		this->OnBnClickedButtonClose();
		break;
	default:
		CDialogEx::OnSysCommand(nID, lParam);
		break;
	}
}

//--------------------------------------------------
// Called Whenever the Dialog Window Needs to be Painted
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

//--------------------------------------------------
// Method Called when the 'Enabled' Checkbox is Modified
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

	/* Send event to iTunes to trigger status display */
	InvokeITunesEvent();
}

//--------------------------------------------------
// Method Called when the 'Startup Application' Checkbox is Modified
void CiTunesDiscordRPCDlg::OnBnClickedCheckboxStartupApp()
{
	CButton *pButton = (CButton *)GetDlgItem( IDC_CHECKBOX_STARTUPAPP );

	SetApplicationRegValue( APP_REG_VALUE_STARTUPAPP, pButton->GetCheck() );
	SetApplicationStartupProgram( pButton->GetCheck() );
}

//--------------------------------------------------
// Method Called when the 'Minimize to Tray' Checkbox is Modified
void CiTunesDiscordRPCDlg::OnBnClickedCheckboxMinimizeToTray()
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

//--------------------------------------------------
// Method Called when the 'Close' Button is Clicked
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
