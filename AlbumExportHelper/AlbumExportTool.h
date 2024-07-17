#pragma once

#include <Windows.h>

#ifdef _AET_FULL_

#include "iTunes/interface.h"
#include <vector>
#include <tuple>
#include <string>
#include <strsafe.h>

#pragma message( "APP_DIR == " APP_DIR)

#define DISCORD_APP_MAX_ASSETS				300
#define DISCORD_ASSETS_HEADER_FILE_NAME		L"DiscordAlbumAssets.h"
#define DISCORD_ASSETS_HEADER_FILE_NAME2	"DiscordAlbumAssets.h"

#define CONSOLE_PRINT_HEADER_MSG    "iTunesDiscordRPC Album Export Helper tool for displaying album cover images on user profile (RPC)\n" \
                                    "For a more detailed guide, watch the youtube video: \n\n"

/* Generated DiscordAlbumAssets.h file header */
static const char g_pszAlbumExportFileHeader[] = {
	"/* !! DON'T MODIFY UNLESS YOU KNOW WHAT YOU ARE DOING !! */\n"
	"#pragma once\n"
	"\n"
	"/* Includes generic iTunes logo for any songs that dont have a corresponding album asset */\n"
	"#define DEFAULT_DISCORD_APPLICATION_ID		\"1095131480147103744\"\n"
	"#define DEFAULT_DISCORD_APP_IMAGE_KEY		 \"small-icon\"\n"
	"\n"
	"struct DiscordAppAlbumAssets\n"
	"{\n"
	"	 const char* ApplicationId;\n"
	"	 const char* AlbumNames[300];\n"
	"};\n"
	"\n"
};

typedef std::vector<LPSTR>			MBStringVector_t;
typedef std::tuple<LONG, LPSTR>		AlbumTuple_t;
typedef std::vector<AlbumTuple_t>	AlbumInfoVector_t;


//
// AlbumExportTool Functions
// 

int AETCreateAssetData();
int AETUpdateAssetData();


//
// Helper functions
//

HRESULT GetiTunesLibraryAlbums(IITTrackCollection *, AlbumInfoVector_t &);
BSTR	ValidateAndConvertAlbumName(LPCSTR);
LPSTR	FixQuotesInAlbumName(LPCSTR);

static inline void GetDiscordAssetsFileName(LPSTR lpszBuffer, SIZE_T cbBuffer)
{
	StringCbPrintfA(lpszBuffer,
		cbBuffer,
		"%s\\include\\%s",
		APP_DIR,
		DISCORD_ASSETS_HEADER_FILE_NAME2
	);
}
static inline void GetDiscordAssetsFileName(LPWSTR lpszBuffer, SIZE_T cchBuffer)
{
	DWORD	cchNeeded;
	LPWSTR	lpszSolutionDir;

	cchNeeded = MultiByteToWideChar(CP_UTF8, 0, APP_DIR, -1, NULL, 0);
	if (cchNeeded > 0)
	{
		lpszSolutionDir = new WCHAR[cchBuffer];
		if (MultiByteToWideChar(CP_UTF8, 0, APP_DIR, -1, lpszSolutionDir, (int)cchBuffer))
		{
			StringCbPrintf(lpszBuffer,
				cchBuffer * sizeof(WCHAR),
				L"%s\\include\\%s",
				lpszSolutionDir,
				DISCORD_ASSETS_HEADER_FILE_NAME
			);
		}

		delete[] lpszSolutionDir;
	}
}

static inline VOID FreeStringVector(MBStringVector_t &rgStrings)
{
	for (auto eString : rgStrings) {
		if (eString) {
			delete[] eString;
		}
	}

	rgStrings.clear();
}
static inline VOID FreeStringVector(AlbumInfoVector_t &rgAlbumInfoStrings)
{
	for (auto eAlbumInfo : rgAlbumInfoStrings)
	{
		if (std::get<1>(eAlbumInfo)) {
			delete[] std::get<1>(eAlbumInfo);
		}
	}

	rgAlbumInfoStrings.clear();
}

#endif //_AET_FULL_

/* Regex for Discord Application asset names */
#define DiscordAssetCharacterAllowed(c)		\
	( (c == L'_' || c == L'-') || (c >= 48 && ((c >= 97 && c <= 122 ) || (c <= 57 ) || (c >= 65 && c <= 90))) )

/* BSTR to (UTF-8) LPSTR */
static inline LPSTR SysStringToUTFMultiByte(BSTR pszString)
{
	LPSTR lpszMultibyte;
	DWORD cbSizeNeeded;

	lpszMultibyte = NULL;

	cbSizeNeeded = WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, pszString, -1, NULL, 0, NULL, NULL);
	if (cbSizeNeeded > 0)
	{
		lpszMultibyte = new CHAR[cbSizeNeeded];
		WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, pszString, -1, lpszMultibyte, cbSizeNeeded, NULL, NULL);
	}

	return lpszMultibyte;
}

