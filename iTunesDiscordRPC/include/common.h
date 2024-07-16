/* Safe Release COM Objects */
template<typename T> static inline void SafeRelease(T **pp)
{
	if (pp && *pp)
	{
		(*pp)->Release();
		*pp = NULL;
	}
}

#ifdef __AFXWIN_H__
inline void TerminateApplication(CWnd *pWnd, LPCWSTR lpszMessage, DWORD dwError)
{
	WCHAR szErrorBuf[1024];

	StringCbPrintf(szErrorBuf, sizeof(szErrorBuf), L"%s %lu", lpszMessage, dwError);
	if (dwError != 0) {
		StringCbPrintf(szErrorBuf, sizeof(szErrorBuf), L"%s %lu", lpszMessage, dwError);
	}
	else {
		StringCbCopy(szErrorBuf, sizeof(szErrorBuf), lpszMessage);
	}

	pWnd->MessageBox(szErrorBuf, L"Error Occurred", MB_OK);
	ExitProcess(0);
}
#endif //__AFXWIN_H__
