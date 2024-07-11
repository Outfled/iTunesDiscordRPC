#define _AET_FULL_

#include <Windows.h>
#include <iostream>
#include <comdef.h>
#include <Shlwapi.h>
#include "AlbumExportTool.h"
#include "iTunes/Interface_i.c"
#include "common.h"


#pragma comment( lib, "Shlwapi.lib" )
#pragma warning( disable : 6031 )   /* Return value ignored (getchar())*/

#define FINAL_STEPS_PRINTF(DiscordAppIds)           \
{   \
    printf("    For each Discord Application you previously created, select it from the Discord Application Portal and then "   \
        "take note of the application id from the general information tab\n");  \
    printf("      1) On the left, navigate to Rich Presence -> Art Assets\n");   \
    printf("      2) Click the Add Image(s) button and navigate to the current directory & find the folder name that is the same " \
        "as the respective Discord Application id\n"); \
    printf("      3) Select all the images in the folder. Once finished, there should be a green 'Save Changes' button " \
        "(!! DONT CHANGE ANY OF THE ASSET KEY NAMES UNLESS YOU KNOW WHAT YOU ARE DOING !!)\n"); \
    printf("      4) Press the 'Save Changes' button and WAIT\n");   \
    printf("    If you start getting messages saying 'You are being rate limited.' or 'Oops! You've caught an ultra rare error.', " \
        "wait a few seconds and press the 'Save Changes' button again." \
        "(You will probably have to repeat a few times before the error message(s) go away)\n");  \
    if (DiscordAppIds.size() > 1)    \
    {   \
        printf("Note: Remember you need to complete the final steps for all %d applications; "  \
            "make sure youre uploading from the folder that has the same name as the corresponding application id\n",   \
            (ULONG)DiscordAppIds.size()  \
        );  \
    }   \
    printf("\nAfter all uploads have completed, you are done. It may take up to a few minutes before the images show up on your RPC status\n"); \
}


struct DiscordAlbumAssetFileData
{
    LPSTR               HeaderFileData;
    LONGLONG            FileLength;
    MBStringVector_t    DiscordApplicationIds;
    MBStringVector_t    *ApplicationAssets;

    DiscordAlbumAssetFileData() : HeaderFileData(NULL), FileLength(0), ApplicationAssets(NULL)
    {}
    ~DiscordAlbumAssetFileData();
};


//---------------------------------------------------------------------------
// 
// Helper Function Declarations
//
//---------------------------------------------------------------------------
static MBStringVector_t RetrieveUserDiscordAppIds(int);
static HRESULT          ExportDiscordAppsAlbumAssets(IITTrackCollection*, MBStringVector_t, AlbumInfoVector_t);
static BOOL             DeleteDirectory(LPCSTR);
static PSTR             DuplicateString(LPCSTR);
static BOOL             ReadDiscordAlbumAssetFile(DiscordAlbumAssetFileData *pFileData);
static BOOL             AddAlbumsToExistingApplication(DiscordAlbumAssetFileData&, AlbumInfoVector_t);
static BOOL             AddAlbumsToNewApplication(
    SIZE_T *,
    IITTrackCollection *,
    LPCSTR,
    AlbumInfoVector_t,
    PBYTE *,
    DWORD *,
    DiscordAlbumAssetFileData *
);


int main()
{ 
    std::string szOption;

    printf(CONSOLE_PRINT_HEADER_MSG);

    printf("1) Generate & export (create) RPC album data (Choose if first time running program OR you want to redo)\n");
    printf("2) Update RPC Album Data (Only choose if you have run this tool before (with option 1) AND *successfully* followed the steps)\n");

    printf("\nSelect Option<1-2>: ");
    getline(std::cin, szOption);

    if (0 == szOption.compare("1"))
    {
        system("cls");
        return AETCreateAssetData();
    }
    else if (0 == szOption.compare("2"))
    {
        system("cls");
        return AETUpdateAssetData();
    }

    printf("Error: Invalid option selected\n");

    getchar();
    return 1;
}


DiscordAlbumAssetFileData::~DiscordAlbumAssetFileData()
{
    FreeStringVector(DiscordApplicationIds);
    if (ApplicationAssets)
    {
        for (int i = 0; i < ApplicationAssets->size(); ++i)
        {
            FreeStringVector(ApplicationAssets[i]);
        }

        delete[] ApplicationAssets;
    }
    if (HeaderFileData)
    {
        delete[] HeaderFileData;
    }
}

//--------------------------------------------------
// Export Albums from iTunes Library to (Image) Files & to the DiscordAlbumAssets.h Header File
int AETCreateAssetData()
{
    HRESULT				hResult;
    IiTunes             *pITunes;
    IITLibraryPlaylist  *pLibrary;
    IITTrackCollection  *pTracks;
    MBStringVector_t    rgApplicationIds;
    AlbumInfoVector_t   rgLibraryAlbums;

    pTracks = NULL;

    printf(CONSOLE_PRINT_HEADER_MSG);
    printf("Tool Usage: First time running\n\n");

    //
    // Connect to iTunes process through COM
    //
    hResult = CoInitializeEx(NULL, COINITBASE_MULTITHREADED);
    if (FAILED(hResult))
    {
        printf("[Error]: Failed to initialize COM (%ls)\n", _com_error(hResult).ErrorMessage());

        getchar();
        return 1;
    }

    hResult = CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pITunes));
    if (hResult != S_OK)
    {
        printf("[Error]: Failed to connect to iTunes process. Double check the following:\n");
        printf("1) iTunes (.exe) is opened\n");
        printf("2) This process is NOT running as administrator UNLESS iTunes.exe is running as administrator\n");
        CoUninitialize();

        getchar();
        return 1;
    }

    printf("Connected to iTunes process\n");


    //
    // Scan the iTunes library for list of all album names
    //
    printf("Scanning all albums in iTunes library...\n\n");

    hResult = pITunes->get_LibraryPlaylist(&pLibrary);
    if (SUCCEEDED(hResult)) {
        hResult = pLibrary->get_Tracks(&pTracks);
    }

    if (!pTracks)
    {
        if (pLibrary) {
            pLibrary->Release();
        }
        pITunes->Release();

        printf("[Error]: Failed to retrieve albums in the user library (%ls)\n", _com_error(hResult).ErrorMessage());
        CoUninitialize();

        getchar();
        return 1;
    }

    hResult = GetiTunesLibraryAlbums(pTracks, rgLibraryAlbums);
    if (hResult != S_OK)
    {
        if (pLibrary) {
            pLibrary->Release();
        }
        pITunes->Release();
        
        printf("[Error]: Failed to retrieve albums in the user library (%ls)\n", _com_error(hResult).ErrorMessage());
        CoUninitialize();

        getchar();
        return 1;
    }

    pLibrary->Release();


    //
    // Retrieve the Discord application id(s)
    //
    int nAppIds = 0;
    if (rgLibraryAlbums.size() <= DISCORD_APP_MAX_ASSETS) {
        nAppIds = 1;
    }
    else if (rgLibraryAlbums.size() % DISCORD_APP_MAX_ASSETS == 0) {
        nAppIds = (int)rgLibraryAlbums.size() / DISCORD_APP_MAX_ASSETS;
    }
    else {
        nAppIds = (int)(rgLibraryAlbums.size() / DISCORD_APP_MAX_ASSETS) + 1;
    }

    rgApplicationIds = RetrieveUserDiscordAppIds(nAppIds);


    //
    // Save each album cover image to file & generate the header file
    //
    printf("Generating album images for Discord applications; avoid interacting with iTunes window to prevent hangs\n");
    printf("This may take a few minutes based on album count...\n");

    hResult = ExportDiscordAppsAlbumAssets(pTracks, rgApplicationIds, rgLibraryAlbums);
    if (FAILED(hResult))
    {
        printf("[Error]: Failed to export album images/generate header file (Error: %ls)\n", _com_error(hResult).ErrorMessage());

        pTracks->Release();
        pITunes->Release();
        CoUninitialize();

        FreeStringVector(rgApplicationIds);
        FreeStringVector(rgLibraryAlbums);

        getchar();
        return 1;
    }

    MessageBeep(MB_ICONEXCLAMATION);

    pTracks->Release();
    pITunes->Release();
    FreeStringVector(rgLibraryAlbums);

    printf("Tool complete. Final steps below:\n");
    FINAL_STEPS_PRINTF(rgApplicationIds);

    //
    // Cleanup
    //
    FreeStringVector(rgApplicationIds);
    CoUninitialize();

    printf("Press enter to exit ");
    getchar();

    return 0;
}

