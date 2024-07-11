#pragma once

/* CiTunesDiscordRPCDlg Dialog class */
class CiTunesDiscordRPCDlg : public CDialogEx
{
public:
	CiTunesDiscordRPCDlg(CWnd* pParent = nullptr);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	//
	// Message Map Methods
	//

	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg HBRUSH	OnCtlColor( CDC *pDC, CWnd *pWnd, UINT uCtlColor );
	DECLARE_MESSAGE_MAP()

public:
	//
	// Dialog Event Methods
	//

	afx_msg void OnBnClickedCheckboxStartupApp();
	afx_msg void OnBnClickedCheckboxEnabled();
	afx_msg void OnBnClickedCheckboxMinimizeToTray();
	afx_msg void OnBnClickedButtonClose();

protected:
	HICON m_hIcon;
};
