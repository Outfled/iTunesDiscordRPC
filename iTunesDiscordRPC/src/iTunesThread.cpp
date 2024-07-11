#include "pch.h"
#include "framework.h"
#include "include/iTunesThread.h"
#include "include/iTunesObject.h"
#include "include/DiscordStatus.h"
#include "include/AppTray.h"

#include <TlHelp32.h>

#define ITUNES_EXE_NAME		L"iTunes.exe"
#define ADMIN_REQUIRED(hr)	(hr == HRESULT_FROM_WIN32(ERROR_ELEVATION_REQUIRED))


//---------------------------------------------------------------------------
// 
// Globals
//
//---------------------------------------------------------------------------
IiTunes			*g_ItunesConnection;
HANDLE			g_hITunesClosingEvent;
static HANDLE	g_hInvokeEvent;


static BOOL iTunesProcessFound();


//--------------------------------------------------
// Main Thread for Connecting to iTunes Application
DWORD WINAPI iTunesHandlerThread( LPVOID Parameter )
{
	CWnd			*DlgWnd;
	HRESULT			hResult;
	CiTunesObject	*Object;

	DlgWnd	= (CWnd *)Parameter;
	hResult = S_OK;

	/* Create event for invoking iTunes event */
	g_hInvokeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	/* Initialize COM for multithreaded use */
	hResult = CoInitializeEx( NULL, COINITBASE_MULTITHREADED );
	if (FAILED( hResult ))
	{
		TerminateApplication( DlgWnd, L"An unknown error occurred", hResult );
	}

	while (hResult == S_OK)
	{
		LONG			cVolume;
		ITPlayerState	iState;

		Object				= new CiTunesObject;
		g_ItunesConnection	= NULL;

		/* Wait for iTunes to Open */
		while (FALSE == iTunesProcessFound())
		{
			Sleep( 100 );
		}

		Sleep( 1000 );

		/* Connect to iTunes */
		g_ItunesConnection = Object->GetConnection();
		if (g_ItunesConnection == NULL)
		{
			if (ADMIN_REQUIRED( Object->hResult ))
			{
				/* Administrative rights are needed */
				DlgWnd->MessageBox(
					L"iTunes is running as administrator\nResart this program as adminstrator to fix error.",
					L"Error",
					MB_OK 
				);
			}
			else
			{
				/* Unknown error occurred */
				TerminateApplication( DlgWnd, L"An unknown error occurred:", Object->hResult );
			}

			ExitProcess( Object->hResult );
		}

		/* Cause an immediate event by lowering the volume by 1 and then revert back to original */
		g_ItunesConnection->get_PlayerState( &iState );
		if (iState == ITPlayerStatePlaying)
		{
			g_ItunesConnection->get_SoundVolume( &cVolume );
			g_ItunesConnection->put_SoundVolume( ++cVolume );
			g_ItunesConnection->put_SoundVolume( --cVolume );
		}

		/* Wait for iTunes to close */
		while (WAIT_OBJECT_0 != WaitForSingleObject( g_hITunesClosingEvent, 1 ))
		{
			Sleep( 250 );

			if (g_ItunesConnection == nullptr) {
				break;
			}

			//
			// Periodically notify discord to update the presence to ensure iTunesDiscordRPC is always the active status
			//
			if (g_nLastUpdateTime != 0 && GetTickCount64() - g_nLastUpdateTime >= 5000)
			{
				if (TryEnterCriticalSection( &g_CritSection ))
				{
					//
					// Change an unused variable in the discord rich presence structure so
					//  that Discord_UpdatePresence() registers and shows the 'updated' status
					//
					g_DiscordRichPresence.partyMax = ( g_DiscordRichPresence.partyMax == 0 ) ? 1 : 0;

					Discord_UpdatePresence( &g_DiscordRichPresence );
					Discord_RunCallbacks();

					g_nLastUpdateTime = GetTickCount64();
					LeaveCriticalSection( &g_CritSection);
				}
			}

			/* Cause an immediate event by lowering the volume by 1 and then revert back to original */
			if (WaitForSingleObject(g_hInvokeEvent, 0) == WAIT_OBJECT_0)
			{
				LONG lVolume;

				g_ItunesConnection->get_SoundVolume(&lVolume);
				g_ItunesConnection->put_SoundVolume(++lVolume);

				Sleep(5);
				g_ItunesConnection->put_SoundVolume(--lVolume);
			}
		}

		/* App is exiting */
		if (g_ItunesConnection == nullptr && WAIT_OBJECT_0 != WaitForSingleObject( g_hITunesClosingEvent, 1 ))
		{
			/* Cleanup connection object */
			delete Object;
			ExitThread( 0 );
		}

		/* Clear the iTunes connection */
		SafeRelease( &g_ItunesConnection );
		delete Object;

		/* iTunes application has closed -- Clear the discord status */
		UpdateDiscordStatus( NULL );

		/* iTunes.exe may take some time to close -- wait for close before continue */
		while (iTunesProcessFound())
		{
			Sleep( 0 );
		}

		ResetEvent( g_hITunesClosingEvent );
	}

	return 0;
}

//--------------------------------------------------
// Simulate iTunes Event
VOID InvokeITunesEvent()
{
	SetEvent(g_hInvokeEvent);
}

//--------------------------------------------------
// Check if iTunes.exe Proecess Found
BOOL iTunesProcessFound()
{
	HANDLE			Snapshot;
	PROCESSENTRY32	EntryData;
	BOOL			Result;

	Result = FALSE;

	Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (Snapshot != INVALID_HANDLE_VALUE)
	{
		EntryData.dwSize = sizeof(EntryData);

		Result = Process32First(Snapshot, &EntryData);
		if (Result)
		{
			Result = FALSE;

			do
			{
				if (0 == _wcsicmp(EntryData.szExeFile, ITUNES_EXE_NAME))
				{
					Result = TRUE;
					break;
				}
			} while (Process32Next(Snapshot, &EntryData));
		}

		CloseHandle(Snapshot);
	}

	return Result;
}