//--------------------------------------------------
// Export Albums from iTunes Library that are not currently included in the DiscordAlbumAssets.h Header File
int AETUpdateAssetData()
{
    BOOL                        bSuccess;
    DiscordAlbumAssetFileData   AlbumAssetData;
    int                         nCurrentApplications;
    HRESULT				        hResult;
    IiTunes                     *pITunes;
    IITLibraryPlaylist          *pLibrary;
    IITTrackCollection          *pTracks;
    AlbumInfoVector_t           rgUpdatedAlbumData;

    pTracks = NULL;

    printf(CONSOLE_PRINT_HEADER_MSG);
    printf("Tool Usage: Update Existing (Discord Application) Album Asset Data\n\n");

    //
    // Find all current discord applications and their album assets
    //
    bSuccess = ReadDiscordAlbumAssetFile(&AlbumAssetData);
    if (!bSuccess)
    {
        getchar();
        return 1;
    }

    nCurrentApplications = (int)AlbumAssetData.DiscordApplicationIds.size();
    if (!nCurrentApplications || !AlbumAssetData.ApplicationAssets)
    {
        printf("Error: No Discord Applications were found in the DiscordAlbumAssets.h header file\n");
        getchar();

        return 1;
    }

    printf("Imported all albums from %d Discord Applications\n\n", (ULONG)AlbumAssetData.DiscordApplicationIds.size());


    //
    // Connect to iTunes process through COM
    //
    hResult = CoInitializeEx(NULL, COINITBASE_MULTITHREADED);
    if (FAILED(hResult))
    {
        printf("[Error]: Failed to initialize COM (%ls)\n", _com_error(hResult).ErrorMessage());

        getchar();
        return 1;
    }

    hResult = CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pITunes));
    if (hResult != S_OK)
    {
        printf("[Error]: Failed to connect to iTunes process. Double check the following:\n");
        printf("1) iTunes (.exe) is opened\n");
        printf("2) This process is NOT running as administrator UNLESS iTunes.exe is running as administrator\n");
        CoUninitialize();

        getchar();
        return 1;
    }

    printf("Connected to iTunes process\n");


    //
    // Scan the iTunes library for list of all album names
    //
    printf("Getting current album list from iTunes library...\n\n");

    hResult = pITunes->get_LibraryPlaylist(&pLibrary);
    if (SUCCEEDED(hResult)) {
        hResult = pLibrary->get_Tracks(&pTracks);
    }

    if (!pTracks)
    {
        if (pLibrary) {
            pLibrary->Release();
        }
        pITunes->Release();

        printf("[Error]: Failed to retrieve albums in the user library (%ls)\n", _com_error(hResult).ErrorMessage());
        CoUninitialize();

        getchar();
        return 1;
    }

    hResult = GetiTunesLibraryAlbums(pTracks, rgUpdatedAlbumData);
    if (hResult != S_OK)
    {
        if (pLibrary) {
            pLibrary->Release();
        }
        pITunes->Release();

        printf("[Error]: Failed to retrieve albums in the user library (%ls)\n", _com_error(hResult).ErrorMessage());
        CoUninitialize();

        getchar();
        return 1;
    }

    pITunes->Release();
    pLibrary->Release();


    //
    // Locate the new albums
    //
    printf("Searching album list for any unregistered albums...\n\n");

    std::vector<size_t> rgErasePositions;
    for (size_t nUpdatedAlbum = 0; nUpdatedAlbum < rgUpdatedAlbumData.size(); ++nUpdatedAlbum)
    {
        LPSTR   lpszAlbum   = std::get<1>(rgUpdatedAlbumData[nUpdatedAlbum]);
        BOOL    bFound      = FALSE;

        for (int i = 0; i < AlbumAssetData.DiscordApplicationIds.size(); ++i)
        {
            for (auto eCurAlbumAsset : AlbumAssetData.ApplicationAssets[i])
            {
                if (0 == strcmp(lpszAlbum, eCurAlbumAsset))
                {
                    bFound = TRUE;
                    break;
                }
            }
        }

        if (bFound)
        {
            rgErasePositions.push_back(nUpdatedAlbum);
        }
    }

    if (rgErasePositions.size())
    {
        for (int i = (int)rgErasePositions.size() - 1; i >= 0; --i)
        {
            delete[] std::get<1>(rgUpdatedAlbumData[rgErasePositions[i]]);
            rgUpdatedAlbumData.erase(rgUpdatedAlbumData.begin() + rgErasePositions[i]);
        }
    }

    //
    // No new albums detected
    //
    if (rgUpdatedAlbumData.size() == 0)
    {
        printf("Search complete. No new albums were found; all albums in the iTunes library are registered to a Discord Application\n");
        printf("Press enter to exit\n");
    }

    //
    // Save the new albums
    //
    else
    {
        int                 nRequiredNewApplications;
        int                 nAlbumAssetsNeeded;
        HANDLE              hHeaderFile;
        CHAR                szAlbumAssetsFile[MAX_PATH];
        MBStringVector_t    rgCreatedAppIds;

        nRequiredNewApplications    = 0;
        nAlbumAssetsNeeded          = (int)rgUpdatedAlbumData.size();
        GetDiscordAssetsFileName(szAlbumAssetsFile, sizeof(szAlbumAssetsFile));

        //
        // Determine how many new Discord Applciations are needed
        //
        nAlbumAssetsNeeded -= (int)(DISCORD_APP_MAX_ASSETS - AlbumAssetData.ApplicationAssets[nCurrentApplications - 1].size());
        if (nAlbumAssetsNeeded <= 0) {
            nRequiredNewApplications = 0;
        }
        else if (nAlbumAssetsNeeded <= DISCORD_APP_MAX_ASSETS) {
            nRequiredNewApplications = 1;
        }
        else if (nAlbumAssetsNeeded % DISCORD_APP_MAX_ASSETS == 0) {
            nRequiredNewApplications = nAlbumAssetsNeeded / DISCORD_APP_MAX_ASSETS;;
        }
        else {
            nRequiredNewApplications = (nAlbumAssetsNeeded / DISCORD_APP_MAX_ASSETS) + 1;
        }

        printf("Search complete. A total of %d albums were found without a corresponding Discord Application\n",
            (ULONG)rgUpdatedAlbumData.size()
        );

        /* Create temporary assets header file */
        hHeaderFile = CreateFile(L"TempDiscordAlbumAssets.h", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hHeaderFile == INVALID_HANDLE_VALUE)
        {
            printf("Error: Failed to create TempDiscordAlbumAssets.h file (%ls)\n",
                _com_error(HRESULT_FROM_WIN32(GetLastError())).ErrorMessage()
            );

            FreeStringVector(rgUpdatedAlbumData);
            pTracks->Release();

            CoUninitialize();
            return 1;
        }

        //
        // Assign album assets to new application(s)
        //
        if (nRequiredNewApplications)
        {       
            SIZE_T nOffset = 0;

            //
            // Get the new Discord Application ids from user
            //
            printf("The most recent Discord Application does not have the room to fit all new albums; "
                "You need to create %d more Discord Applications\n\n",
                nRequiredNewApplications
            );
            rgCreatedAppIds = RetrieveUserDiscordAppIds(nRequiredNewApplications);

            //
            // Update the album asset header file buffer
            //
            for (int nApp = 0; nApp < nRequiredNewApplications; ++nApp)
            {
                bSuccess = AddAlbumsToNewApplication(&nOffset,
                    pTracks,
                    rgCreatedAppIds[nApp],
                    rgUpdatedAlbumData,
                    NULL,
                    NULL,
                    &AlbumAssetData
                );
                if (!bSuccess)
                {
                    printf("**Skipping Discord Application (Id: %s)**\n", rgCreatedAppIds[nApp]);
                    continue;
                }
            }

            if (nOffset == 0)
            {
                CloseHandle(hHeaderFile);

                FreeStringVector(rgUpdatedAlbumData);
                pTracks->Release();

                CoUninitialize();
                return 1;
            }
        }

        //
        // Assign album assets to the most recent application
        //
        else
        { 
            printf("The most recent Discord Application(id: %s) has enough space to fit the new / updated album assets",
                AlbumAssetData.DiscordApplicationIds[AlbumAssetData.DiscordApplicationIds.size() - 1]
            );

            bSuccess = AddAlbumsToExistingApplication(AlbumAssetData, rgUpdatedAlbumData);
            if (!bSuccess)
            {
                printf("Error: Failed to update DiscordAlbumAssets header file to contain new album names (%ls)\n",
                    _com_error(HRESULT_FROM_WIN32(GetLastError())).ErrorMessage()
                );

                CloseHandle(hHeaderFile);

                FreeStringVector(rgUpdatedAlbumData);
                pTracks->Release();

                CoUninitialize();
                return 1;
            }
        }

        FreeStringVector(rgUpdatedAlbumData);

        /* Write the updated DiscordAlbumAssets.h buffer to the temp file */
        bSuccess = WriteFile(hHeaderFile, AlbumAssetData.HeaderFileData, (DWORD)strlen(AlbumAssetData.HeaderFileData), NULL, NULL);
        if (!bSuccess)
        {
            printf("Error: Failed to write data to TempDiscordAlbumAssets.h file (%ls)\n",
                _com_error(HRESULT_FROM_WIN32(GetLastError())).ErrorMessage()
            );

            CloseHandle(hHeaderFile);

            FreeStringVector(rgUpdatedAlbumData);
            pTracks->Release();

            CoUninitialize();
            return 1;
        }

        CloseHandle(hHeaderFile);

        //
        // Replace the original DiscordAlbumAssets.h with the temp file
        //
        bSuccess = MoveFileExA("TempDiscordAlbumAssets.h", szAlbumAssetsFile, MOVEFILE_REPLACE_EXISTING);
        if (!bSuccess)
        {
            printf("Error: Failed to rename/move TempDiscordAlbumAssets.h file (%ls)\n",
                _com_error(HRESULT_FROM_WIN32(GetLastError())).ErrorMessage()
            );

            FreeStringVector(rgCreatedAppIds);

            pTracks->Release();
            CoUninitialize();

            return 1;
        }

        if (nRequiredNewApplications)
        {
            printf("Export complete. Final steps below:\n");
            FINAL_STEPS_PRINTF(rgCreatedAppIds);           
        }

        FreeStringVector(rgCreatedAppIds);
        Sleep(0);
    }

    pTracks->Release();
    getchar();

    return 0;
}


