#pragma once
#include <cstdint>

namespace Config
{
	inline constexpr int         ServerPort = 7777;
	inline constexpr const char* ServerRegion = "NAE";
	inline constexpr const char* MapName = "Athena_Terrain";
	inline constexpr const char* PlaylistName = "Playlist_DefaultSolo";
	inline constexpr int         MaxPlayers = 100;
	inline constexpr int         WarmupPlayers = 1;

	inline constexpr bool KickPlayerPatch = true;
	inline constexpr bool NetModePatch = true;

	inline constexpr bool LogDebug = true;
	inline constexpr bool LogInfo = true;
	inline constexpr bool LogWarn = true;
	inline constexpr bool LogError = true;
	inline constexpr bool ColorLogging = true;
	inline constexpr bool FileLogging = true;
	inline constexpr const char* LogFile = "9.10-server.log";

	inline constexpr bool Heartbeat = true;
	inline constexpr int  HeartbeatSeconds = 30;

	inline constexpr bool         DiscordWebhook = true;
	inline constexpr const char* WebhookUrl = "https://discord.com/api/webhooks/1564062458157465610/JXQUCPDpKQMjHkEQSVZoGs5IedVZx9Uy4KwoRgV58kw-QyP2cGqUtvJBTt13CcJfeNy4";
	inline constexpr const char* WebhookUsername = "9.10 Server";
	inline constexpr const char* WebhookAvatar = "";
	inline constexpr uint32_t     WebhookColor = 0x57F287;
	inline constexpr int          WebhookRetries = 3;
	inline constexpr int          WebhookTimeoutMs = 5000;
}

static_assert(Config::ServerPort > 0 && Config::ServerPort <= 65535, "ServerPort must be 1-65535");
static_assert(Config::MaxPlayers > 0 && Config::MaxPlayers <= 100, "MaxPlayers must be 1-100");
static_assert(Config::WarmupPlayers > 0 && Config::WarmupPlayers <= Config::MaxPlayers, "WarmupPlayers must be between 1 and MaxPlayers");
static_assert(Config::HeartbeatSeconds > 0, "HeartbeatSeconds must be more than 0");
static_assert(Config::WebhookRetries >= 1, "WebhookRetries must be at least 1");
static_assert(Config::WebhookTimeoutMs >= 500, "WebhookTimeoutMs must be at least 500");