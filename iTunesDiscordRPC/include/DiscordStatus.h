#pragma once

#include "include/Discord/RPC.h"

#define SAFE_STR_DELETE(str)				if (str != NULL) delete[] str;

typedef struct _RPCSTATUS_DATA_
{
	BSTR	SongName;			/* DiscordRichPresence::details */
	BSTR	Artist;				/* DiscordRichPresence::state */
	BSTR	AlbumName;			/* DiscordRichPresence::state && DiscordRichPresence::largeImageText */
	BSTR	AlbumAssetName;		/* DiscordRichPresence::largeImageKey */
	SIZE_T	Time;				/* DiscordRichPresence::startTimestamp */
}RPCSTATUS_DATA, *PRPCSTATUS_DATA;


extern HANDLE				g_hAppDisabledEvent;
extern SIZE_T				g_nLastUpdateTime;
extern DiscordRichPresence	g_DiscordRichPresence;
extern CRITICAL_SECTION		g_CritSection;

VOID UpdateDiscordStatus(PRPCSTATUS_DATA pStatus );