#pragma warning( push )
#pragma warning( disable : 6386 ) /* Potential buffer overrun */

//--------------------------------------------------
// Validate Any Quotation Marks in Album Name String (Used for rewriting DiscordAlbumAssets.h) 
static inline LPSTR FixQuotesInAlbumName(LPCSTR lpszAlbumName)
{
    LPSTR   lpszFixedName;
    int     nFound;

    nFound = 0;

    for (size_t i = 0; i < strlen(lpszAlbumName); ++i)
    {
        if (lpszAlbumName[i] == '\"')
        {
            ++nFound;
        }
    }

    lpszFixedName = new CHAR[strlen(lpszAlbumName) + nFound + 1]{};
    for (size_t i = 0, k = 0; i < strlen(lpszAlbumName); ++i)
    {
        if (lpszAlbumName[i] == '\"')
        {
            lpszFixedName[k++] = '\\';
        }

        lpszFixedName[k++] = lpszAlbumName[i];
    }

    return lpszFixedName;
}

#pragma warning( pop )

//--------------------------------------------------
// Get User Discord Application Ids from Console
MBStringVector_t RetrieveUserDiscordAppIds(int nRequired)
{
    MBStringVector_t rgApplicationIds;

    printf("The following steps are required to continue:\n");
    printf("    1) Login to the Discord Developer Portal w/ any existing Discord account -> https://discord.com/developers/applications \n");
    printf("    2) Create a new application by pressing the \"New Application\" button on the top right\n");
    printf("    3) Name the application 'iTunes' or 'Apple Music' (without quotes) and press create\n");
    printf("    4) Look for the Application ID under the general information tab; copy it and paste here\n");

    if (nRequired > 1)
    {
        printf("\n**Due to Discord limiting the amount of RPC image assets per application, you will "
            "need to create %d applications and paste each respective id below (each application can have the same name).**\n\n",
            nRequired
        );
    }
 
    for (int i = 0; i < nRequired; ++i)
    {
        std::string szInput;
        LPSTR       lpszApplicationId;

        if (nRequired == 1) {
            printf("\nApplication ID: ");
        }
        else {
            printf("\nApplication (%d) ID: ", i + 1);
        }

        std::getline(std::cin, szInput);
        if (szInput.size() == 0)
        {
            printf("\nInvalid ID\n");
            continue;
        }
        
        lpszApplicationId = new CHAR[szInput.length() + 1];
        CopyMemory(lpszApplicationId, szInput.c_str(), szInput.length() + 1);

        rgApplicationIds.push_back(lpszApplicationId);
    }

    printf("\n");
    return rgApplicationIds;
}

