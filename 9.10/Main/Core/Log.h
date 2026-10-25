#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <mutex>
#include "Config.h"

namespace Log
{
	enum Level { Debug = 0, Info = 1, Warn = 2, Error = 3 };

	struct State {
		std::mutex Lock;
		FILE* File = nullptr;
	};

	inline State& Get() {
		static State S;
		return S;
	}

	inline void Init() {
		if (!Config::FileLogging) return;

		State& S = Get();
		std::lock_guard<std::mutex> Guard(S.Lock);
		if (S.File) return;

		fopen_s(&S.File, Config::LogFile, "a");
		if (S.File) {
			fprintf(S.File, "\n========== session started ==========\n");
			fflush(S.File);
		}
	}

	inline void Shutdown() {
		State& S = Get();
		std::lock_guard<std::mutex> Guard(S.Lock);
		if (S.File) {
			fprintf(S.File, "========== session ended ==========\n");
			fclose(S.File);
			S.File = nullptr;
		}
	}

	inline void Write(Level Lvl, const char* Fmt, ...) {
		static const bool Enabled[] = { Config::LogDebug, Config::LogInfo, Config::LogWarn, Config::LogError };
		if (!Enabled[Lvl]) return;

		static const char* Names[] = { "DEBUG", "INFO ", "WARN ", "ERROR" };
		static const WORD  Colors[] = { 8, 7, 14, 12 };

		SYSTEMTIME St;
		GetLocalTime(&St);

		char Msg[2048];
		va_list Args;
		va_start(Args, Fmt);
		vsnprintf(Msg, sizeof(Msg), Fmt, Args);
		va_end(Args);

		char Line[2200];
		snprintf(Line, sizeof(Line), "[%02d:%02d:%02d.%03d] [%s] [T%lu] %s\n",
			St.wHour, St.wMinute, St.wSecond, St.wMilliseconds,
			Names[Lvl], GetCurrentThreadId(), Msg);

		State& S = Get();
		std::lock_guard<std::mutex> Guard(S.Lock);

		HANDLE Console = GetStdHandle(STD_OUTPUT_HANDLE);
		bool Colored = Config::ColorLogging && Console && Console != INVALID_HANDLE_VALUE;

		if (Colored) SetConsoleTextAttribute(Console, Colors[Lvl]);
		fputs(Line, stdout);
		fflush(stdout);
		if (Colored) SetConsoleTextAttribute(Console, 7);

		if (S.File) {
			fputs(Line, S.File);
			if (Lvl >= Warn) fflush(S.File);
		}
	}
}

#define LOG_DEBUG(...) Log::Write(Log::Debug, __VA_ARGS__)
#define LOG_INFO(...)  Log::Write(Log::Info,  __VA_ARGS__)
#define LOG_WARN(...)  Log::Write(Log::Warn,  __VA_ARGS__)
#define LOG_ERROR(...) Log::Write(Log::Error, __VA_ARGS__)