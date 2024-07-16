#include "pch.h"
#include "framework.h"
#include "include/iTunesEvents.h"
#include "include/DiscordStatus.h"
#include "include/iTunesThread.h"
#include "include/iTunes/Interface_i.c"
#include "..\AlbumExportHelper\AlbumExportTool.h"

#include <PathCch.h>
#include <time.h>

#pragma comment( lib, "Pathcch.lib" )

//#define WINDOWS_TICK					10000000
//#define EPOCH_DIFFERENCE				11644473600LL

#define CopyString(Source, Dest)		( CopyMemory(Dest, Source, (wcslen(Source) + 1) * sizeof(wchar_t)))
#define SAFE_STR_FREE(str)				if (str != NULL) SysFreeString(str); (str) = NULL;


static const IID IID_IiTunesEvents = { 0x429DD3C8, 0x703E, 0x4188, { 0x96, 0xE, 0xA9, 0x82, 0x1F, 0x14, 0xB0, 0x4C } };

static VOID PrepareEventUpdate(IITTrack * Track, BOOL Paused = FALSE);


//--------------------------------------------------
// CiTunesEvents Constructor
CiTunesEvents::CiTunesEvents()
{
	EnableAutomation();
}

//--------------------------------------------------
// CiTunesEvents Destructor
CiTunesEvents::~CiTunesEvents()
{
}

//--------------------------------------------------
// CiTunesEvents Message Map
BEGIN_MESSAGE_MAP(CiTunesEvents, CCmdTarget)

END_MESSAGE_MAP()

//--------------------------------------------------
//  Add IiTunesEvent Interface Support for the CiTunesEvents Object
BEGIN_INTERFACE_MAP(CiTunesEvents, CCmdTarget)
	INTERFACE_PART(CiTunesEvents, IID_IiTunesEvents, Dispatch) //
END_INTERFACE_MAP()


//---------------------------------------------------------------------------
// 
// Expose the CiTunesEvents Event Methods
//
//---------------------------------------------------------------------------
BEGIN_DISPATCH_MAP( CiTunesEvents, CCmdTarget )
	//{{AFX_DISPATCH_MAP(CiTunesEventHandler)
	DISP_FUNCTION( CiTunesEvents, "OnDatabaseChangedEvent",
		OnDatabaseChangedEvent, VT_EMPTY, VTS_VARIANT VTS_VARIANT )
	DISP_FUNCTION( CiTunesEvents, "OnPlayerPlayEvent",
		OnPlayerPlayEvent, VT_EMPTY, VTS_VARIANT )
	DISP_FUNCTION( CiTunesEvents, "OnPlayerStopEvent",
		OnPlayerStopEvent, VT_EMPTY, VTS_VARIANT )
	DISP_FUNCTION( CiTunesEvents, "OnPlayerPlayingTrackChangedEvent",
		OnPlayerPlayingTrackChangedEvent, VT_EMPTY, VTS_VARIANT )
	DISP_FUNCTION( CiTunesEvents, "OnUserInterfaceEnabledEvent",
		OnUserInterfaceEnabledEvent, VT_EMPTY, VTS_NONE )
	DISP_FUNCTION( CiTunesEvents, "OnCOMCallsDisabledEvent",
		OnCOMCallsDisabledEvent, VT_EMPTY, VTS_I2 )
	DISP_FUNCTION( CiTunesEvents, "OnCOMCallsEnabledEvent",
		OnCOMCallsEnabled, VT_EMPTY, VTS_NONE )
	DISP_FUNCTION( CiTunesEvents, "OnQuittingEvent", OnQuittingEvent,
		VT_EMPTY, VTS_NONE )
	DISP_FUNCTION( CiTunesEvents, "OnAboutToPromptUserToQuitEvent",
		OnAboutToPromptUserToQuitEvent, VT_EMPTY, VTS_NONE )
	DISP_FUNCTION( CiTunesEvents, "OnSoundVolumeChangedEvent",
		OnSoundVolumeChangedEvent, VT_EMPTY, VTS_I4 )
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

//--------------------------------------------------
// Fired when the iTunes Database is Changed
VOID CiTunesEvents::OnDatabaseChangedEvent(CONST VARIANT &DeletedObjectIDs, CONST VARIANT &ChangedObjectIDs)
{
	IITTrack		*Track;
	ITPlayerState	State;

	if (g_ItunesConnection)
	{
		g_ItunesConnection->get_PlayerState( &State );
		g_ItunesConnection->get_CurrentTrack( &Track );
		if (Track)
		{
			PrepareEventUpdate( Track, (State == ITPlayerStateStopped) );
		}
	}
}

//--------------------------------------------------
// Fired when a Track (Song) Begins Playing on iTunes
VOID CiTunesEvents::OnPlayerPlayEvent(CONST VARIANT &iTrack)
{
	IITTrack *Track;
	IDispatch *Source;

	Source = iTrack.pdispVal;
	if (Source)
	{
		Source->QueryInterface(&Track);
		if (Track) {
			PrepareEventUpdate(Track, FALSE);
		}

		SafeRelease(&Track);
	}

	SafeRelease(&Source);
}

//--------------------------------------------------
// Fired when a Track (Song) Stops Playing on iTunes
VOID CiTunesEvents::OnPlayerStopEvent(CONST VARIANT &iTrack)
{
	IITTrack	*Track;
	IDispatch	*Source;

	Source = iTrack.pdispVal;
	if (Source)
	{
		Source->QueryInterface( &Track );
		if (Track) {
			PrepareEventUpdate( Track, TRUE );
		}

		SafeRelease(&Track);
	}

	SafeRelease(&Source);
}