//--------------------------------------------------
// Get Array (Vector) of all Album Names in the User's iTunes Library
HRESULT GetiTunesLibraryAlbums(
    IITTrackCollection  *pLibrarySongs,
    AlbumInfoVector_t   &rgAlbumList
)
{
    LONG    nTracks;
    HRESULT hResult;

    /* Get number of songs in the library */
    hResult = pLibrarySongs->get_Count(&nTracks);
    if (FAILED(hResult)) {
        return hResult;
    }

    /* Get list of all album names */
    for (LONG iSong = 1; iSong < nTracks; ++iSong)
    {
        IITTrack *pSongTrack;

        /* Get the current song track */
        hResult = pLibrarySongs->get_Item(iSong, &pSongTrack);
        if (SUCCEEDED(hResult))
        {
            BSTR pszAlbumName;

            /* Get the album name */
            hResult = pSongTrack->get_Album(&pszAlbumName);
            if (SUCCEEDED(hResult))
            {
                LPSTR lpszMultibyte = SysStringToUTFMultiByte(pszAlbumName);
                if (lpszMultibyte)
                {
                    rgAlbumList.push_back(std::make_tuple(iSong, lpszMultibyte));
                }

                SysFreeString(pszAlbumName);
            }

            pSongTrack->Release();
        }
    }

    /* Remove any duplicate names */
    for (size_t i = 0; i < rgAlbumList.size() - 1; ++i)
    {
        std::vector<size_t> rgErasePositions;
        LPSTR               lpszCurAlbumName;

        lpszCurAlbumName = std::get<1>(rgAlbumList[i]);
        for (size_t k = i + 1; k < rgAlbumList.size(); ++k)
        {
            if (0 == strcmp(lpszCurAlbumName, std::get<1>(rgAlbumList[k])))
            {
                rgErasePositions.push_back(k);
            }
        }
        if (rgErasePositions.size())
        {
            for (size_t k = rgErasePositions.size() - 1; k != (SIZE_T)-1; --k)
            {
                delete[]  std::get<1>(rgAlbumList[rgErasePositions[k]]);
                rgAlbumList.erase(rgAlbumList.begin() + rgErasePositions[k]);
            }
        }
    }

    return S_OK;
}

