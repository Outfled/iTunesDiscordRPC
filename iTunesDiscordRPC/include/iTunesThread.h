#pragma once

extern HANDLE	g_hITunesClosingEvent;

DWORD WINAPI iTunesHandlerThread( LPVOID );

VOID InvokeITunesEvent();