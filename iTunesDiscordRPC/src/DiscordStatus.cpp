#include "pch.h"
#include "include/DiscordStatus.h"
#include "include/DiscordAlbumAssets.h"
#include "..\AlbumExportHelper\AlbumExportTool.h"

#pragma comment ( lib, "discord-rpc.lib" )


//------------------------------------------------------------------------
//
// Global Variables
//
//------------------------------------------------------------------------
HANDLE				g_hAppDisabledEvent;
DiscordRichPresence g_DiscordRichPresence{};
CRITICAL_SECTION	g_CritSection;
SIZE_T				g_nLastUpdateTime(0);
static LPCSTR		g_lpszCurrentAppID(NULL);


static inline VOID FreeDiscordStatus();


//--------------------------------------------------
// Send Status to Discord RPC  
VOID UpdateDiscordStatus(PRPCSTATUS_DATA pStatus)
{
	LPSTR	Artist, Album;
	LPCSTR	lpszApplicationID;

	EnterCriticalSection( &g_CritSection);
	FreeDiscordStatus();

	/* NULL lpStatus means to clear the RPC status */
	if (pStatus == NULL)
	{
		Discord_ClearPresence();
		g_nLastUpdateTime = 0;

		LeaveCriticalSection( &g_CritSection);
		return;
	}

	//
	// Discord requires strings to be UTF8 with single-byte characters
	//
	Album	= SysStringToUTFMultiByte(pStatus->AlbumName);
	Artist	= SysStringToUTFMultiByte(pStatus->Artist);

	//
	// Set the Discord RPC status info
	//
	g_DiscordRichPresence.startTimestamp	= pStatus->Time;
	g_DiscordRichPresence.details			= SysStringToUTFMultiByte(pStatus->SongName);
	g_DiscordRichPresence.largeImageKey		= SysStringToUTFMultiByte(pStatus->AlbumAssetName);
	g_DiscordRichPresence.largeImageText	= Album;

	/* Combine album & artist into single string */
	if (Artist && Album)
	{
		CHAR szState[256];
		StringCbPrintfA(szState, sizeof(szState), "by %s on '%s'", Artist, Album);

		g_DiscordRichPresence.state = new CHAR[strlen(szState) + 1];
		CopyMemory((char*)g_DiscordRichPresence.state, szState, strlen(szState) + 1);
	}

	SAFE_STR_DELETE(Artist);


	//
	// Locate the application ID that contains the current album image asset
	//
	lpszApplicationID = NULL;
	for (int i = 0; i < ARRAYSIZE(g_DiscordAppsAlbums); ++i)
	{
		for (int k = 0; k < ARRAYSIZE(g_DiscordAppsAlbums[i].AlbumNames); ++k)
		{
			if (g_DiscordAppsAlbums[i].AlbumNames[k] && (0 == _stricmp(g_DiscordAppsAlbums[i].AlbumNames[k], Album)))
			{
				lpszApplicationID = g_DiscordAppsAlbums[i].ApplicationId;
				break;
			}
		}
	}

	/* Album asset not found -- Use the default app id that contains generic iTunes logo image asset */
	if (lpszApplicationID == NULL)
	{
		lpszApplicationID					= DEFAULT_DISCORD_APPLICATION_ID;
		g_DiscordRichPresence.largeImageKey = DEFAULT_DISCORD_APP_IMAGE_KEY;
	}


	//
	// Initialize Discord RPC with the app id if needed
	//
	if (g_lpszCurrentAppID != lpszApplicationID)
	{
		if (g_lpszCurrentAppID != NULL)
		{
			/* Shutdown the current RPC to initialize w/ new app id */
			Discord_ClearPresence();
			Discord_Shutdown();
		}

		Discord_Initialize(lpszApplicationID, NULL, 1, NULL);
	}

	/* Send the RPC status */
	Discord_UpdatePresence(&g_DiscordRichPresence);
	Discord_RunCallbacks();

	g_nLastUpdateTime	= GetTickCount64();
	g_lpszCurrentAppID	= lpszApplicationID;

	LeaveCriticalSection( &g_CritSection);
}

//--------------------------------------------------
// Propery Cleanup g_DiscordRichPresence Members
static inline VOID FreeDiscordStatus()
{
	SAFE_STR_DELETE(g_DiscordRichPresence.details);
	SAFE_STR_DELETE(g_DiscordRichPresence.state);
	SAFE_STR_DELETE(g_DiscordRichPresence.smallImageText);
	SAFE_STR_DELETE(g_DiscordRichPresence.largeImageText);

	if (g_DiscordRichPresence.largeImageKey != DEFAULT_DISCORD_APP_IMAGE_KEY) {
		SAFE_STR_DELETE(g_DiscordRichPresence.largeImageKey);
	}
	if (g_DiscordRichPresence.smallImageKey != DEFAULT_DISCORD_APP_IMAGE_KEY) {
		SAFE_STR_DELETE(g_DiscordRichPresence.smallImageKey);
	}

	ZeroMemory(&g_DiscordRichPresence, sizeof(DiscordRichPresence));
}