//--------------------------------------------------
// Rewrite DiscordAlbumAssets.h & Export All Album Cover Images to Respective Discord Application Folder(s)
HRESULT ExportDiscordAppsAlbumAssets(
    IITTrackCollection  *pLibrarySongs,
    MBStringVector_t	DiscordApplicationIds,
    AlbumInfoVector_t	rgAlbumNames
)
{
    HRESULT             hResult;
    HANDLE              hExportFile;
    BOOL                bSuccess;
    CHAR                szBuffer[256];
    CHAR                szCurrentDirectory[MAX_PATH];
    MBStringVector_t    rgSkippedAlbums;

    GetCurrentDirectoryA(MAX_PATH, szCurrentDirectory);

    /* Create the export file */
    hExportFile = CreateFile(L"DiscordAlbumAssets.h",
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hExportFile == INVALID_HANDLE_VALUE)
    {
        hResult = HRESULT_FROM_WIN32(GetLastError());
        FreeStringVector(rgAlbumNames);

        return hResult;
    }

    //
    // Export the album images & rite the data to the export file
    //
    bSuccess = WriteFile(hExportFile, g_pszAlbumExportFileHeader, (DWORD)strlen(g_pszAlbumExportFileHeader), NULL, NULL);
    if (bSuccess)
    {
        StringCbPrintfA(szBuffer,
            sizeof(szBuffer),
            "const DiscordAppAlbumAssets g_DiscordAppsAlbums[%d] = {\n",
            (ULONG)DiscordApplicationIds.size()
        );
        bSuccess = WriteFile(hExportFile, szBuffer, (DWORD)strlen(szBuffer), NULL, NULL);
    }

    for (size_t nApplication = 0, nAlbum = 0; bSuccess && nApplication < DiscordApplicationIds.size(); ++nApplication)
    {
        MBStringVector_t    rgCurrentAppAlbums;
        LPWSTR              lpszWideAppDirectory;

        lpszWideAppDirectory = NULL;

        //
        // Create the folder to hold the application album asset images 
        //
        StringCbPrintfA(szBuffer, sizeof(szBuffer), "%s\\%s", szCurrentDirectory, DiscordApplicationIds[nApplication]);
        if (PathIsDirectoryA(szBuffer))
        {
            if (!PathIsDirectoryEmptyA(szBuffer) && !DeleteDirectory(szBuffer))
            {
                printf("*Failed to delete existing files in the album export folder \"%s\" --", szBuffer);
                printf("Album images for the application (id: %s) will not be exported*\n", DiscordApplicationIds[nApplication]);
            }

            RemoveDirectoryA(szBuffer);
        }

        bSuccess = CreateDirectoryA(szBuffer, NULL);
        if (!bSuccess)
        {
            break;
        }

        /* Convert the folder path string to wide */
        DWORD cchPath = MultiByteToWideChar(CP_UTF8, 0, szBuffer, -1, NULL, 0);
        if (cchPath > 0)
        {
            lpszWideAppDirectory = new WCHAR[cchPath];
            MultiByteToWideChar(CP_UTF8, 0, szBuffer, -1, lpszWideAppDirectory, cchPath);
        }

        if (!lpszWideAppDirectory)
        {
            printf("*Error occurred while exporting album images for application (id: %s); ", DiscordApplicationIds[nApplication]);
            printf("album images for the application will not be exported*\n");

            RemoveDirectoryA(szBuffer);
            continue;
        }

        //
        // Write to the header file
        //
        StringCbPrintfA(szBuffer,
            sizeof(szBuffer),
            "    { \"%s\", {\n",
            DiscordApplicationIds[nApplication]
        );
        bSuccess = WriteFile(hExportFile, szBuffer, (DWORD)strlen(szBuffer), NULL, NULL);

        /* Enumerate the next album group */
        for (int nIndex = 0, nStartCount = (int)nAlbum; bSuccess && nAlbum - nStartCount < DISCORD_APP_MAX_ASSETS &&
            nAlbum < rgAlbumNames.size(); ++nIndex)
        {
            IITTrack                *pSong;
            IITArtworkCollection    *pAlbumImageCollection;
            IITArtwork              *pAlbumImage;
            LPSTR                   lpszCurrentAlbumName;
            BSTR                    pszValidAlbumName;
            BSTR                    pszExportImagePath;

            if (nIndex + nStartCount >= rgAlbumNames.size())
            {
                nAlbum = rgAlbumNames.size();
                break;
            }

            pAlbumImageCollection   = NULL;
            pAlbumImage             = NULL;
            lpszCurrentAlbumName    = std::get<1>(rgAlbumNames[nStartCount + nIndex]);

            //
            // Get the album artwork (image)
            //
            hResult = pLibrarySongs->get_Item(std::get<0>(rgAlbumNames[nStartCount + nIndex]), &pSong);
            if (SUCCEEDED(hResult)) {
                hResult = pSong->get_Artwork(&pAlbumImageCollection);
            }
            if (SUCCEEDED(hResult)) {
                hResult = pAlbumImageCollection->get_Item(1, &pAlbumImage);
            }

            SafeRelease(&pAlbumImageCollection);
            SafeRelease(&pSong);

            if (hResult != S_OK)
            {
                SafeRelease(&pAlbumImage);
                rgSkippedAlbums.push_back(lpszCurrentAlbumName);
                continue;
            }
          
            /* Correct & update the album name for saving to file */
            pszValidAlbumName = ValidateAndConvertAlbumName(lpszCurrentAlbumName);
            if (!pszValidAlbumName)
            {
                SafeRelease(&pAlbumImage);
                rgSkippedAlbums.push_back(lpszCurrentAlbumName);
                continue;
            }

            //
            // Export the album image to file
            //
            DWORD cchExportPath = (DWORD)(wcslen(lpszWideAppDirectory) + wcslen(L"\\") + wcslen(pszValidAlbumName) +
                wcslen(L".jpeg") + 1);

            pszExportImagePath = SysAllocStringLen(NULL, cchExportPath);
            StringCchPrintf(pszExportImagePath,
                cchExportPath + 1,
                L"%s\\%s.jpeg",
                lpszWideAppDirectory,
                pszValidAlbumName
            );

            hResult = pAlbumImage->SaveArtworkToFile(pszExportImagePath);

            SysFreeString(pszExportImagePath);
            SafeRelease(&pAlbumImage);

            if (FAILED(hResult))
            {
                rgSkippedAlbums.push_back(lpszCurrentAlbumName);
                continue;
            }

            //
            // Update the header file
            //
            bSuccess = WriteFile(hExportFile, "        \"", (DWORD)strlen("        \""), NULL, NULL);
            if (bSuccess)
            {
                if (strstr(lpszCurrentAlbumName, "\""))
                {
                    LPSTR lpszFixedName = FixQuotesInAlbumName(lpszCurrentAlbumName);
                    bSuccess = WriteFile(hExportFile, lpszFixedName, (DWORD)strlen(lpszFixedName), NULL, NULL);

                    rgCurrentAppAlbums.push_back(lpszFixedName);
                }
                else
                {
                    bSuccess = WriteFile(hExportFile, lpszCurrentAlbumName, (DWORD)strlen(lpszCurrentAlbumName), NULL, NULL);
                    rgCurrentAppAlbums.push_back(lpszCurrentAlbumName);
                }
            }
            if (bSuccess)
            {
                if (nAlbum - nStartCount + 1 < DISCORD_APP_MAX_ASSETS && nIndex + 1 + nStartCount < rgAlbumNames.size()) {
                    bSuccess = WriteFile(hExportFile, "\",\n", (DWORD)strlen("\",\n"), NULL, NULL);
                }
                else {
                    bSuccess = WriteFile(hExportFile, "\"\n", (DWORD)strlen("\"\n"), NULL, NULL);
                }
            }

            if (bSuccess) {
                ++nAlbum;
            }
        }
        
        /* Finalize the current application in the header file */
        if (bSuccess)
        {
            if (nApplication + 1 == DiscordApplicationIds.size())
            {
                bSuccess = WriteFile(hExportFile, "    } }\n", (DWORD)strlen("    } }\n"), NULL, NULL);
            }
            else
            {
                bSuccess = WriteFile(hExportFile, "    } },\n", (DWORD)strlen("    } },\n"), NULL, NULL);
            }
        }
    }

    /* Finalize header file */
    if (bSuccess) {
        bSuccess = WriteFile(hExportFile, "};\n", (DWORD)strlen("};\n"), NULL, NULL);
    }

    if (!bSuccess)
    {
        hResult = HRESULT_FROM_WIN32(GetLastError());

        CloseHandle(hExportFile);
        DeleteFile(L"DiscordAlbumAssets.h");

        return hResult;
    }

    CloseHandle(hExportFile);
    if (rgSkippedAlbums.size())
    {
        printf("\nThe following album images were not exported (usually due to the album not having an image):\n");
        for (size_t i = 0; i < rgSkippedAlbums.size(); ++i) {
            printf("%d: %s\n", (ULONG)i + 1, rgSkippedAlbums[i]);
        }
        printf("If you decide to fix any of the album images, you can run this tool again to update\n");
    }

    return S_OK;
}