//--------------------------------------------------
// Fired when Information About the Currently Playing Track (Song) has Changed
VOID CiTunesEvents::OnPlayerPlayingTrackChangedEvent(CONST VARIANT &iTrack)
{
	IITTrack	*Track;
	IDispatch	*Source;

	Source = iTrack.pdispVal;
	if (Source)
	{
		Source->QueryInterface( &Track );
		if (Track) {
			PrepareEventUpdate( Track );
		}

		SafeRelease(&Track);
	}

	SafeRelease(&Source);
}

VOID CiTunesEvents::OnUserInterfaceEnabledEvent()
{
}

//--------------------------------------------------
// Fired when Calls to the iTunes COM Interface Will be Deferred
VOID CiTunesEvents::OnCOMCallsDisabledEvent( SHORT sReason )
{
}

//--------------------------------------------------
// Fired when Calls to the iTunes COM Interface Will no Longer be Deferred
VOID CiTunesEvents::OnCOMCallsEnabled()
{
}

//--------------------------------------------------
// Fired when iTunes (Application) is About to Quit
VOID CiTunesEvents::OnQuittingEvent()
{
	SetEvent(g_hITunesClosingEvent);
}

//--------------------------------------------------
// Fired when iTunes (Application) is About to Prompt the User to Quit
VOID CiTunesEvents::OnAboutToPromptUserToQuitEvent()
{
	SetEvent( g_hITunesClosingEvent );
}

//--------------------------------------------------
// Fired when the iTunes Sound Output Volume has Changed
VOID CiTunesEvents::OnSoundVolumeChangedEvent( LONG NewVolume )
{
	IITTrack		*Track;
	ITPlayerState	State;

	if (g_ItunesConnection)
	{
		g_ItunesConnection->get_PlayerState( &State );
		g_ItunesConnection->get_CurrentTrack( &Track );

		if (Track)
		{
			PrepareEventUpdate( Track, (State == ITPlayerStateStopped) ? TRUE : FALSE );
		}
	}
}

//--------------------------------------------------
// Get iTunes Event Update & Prepare Data for RPC Status
VOID PrepareEventUpdate(IITTrack *Track, BOOL Paused)
{
	RPCSTATUS_DATA	Status;
	LONG			lDuration;
	BSTR			strValue;
	
	ZeroMemory(&Status, sizeof(RPCSTATUS_DATA));

	/* App is disabled or paused -- Send empty status to clear RPC */
	if (WAIT_OBJECT_0 == WaitForSingleObject(g_hAppDisabledEvent, 0) || Paused)
	{
		UpdateDiscordStatus(NULL);
		Track->Release();

		return;
	}

	//-------------------------
	// Song name
	if (SUCCEEDED(Track->get_Name(&strValue)))
	{
		if (wcslen(strValue) == 1)
		{
			WCHAR szBuf[3];
			StringCbPrintf(szBuf, sizeof(szBuf), L" %c", *strValue);

			Status.SongName = SysAllocString(szBuf);
		}
		else {
			Status.SongName = SysAllocString(strValue);
		}
		SysFreeString(strValue);
	}

	//-------------------------
	// Artist
	if (SUCCEEDED(Track->get_Artist(&strValue)))
	{
		Status.Artist = SysAllocString(strValue);
		SysFreeString(strValue);
	}

	//-------------------------
	// Album name / Album asset name
	if (SUCCEEDED(Track->get_Album(&strValue)))
	{
		Status.AlbumName = SysAllocString(strValue);

		//
		// Convert album name to the Discord Application asset name
		//
		WCHAR szAssetNameBuffer[MAX_PATH]{};
		for (SIZE_T nOriginal = 0, nBuffer = 0; nOriginal < wcslen(strValue) && nBuffer < MAX_PATH; ++nOriginal, ++nBuffer)
		{
			WCHAR chCurrent = strValue[nOriginal];

			/* Check if character is not allowed */
			if (!DiscordAssetCharacterAllowed(chCurrent))
			{
				/* Invalid characters are replaced with underscores when uploading discord assets */
				szAssetNameBuffer[nBuffer] = L'_';

				/* Add extra underscore if the current character is a colon & the next character is space */
				if (chCurrent == ':' && strValue[nOriginal + 1] == ' ')
				{
					if (nBuffer + 1 < MAX_PATH) {
						szAssetNameBuffer[++nBuffer] = L'_';
					}
				}

				/* Continue loop if the current & next character is a space */
				else if (chCurrent == ' ' && strValue[nOriginal + 1] == ' ') {
					continue;
				}

				/* Go to the next valid character in the album name */
				++nOriginal;
				while (!DiscordAssetCharacterAllowed(strValue[nOriginal]))
				{
					if (++nOriginal >= wcslen(strValue)) {
						break;
					}
				}

				--nOriginal;
				continue;
			}

			//
			// The current character is allowed
			// Discord automatically sets all asset name characters to lowercase when uploading
			//
			szAssetNameBuffer[nBuffer] = towlower(strValue[nOriginal]);
		}

		SysFreeString(strValue);
		Status.AlbumAssetName = SysAllocString(szAssetNameBuffer);
	}

	//-------------------------
	// Song duration & player position
	if (SUCCEEDED(Track->get_Duration(&lDuration)))
	{
		/* Get song position in Unix Epoch time */
		LONG nPosition;
		if (g_ItunesConnection && SUCCEEDED(g_ItunesConnection->get_PlayerPosition(&nPosition)))
		{
			Status.Time = time_t(time(0) - nPosition);
		}
	}

	/* Send discord status */
	UpdateDiscordStatus(&Status);

	//
	// Cleanup
	//
	SAFE_STR_FREE(Status.SongName);
	SAFE_STR_FREE(Status.Artist);
	SAFE_STR_FREE(Status.AlbumName);
	SAFE_STR_FREE(Status.AlbumAssetName);
}
