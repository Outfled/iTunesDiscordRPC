/* !! DON'T MODIFY UNLESS YOU KNOW WHAT YOU ARE DOING !! */
#pragma once

/* Includes generic iTunes logo for any songs that dont have a corresponding album asset */
#define DEFAULT_DISCORD_APPLICATION_ID			"1095131480147103744"
#define DEFAULT_DISCORD_APP_IMAGE_KEY			"small-icon"

typedef struct DiscordAppAlbumAssets
{
	const char *ApplicationId;
	const char *AlbumNames[300];
};

const DiscordAppAlbumAssets g_DiscordAppsAlbums[1] = {
	{ DEFAULT_DISCORD_APPLICATION_ID, { DEFAULT_DISCORD_APP_IMAGE_KEY } }
};