//--------------------------------------------------
// Read DiscordAlbumAssets.h & Find Each Discord Application ID
//  Alongisde their Assigned Album Assets
BOOL ReadDiscordAlbumAssetFile(DiscordAlbumAssetFileData *pFileData)
{
    HANDLE          hHeaderFile;
    LARGE_INTEGER   liFileSize;
    WCHAR           szAlbumAssetsFile[MAX_PATH];
    BOOL            bSuccess;

    ZeroMemory(pFileData, sizeof(DiscordAlbumAssetFileData));

    GetDiscordAssetsFileName(szAlbumAssetsFile, ARRAYSIZE(szAlbumAssetsFile));

    /* Open the DiscordAlbumAssets.h file */
    hHeaderFile = CreateFile(szAlbumAssetsFile,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hHeaderFile == INVALID_HANDLE_VALUE)
    {
        printf("[Error]: Failed to open %ls file (%ls)\n",
            DISCORD_ASSETS_HEADER_FILE_NAME,
            _com_error(HRESULT_FROM_WIN32(GetLastError())).ErrorMessage()
        );
        return FALSE;
    }

    //
    // Read the file data
    // 
    bSuccess = GetFileSizeEx(hHeaderFile, &liFileSize);
    if (!bSuccess || liFileSize.QuadPart <= (LONGLONG)strlen(g_pszAlbumExportFileHeader))
    {
        if (!bSuccess)
        {
            printf("[Error]: Failed to read %ls file (%ls)\n",
                DISCORD_ASSETS_HEADER_FILE_NAME,
                _com_error(HRESULT_FROM_WIN32(GetLastError())).ErrorMessage()
            );
        }
        else
        {
            printf("[Error]: The Discord Album Asset header file (%ls) is corrupted/empty\n", DISCORD_ASSETS_HEADER_FILE_NAME);
        }

        CloseHandle(hHeaderFile);
        return FALSE;
    }

    pFileData->HeaderFileData = new CHAR[liFileSize.QuadPart + 1];
    if (!ReadFile(hHeaderFile, pFileData->HeaderFileData, (DWORD)liFileSize.QuadPart, NULL, NULL))
    {
        printf("[Error]: Failed to read %ls file (%ls)\n",
            DISCORD_ASSETS_HEADER_FILE_NAME,
            _com_error(HRESULT_FROM_WIN32(GetLastError())).ErrorMessage()
        );

        delete[] pFileData->HeaderFileData;
        CloseHandle(hHeaderFile);

        return FALSE;
    }

    CloseHandle(hHeaderFile);

    pFileData->FileLength                                   = liFileSize.QuadPart + 1;
    pFileData->HeaderFileData[pFileData->FileLength - 1]    = '\0';  

    //
    // Get the number of Discord Applications in the header file
    //
    LPSTR lpszBuffer = pFileData->HeaderFileData;
    if ((lpszBuffer = strstr(pFileData->HeaderFileData + strlen(g_pszAlbumExportFileHeader), "g_DiscordAppsAlbums[")))
    {
        int nApplications;

        lpszBuffer      += strlen("g_DiscordAppsAlbums[");
        nApplications   = atoi(&lpszBuffer[0]);

        if (nApplications)
        {
            pFileData->ApplicationAssets = new MBStringVector_t[nApplications]{};

            lpszBuffer = strstr(lpszBuffer, "\n");
            for (int i = 0; i < nApplications && lpszBuffer; ++i)
            {
                LPSTR lpszDataStart = strstr(lpszBuffer, "\"");
                if (!lpszDataStart) 
                {
                    break;
                }

                ++lpszDataStart;

                CHAR szIdBuffer[20] = { 0 };            
                for (int k = 0; k < strlen(lpszDataStart); ++k)
                {
                    if (!lpszDataStart || lpszDataStart[k] == '\"')
                    {
                        break;
                    }

                    szIdBuffer[k] = lpszDataStart[k];
                }

                lpszBuffer = ++lpszDataStart;
                if (!lpszBuffer) {
                    break;
                }

                lpszBuffer = strstr(lpszBuffer, "\n");
                ++lpszBuffer;

                CHAR szAlbumBuffer[MAX_PATH];
                for (int nAlbumLine = 0; lpszBuffer && *lpszBuffer && nAlbumLine < DISCORD_APP_MAX_ASSETS; ++nAlbumLine)
                {
                    lpszBuffer = strstr(lpszBuffer, "\"");
                    if (!lpszBuffer) {
                        continue;
                    }

                    lpszDataStart = ++lpszBuffer;

                    ZeroMemory(szAlbumBuffer, sizeof(szAlbumBuffer));
                    for (int k = 0; lpszDataStart && *lpszDataStart; ++k)
                    {
                        if (lpszDataStart[k] == '\"')
                        {
                            if ((lpszDataStart[k + 1] == ',' && lpszDataStart[k + 2] == '\n') ||
                                (lpszDataStart[k + 1] == '\n'))
                            {
                                break;
                            }
                        }

                        if (lpszDataStart[k] == '\\' && lpszDataStart[k + 1] == '\"')
                        {
                            ++lpszDataStart;
                            --k;
                            continue;
                        }

                        szAlbumBuffer[k] = lpszDataStart[k];
                    }

                    pFileData->ApplicationAssets[i].push_back(DuplicateString(szAlbumBuffer));

                    lpszBuffer = lpszDataStart += strlen(szAlbumBuffer);
                    if (lpszBuffer) {
                        lpszBuffer = strstr(lpszBuffer, "\n");
                    }
                    if (lpszBuffer) {
                        ++lpszBuffer;
                    }
                    
                    BOOL bFinished = TRUE;
                    while (lpszBuffer && *lpszBuffer)
                    {
                        if (*lpszBuffer != ' ')
                        {
                            if (*lpszBuffer == '\"')
                            {
                                bFinished = FALSE;
                                --lpszBuffer;
                            }

                            break;
                        }

                        ++lpszBuffer;
                    }

                    if (bFinished) {
                        break;
                    }
                }

                pFileData->DiscordApplicationIds.push_back(DuplicateString(szIdBuffer));
                if (lpszBuffer && *lpszBuffer)
                {
                    lpszBuffer = strstr(lpszBuffer, "\n");
                    if (!lpszBuffer) {
                        break;
                    }

                    ++lpszBuffer;
                    if (!lpszBuffer || !*lpszBuffer) {
                        break;
                    }
                }
            }
        }
    }
    
    return TRUE;
}

//--------------------------------------------------
// Assign/Append Album Assets to the Most Recent Discord Application
BOOL AddAlbumsToExistingApplication(DiscordAlbumAssetFileData &AssetData, AlbumInfoVector_t AlbumData)
{
    LPSTR               lpszEndingBuffer;
    MBStringVector_t    rgRecentApplication;
    SIZE_T              cchNewFileLength;
    int                 nAppendStartingPos;

    lpszEndingBuffer    = AssetData.HeaderFileData;
    rgRecentApplication = AssetData.ApplicationAssets[AssetData.DiscordApplicationIds.size() - 1];
    cchNewFileLength    = AssetData.FileLength;

    lpszEndingBuffer    = strstr(AssetData.HeaderFileData, rgRecentApplication[rgRecentApplication.size() - 1]);
    lpszEndingBuffer    += strlen(rgRecentApplication[rgRecentApplication.size() - 1]);
    lpszEndingBuffer    += strlen("\"");
    nAppendStartingPos  = (int)(strlen(AssetData.HeaderFileData) - strlen(lpszEndingBuffer));

    for (size_t i = rgRecentApplication.size(), k = 0; i < DISCORD_APP_MAX_ASSETS && k < AlbumData.size(); ++i, ++k)
    {     
        if (i == rgRecentApplication.size())
        {
            cchNewFileLength += strlen(",");
        }
        
        cchNewFileLength += strlen("        ");
        cchNewFileLength += strlen("\"");
        cchNewFileLength += strlen(std::get<1>(AlbumData[k]));
        cchNewFileLength += strlen("\"");

        if (i + 1 < DISCORD_APP_MAX_ASSETS && k + 1 < AlbumData.size())
        {
            cchNewFileLength += strlen(",");
            cchNewFileLength += strlen("\n");
        }   
    }

    if (cchNewFileLength > (SIZE_T)AssetData.FileLength)
    {
        LPSTR lpszUpdatedFile = new CHAR[cchNewFileLength];
        CopyMemory(lpszUpdatedFile, AssetData.HeaderFileData, nAppendStartingPos);

        LPSTR pUpdateBuffer = lpszUpdatedFile + nAppendStartingPos;
        for (size_t i = rgRecentApplication.size(), k = 0; i < DISCORD_APP_MAX_ASSETS && k < AlbumData.size(); ++i, ++k)
        {
            if (i == rgRecentApplication.size())
            {
                *(pUpdateBuffer)++ = ',';
                *(pUpdateBuffer)++ = '\n';
            }

            CopyMemory(pUpdateBuffer, "        \"", strlen("        \""));
            pUpdateBuffer += strlen("        \"");

            CopyMemory(pUpdateBuffer, std::get<1>(AlbumData[k]), strlen(std::get<1>(AlbumData[k])));
            pUpdateBuffer += strlen(std::get<1>(AlbumData[k]));

            *pUpdateBuffer = '\"';
            ++pUpdateBuffer;

            if (i + 1 < DISCORD_APP_MAX_ASSETS && k + 1 < AlbumData.size())
            {
                *(pUpdateBuffer)++ = ',';
                *(pUpdateBuffer)++ = '\n';
            }        
        }

        CopyMemory(pUpdateBuffer, lpszEndingBuffer, strlen(lpszEndingBuffer) + 1);

        delete[] AssetData.HeaderFileData;

        AssetData.HeaderFileData    = lpszUpdatedFile;
        AssetData.FileLength        = strlen(lpszUpdatedFile) + 1;

        return TRUE;
    }
    
    return FALSE;
}

