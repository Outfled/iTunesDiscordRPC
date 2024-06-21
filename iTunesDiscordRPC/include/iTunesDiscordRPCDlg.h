
// iTunesDiscordRPCDlg.h : header file
//

#pragma once

// CiTunesDiscordRPCDlg dialog
class CiTunesDiscordRPCDlg : public CDialogEx
{
// Construction
public:
	CiTunesDiscordRPCDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ITUNES_DISCORD_RPC_2_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg HBRUSH	OnCtlColor( CDC *pDC, CWnd *pWnd, UINT uCtlColor );
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCheckboxStartupapp();
	afx_msg void OnBnClickedCheckboxEnabled();
	afx_msg void OnBnClickedCheckboxTrayenabled();
	afx_msg void OnBnClickedButtonClose();
};
