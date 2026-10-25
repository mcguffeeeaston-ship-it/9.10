#pragma once
#include "framework.h"
#include "config.h"
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

namespace Webhook
{
	inline bool PostJson(HINTERNET Session, const wchar_t* Host, const wchar_t* Path, DWORD Port, bool Secure, const char* Json)
	{
		HINTERNET Connection = WinHttpConnect(Session, Host, Port, 0);
		if (!Connection) {
			printf("[Webhook] WinHttpConnect failed (error %lu)\n", GetLastError());
			return false;
		}

		HINTERNET Request = WinHttpOpenRequest(Connection, L"POST", Path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
			Secure ? WINHTTP_FLAG_SECURE : 0);
		if (!Request) {
			printf("[Webhook] WinHttpOpenRequest failed (error %lu)\n", GetLastError());
			WinHttpCloseHandle(Connection);
			return false;
		}

		const wchar_t* Headers = L"Content-Type: application/json\r\n";
		int JsonLen = (int)strlen(Json);
		bool Success = false;

		if (WinHttpSendRequest(Request, Headers, (DWORD)-1L, (LPVOID)Json, (DWORD)JsonLen, (DWORD)JsonLen, 0) &&
			WinHttpReceiveResponse(Request, nullptr)) {
			DWORD StatusCode = 0, Size = sizeof(StatusCode);
			WinHttpQueryHeaders(Request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
				&StatusCode, &Size, WINHTTP_NO_HEADER_INDEX);
			Success = (StatusCode == 204);
			if (!Success)
				printf("[Webhook] Unexpected HTTP status: %lu\n", StatusCode);
		}
		else {
			printf("[Webhook] Request failed (error %lu)\n", GetLastError());
		}

		WinHttpCloseHandle(Request);
		WinHttpCloseHandle(Connection);
		return Success;
	}

	inline void SendServerUp()
	{
		char Json[1024];
		sprintf_s(Json, sizeof(Json),
			"{\"username\":\"%s\","
			"\"embeds\":[{"
			"\"title\":\"Server Up\","
			"\"color\":%u,"
			"\"fields\":["
			"{\"name\":\"Server\",\"value\":\"%s\",\"inline\":true},"
			"{\"name\":\"Playlist\",\"value\":\"%s\",\"inline\":true}"
			"]}]}",
			Config::WebhookUsername,
			Config::WebhookColor,
			Config::ServerRegion, Config::PlaylistName);

		wchar_t wUrl[1024] = L"";
		MultiByteToWideChar(CP_UTF8, 0, Config::WebhookUrl, -1, wUrl, 1024);

		URL_COMPONENTS UrlParts{};
		UrlParts.dwStructSize = sizeof(UrlParts);
		wchar_t Host[256] = L"";
		wchar_t Path[1024] = L"/";
		UrlParts.lpszHostName = Host;
		UrlParts.dwHostNameLength = 256;
		UrlParts.lpszUrlPath = Path;
		UrlParts.dwUrlPathLength = 1024;

		if (!WinHttpCrackUrl(wUrl, (DWORD)wcslen(wUrl), 0, &UrlParts)) {
			printf("[Webhook] Failed to parse Config::WebhookUrl (error %lu)\n", GetLastError());
			return;
		}

		HINTERNET Session = WinHttpOpen(L"9.10-Server", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!Session) {
			printf("[Webhook] WinHttpOpen failed (error %lu)\n", GetLastError());
			return;
		}

		WinHttpSetTimeouts(Session, Config::WebhookTimeoutMs, Config::WebhookTimeoutMs, Config::WebhookTimeoutMs, Config::WebhookTimeoutMs);

		for (int Attempt = 1; Attempt <= Config::WebhookRetries; Attempt++) {
			if (PostJson(Session, Host, Path, UrlParts.nPort, UrlParts.nScheme == INTERNET_SCHEME_HTTPS, Json)) {
				printf("[Webhook] Server Up embed sent (Server=%s, Playlist=%s)\n", Config::ServerRegion, Config::PlaylistName);
				break;
			}
			printf("[Webhook] Attempt %d/%d failed\n", Attempt, Config::WebhookRetries);
		}

		WinHttpCloseHandle(Session);
	}

	inline DWORD WINAPI SendServerUpThread(LPVOID)
	{
		SendServerUp();
		return 0;
	}

	inline void SendServerUpAsync()
	{
		if (!Config::DiscordWebhook)
			return;
		if (strstr(Config::WebhookUrl, "REPLACE_ME")) {
			printf("[Webhook] Config::WebhookUrl not set - skipping Server Up embed\n");
			return;
		}
		CreateThread(nullptr, 0, SendServerUpThread, nullptr, 0, nullptr);
	}
}