#pragma warning( push )
#pragma warning( disable : 6387 ) /* Variable (lpszRest) could be NULL */

//--------------------------------------------------
// Assign Album Assets to the a New Discord Application
BOOL AddAlbumsToNewApplication(
    SIZE_T                      *pnAlbumOffset,
    IITTrackCollection          *pLibrarySongs,
    LPCSTR                      lpszApplicationId,
    AlbumInfoVector_t           AlbumData,
    PBYTE                       *ppbFileBuffer,
    DWORD                       *pcbFileBuffer,
    DiscordAlbumAssetFileData   *pAlbumAssetFile
)
{
    BOOL                bSuccess;
    CHAR                szCurrentDirectory[MAX_PATH];
    CHAR                szBuffer[MAX_PATH];
    MBStringVector_t    rgSkippedAlbums;
    LPWSTR              lpszWideAppDirectory;
    std::string         szHeaderFileBuf;

    szHeaderFileBuf         = "";
    lpszWideAppDirectory    = NULL;

    GetCurrentDirectoryA(MAX_PATH, szCurrentDirectory);

    //
    // Create the folder to hold the application album asset images 
    //
    StringCbPrintfA(szBuffer, sizeof(szBuffer), "%s\\%s", szCurrentDirectory, lpszApplicationId);
    if (PathIsDirectoryA(szBuffer))
    {
        if (!PathIsDirectoryEmptyA(szBuffer) && !DeleteDirectory(szBuffer))
        {
            printf("*Failed to delete existing files in the album export folder \"%s\" --", szBuffer);
            printf("Album images for the application (id: %s) will not be exported*\n", lpszApplicationId);
        }

        RemoveDirectoryA(szBuffer);
    }

    bSuccess = CreateDirectoryA(szBuffer, NULL);
    if (!bSuccess)
    {
        return FALSE;
    }

    /* Convert the folder path string to wide */
    DWORD cchPath = MultiByteToWideChar(CP_UTF8, 0, szBuffer, -1, NULL, 0);
    if (cchPath > 0)
    {
        lpszWideAppDirectory = new WCHAR[cchPath];
        MultiByteToWideChar(CP_UTF8, 0, szBuffer, -1, lpszWideAppDirectory, cchPath);
    }

    if (!lpszWideAppDirectory)
    {
        printf("*Error occurred while exporting album images for application (id: %s); ", lpszApplicationId);
        printf("album images for the application will not be exported*\n");

        RemoveDirectoryA(szBuffer);
        return FALSE;
    }

    //
    // Copy the application id to the header file buffer
    //
    szHeaderFileBuf.append(",\n    { \"");
    szHeaderFileBuf.append(lpszApplicationId);
    szHeaderFileBuf.append("\", {\n");

    //
    // Export the album cover images
    //
    for (SIZE_T nIndex = 0, nStartCount = *pnAlbumOffset; *pnAlbumOffset - nStartCount < DISCORD_APP_MAX_ASSETS; ++nIndex)
    {
        IITTrack                *pSong;
        IITArtworkCollection    *pAlbumImageCollection;
        IITArtwork              *pAlbumImage;
        LPSTR                   lpszCurrentAlbumName;
        BSTR                    pszValidAlbumName;
        BSTR                    pszExportImagePath;

        if (nIndex + nStartCount >= AlbumData.size())
        {
            *pnAlbumOffset = AlbumData.size();
            break;
        }

        pAlbumImageCollection   = NULL;
        pAlbumImage             = NULL;
        lpszCurrentAlbumName    = std::get<1>(AlbumData[nStartCount + nIndex]);

        //
        // Get the album artwork (image)
        //
        HRESULT hResult = pLibrarySongs->get_Item(std::get<0>(AlbumData[nStartCount + nIndex]), &pSong);
        if (SUCCEEDED(hResult)) {
            hResult = pSong->get_Artwork(&pAlbumImageCollection);
        }
        if (SUCCEEDED(hResult)) {
            hResult = pAlbumImageCollection->get_Item(1, &pAlbumImage);
        }

        SafeRelease(&pAlbumImageCollection);
        SafeRelease(&pSong);

        if (hResult != S_OK)
        {
            SafeRelease(&pAlbumImage);
            rgSkippedAlbums.push_back(lpszCurrentAlbumName);
            continue;
        }

        /* Correct & update the album name for saving to file */
        pszValidAlbumName = ValidateAndConvertAlbumName(lpszCurrentAlbumName);
        if (!pszValidAlbumName)
        {
            SafeRelease(&pAlbumImage);
            rgSkippedAlbums.push_back(lpszCurrentAlbumName);
            continue;
        }

        //
        // Export the album image to file
        //
        DWORD cchExportPath = (DWORD)(wcslen(lpszWideAppDirectory) + wcslen(L"\\") + wcslen(pszValidAlbumName) +
            wcslen(L".jpeg") + 1);

        pszExportImagePath = SysAllocStringLen(NULL, cchExportPath);
        StringCchPrintf(pszExportImagePath,
            cchExportPath + 1,
            L"%s\\%s.jpeg",
            lpszWideAppDirectory,
            pszValidAlbumName
        );

        hResult = pAlbumImage->SaveArtworkToFile(pszExportImagePath);

        SysFreeString(pszExportImagePath);
        SafeRelease(&pAlbumImage);

        if (FAILED(hResult))
        {
            rgSkippedAlbums.push_back(lpszCurrentAlbumName);
            continue;
        }

        if (!bSuccess)
        {
            break;
        }

        //
        // Update the header file
        //
        szHeaderFileBuf.append("        \"");
        if (bSuccess)
        {
            if (strstr(lpszCurrentAlbumName, "\""))
            {
                LPSTR lpszFixedName = FixQuotesInAlbumName(lpszCurrentAlbumName);
                szHeaderFileBuf.append(lpszFixedName);
                delete[] lpszFixedName;
            }
            else
            {
                szHeaderFileBuf.append(lpszCurrentAlbumName);
            }
        }
        if (bSuccess)
        {
            if (nIndex - nStartCount + 1 < DISCORD_APP_MAX_ASSETS && nIndex + 1 + nStartCount < AlbumData.size()) {
                szHeaderFileBuf.append("\",\n");
            }
            else {
                szHeaderFileBuf.append("\"\n");
            }
        }

        if (bSuccess) {
            ++*pnAlbumOffset;
        }
    }

    delete[] lpszWideAppDirectory;

    /* Finalize the header file buffer */
    szHeaderFileBuf.append("    } }");

    /* Print any skipped albums */
    if (rgSkippedAlbums.size())
    {
        printf("\nThe following album images were not exported (usually due to the album not having an image):\n");
        for (size_t i = 0; i < rgSkippedAlbums.size(); ++i) {
            printf("%d: %s\n", (ULONG)i + 1, rgSkippedAlbums[i]);
        }
        printf("If you decide to fix any of the album images, you can run this tool again to update\n");
    }

    //
    // Copy the file buffer
    //
    if (ppbFileBuffer)
    {
        *ppbFileBuffer = new BYTE[szHeaderFileBuf.length() + 1];
        *pcbFileBuffer = (DWORD)szHeaderFileBuf.length() + 1;

        CopyMemory(*ppbFileBuffer, szHeaderFileBuf.c_str(), szHeaderFileBuf.length() + 1);
        return TRUE;
    }
    
    //
    // Update the file buffer
    //
    else if (pAlbumAssetFile)
    {
        MBStringVector_t    rgRecentAppAlbums;
        LPSTR               lpszBuffer;
        LPSTR               lpszEndingBuffer;
        SIZE_T              cbNewBuffer;
        LPSTR               lpszNewBuffer;
        std::string         szFinalNewBuffer;
        
        lpszEndingBuffer = NULL;

        //
        // Update the number of elements in g_DiscordAppsAlbums
        //
        lpszBuffer = pAlbumAssetFile->HeaderFileData;
        lpszBuffer += strlen(g_pszAlbumExportFileHeader);
        if (lpszBuffer) {
            lpszEndingBuffer = strstr(lpszBuffer, "[");
        }
        if (lpszEndingBuffer + 1)
        {
            LPSTR lpszRest;

            *(++lpszEndingBuffer) = '\0';

            lpszBuffer -= strlen(g_pszAlbumExportFileHeader);
            szFinalNewBuffer.append(lpszBuffer);

            lpszRest = (LPSTR)((PBYTE)lpszEndingBuffer + 1);
            if (*lpszRest != ']') {
                lpszRest = strstr(lpszRest, "]");
            }

            StringCbPrintfA(szBuffer, sizeof(szBuffer), "%d", (ULONG)pAlbumAssetFile->DiscordApplicationIds.size() + 1);
            szFinalNewBuffer.append(szBuffer);

            szFinalNewBuffer.append(lpszRest);
            lpszBuffer = (LPSTR)szFinalNewBuffer.c_str();
        }
        
        rgRecentAppAlbums = pAlbumAssetFile->ApplicationAssets[
            pAlbumAssetFile->DiscordApplicationIds.size() - 1
        ];

        lpszEndingBuffer = strstr(lpszBuffer, rgRecentAppAlbums[rgRecentAppAlbums.size() - 1]);
        if (lpszEndingBuffer) {
            lpszEndingBuffer += strlen(rgRecentAppAlbums[rgRecentAppAlbums.size() - 1]);
        }
        if (lpszEndingBuffer) {
            lpszEndingBuffer = strstr(lpszEndingBuffer, "} }");
        }
        if (lpszEndingBuffer) {
            lpszEndingBuffer += strlen("} }");
        }
        
        if (!lpszEndingBuffer)
        {
            printf("Error: An unknown error occurred while parsing the new DiscordAlbumAssets.h file\n");
            return FALSE;
        }
        
        szHeaderFileBuf.append(lpszEndingBuffer);

        cbNewBuffer     = strlen(lpszBuffer) + szHeaderFileBuf.size() + 1;
        lpszNewBuffer   = new CHAR[cbNewBuffer];

        CopyMemory(lpszNewBuffer, lpszBuffer, strlen(lpszBuffer) - strlen(lpszEndingBuffer));
        CopyMemory(lpszNewBuffer + (strlen(lpszBuffer) - strlen(lpszEndingBuffer)), szHeaderFileBuf.c_str(), szHeaderFileBuf.size() + 1);

        delete[] pAlbumAssetFile->HeaderFileData;

        pAlbumAssetFile->HeaderFileData = lpszNewBuffer;
        pAlbumAssetFile->FileLength     = strlen(lpszNewBuffer);

        return TRUE;
    }

    return FALSE;
}

#pragma warning( pop )

static inline LPSTR DuplicateString(LPCSTR lpszString)
{
    LPSTR lpszCopy = new CHAR[strlen(lpszString) + 1];
    CopyMemory(lpszCopy, lpszString, strlen(lpszString) + 1);

    return lpszCopy;
}

static BOOL DeleteDirectory(LPCSTR lpszDirectory)
{
    LPSTR           lpszOperationPath;
    SHFILEOPSTRUCTA shFileOperation;

    //
    // Create double null terminated string for SHFileOperation
    //
    lpszOperationPath                               = new CHAR[strlen(lpszDirectory) + 2];
    lpszOperationPath[strlen(lpszDirectory) + 1]    = '\0';
    CopyMemory(lpszOperationPath, lpszDirectory, strlen(lpszDirectory) + 1);

    //
    // File operation to delete directory (even if not empty)
    //
    ZeroMemory(&shFileOperation, sizeof(shFileOperation));

    shFileOperation.wFunc   = FO_DELETE;
    shFileOperation.pFrom   = lpszOperationPath;
    shFileOperation.fFlags  = FOF_ALLOWUNDO | FOF_FILESONLY | FOF_SILENT | FOF_NOCONFIRMATION;

    int iResult = SHFileOperationA(&shFileOperation);
    
    delete[] lpszOperationPath;
    return (iResult == NO_ERROR);
    
}

static BSTR ValidateAndConvertAlbumName(LPCSTR lpszAlbumName)
{
    WCHAR szWideBuffer[MAX_PATH] = { 0 };
    
    for (SIZE_T nOriginal = 0, nBuffer = 0; nOriginal < strlen(lpszAlbumName) && nBuffer < MAX_PATH; ++nOriginal, ++nBuffer)
    {
        CHAR chCurrent = lpszAlbumName[nOriginal];

        /* Check if character is not allowed */
        if (!DiscordAssetCharacterAllowed(chCurrent))
        {
            /* Invalid characters are replaced with underscores when uploading discord assets */
            szWideBuffer[nBuffer] = L'_';

            /* Add extra underscore if the current character is a colon & the next character is space */
            if (chCurrent == ':' && lpszAlbumName[nOriginal + 1] == ' ')
            {
                if (nBuffer + 1 < MAX_PATH) {
                    szWideBuffer[++nBuffer] = L'_';
                }     
            }

            /* Continue loop if the current & next character is a space */
            else if (chCurrent == ' ' && lpszAlbumName[nOriginal + 1] == ' ') {
                continue;
            }

            /* Go to the next valid character in the album name */
            ++nOriginal;
            while (!DiscordAssetCharacterAllowed(lpszAlbumName[nOriginal]))
            {
                if (++nOriginal >= strlen(lpszAlbumName)) {
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
        szWideBuffer[nBuffer] = (WCHAR)tolower(lpszAlbumName[nOriginal]);
    }

    /* Convert the updated name to BSTR */
    return SysAllocString(szWideBuffer);
}
