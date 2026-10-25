#pragma once
#include "framework.h"
#include "../Core/config.h"
#include "../Core/Webhook.h"
#include <cstdarg>
#include <ctime>

namespace Log
{
	inline void Write(const char* Level, const char* Fmt, ...) {
		time_t Now = time(nullptr);
		tm LocalTm{};
		localtime_s(&LocalTm, &Now);

		char TimeBuf[16];
		strftime(TimeBuf, sizeof(TimeBuf), "%H:%M:%S", &LocalTm);

		char Msg[1024];
		va_list Args;
		va_start(Args, Fmt);
		vsnprintf(Msg, sizeof(Msg), Fmt, Args);
		va_end(Args);

		printf("[%s] [%s] %s\n", TimeBuf, Level, Msg);
	}
}

#define LOG_INFO(...)  Log::Write("INFO",  __VA_ARGS__)
#define LOG_WARN(...)  Log::Write("WARN",  __VA_ARGS__)
#define LOG_ERROR(...) Log::Write("ERROR", __VA_ARGS__)
#define LOG_DEBUG(...) Log::Write("DEBUG", __VA_ARGS__)

namespace GameMode
{
	inline bool ReadyToStartMatch(AFortGameModeAthena* thisPtr) {
		static int NumberOfCalls = 0;
		static int TotalCalls = 0;
		static int LastAliveCount = -1;
		TotalCalls++;

		if (!thisPtr) {
			LOG_ERROR("ReadyToStartMatch: GameMode is null (call #%d)", TotalCalls);
			return false;
		}

		AFortGameStateAthena* GameState = (AFortGameStateAthena*)thisPtr->GameState;

		if (!GameState) {
			LOG_WARN("ReadyToStartMatch: GameState is null (call #%d)", TotalCalls);
			return false;
		}

		if (!GameState->MapInfo) {
			if (TotalCalls % 300 == 1)
				LOG_WARN("ReadyToStartMatch: MapInfo not ready yet (call #%d)", TotalCalls);
			return false;
		}

		if (NumberOfCalls == 0) {
			LOG_INFO("Stage 0: setting up playlist...");

			char PlaylistPath[160];
			snprintf(PlaylistPath, sizeof(PlaylistPath), "FortPlaylistAthena %s.%s", Config::PlaylistName, Config::PlaylistName);

			UFortPlaylistAthena* Playlist = UObject::FindObject<UFortPlaylistAthena>(PlaylistPath);

			if (!Playlist) {
				LOG_ERROR("Playlist not found: %s", Config::PlaylistName);
				return false;
			}

			LOG_INFO("Found playlist: %s (%p)", Playlist->GetName().c_str(), Playlist);

			GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
			GameState->CurrentPlaylistInfo.MarkArrayDirty();
			LOG_DEBUG("CurrentPlaylistInfo updated (ReplicationKey=%d)", GameState->CurrentPlaylistInfo.PlaylistReplicationKey);

			thisPtr->CurrentPlaylistId = Playlist->PlaylistId;
			thisPtr->CurrentPlaylistName = Playlist->PlaylistName;
			GameState->CurrentPlaylistId = Playlist->PlaylistId;
			LOG_DEBUG("PlaylistId = %d", Playlist->PlaylistId);

			GameState->OnRep_CurrentPlaylistInfo();
			GameState->OnRep_CurrentPlaylistId();
			LOG_DEBUG("OnRep functions called");

			thisPtr->WarmupRequiredPlayerCount = Config::WarmupPlayers;
			LOG_DEBUG("WarmupRequiredPlayerCount = %d", thisPtr->WarmupRequiredPlayerCount);

			NumberOfCalls++;
			LOG_INFO("Stage 0 complete");
		}

		if (NumberOfCalls == 1) {
			LOG_INFO("Stage 1: creating NetDriver and starting listen...");

			auto GameNetDriver = UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");
			UWorld* World = UWorld::GetWorld();

			if (!World) {
				LOG_ERROR("UWorld::GetWorld() returned null");
				return false;
			}

			UNetDriver* NetDriver = Funcs::CreateNetDriver(UEngine::GetEngine(), World, GameNetDriver);

			if (!NetDriver) {
				LOG_ERROR("Failed to create NetDriver!");
				return false;
			}

			LOG_INFO("NetDriver created at %p", NetDriver);

			NetDriver->World = World;
			NetDriver->NetDriverName = GameNetDriver;

			FURL URL{};
			URL.Port = Config::ServerPort;
			LOG_INFO("Calling InitListen on port %d...", URL.Port);

			FString ErrorString = FString();
			bool InitSuccess = Funcs::InitListen(NetDriver, World, URL, false, ErrorString);
			if (!InitSuccess) {
				LOG_ERROR("InitListen failed on port %d! Error: %ls", URL.Port, ErrorString.ToString().c_str());
				return false;
			}
			LOG_INFO("InitListen succeeded");

			Funcs::SetWorld(NetDriver, World);
			LOG_DEBUG("SetWorld done");

			int CollectionCount = 0;
			for (auto& LevelCollection : World->LevelCollections) {
				LevelCollection.NetDriver = NetDriver;
				CollectionCount++;
			}
			LOG_DEBUG("Assigned NetDriver to %d level collection(s)", CollectionCount);

			thisPtr->bWorldIsReady = true;

			SetConsoleTitleA("9.10 | Listening");
			NumberOfCalls++;
			LOG_INFO("Stage 1 complete - server is listening on port %d", URL.Port);

			Webhook::SendServerUpAsync();
		}

		int AliveCount = thisPtr->AlivePlayers.Num();
		int Required = thisPtr->WarmupRequiredPlayerCount;

		if (AliveCount != LastAliveCount) {
			LOG_INFO("Alive players: %d / %d required", AliveCount, Required);
			LastAliveCount = AliveCount;
		}

		bool Ready = AliveCount >= Required;
		static bool LoggedReady = false;
		if (Ready && !LoggedReady) {
			LOG_INFO("Player requirement met - match is ready to start!");
			LoggedReady = true;
		}

		return Ready;
	}

	APawn* SpawnDefaultPawnFor(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* Controller, AActor* StartSpot) {
		if (!GameMode || !Controller || !StartSpot) {
			LOG_ERROR("SpawnDefaultPawnFor: null pointer (GameMode=%p, Controller=%p, StartSpot=%p)", GameMode, Controller, StartSpot);
			return nullptr;
		}

		LOG_INFO("Spawning pawn for controller %p at start spot %s", Controller, StartSpot->GetName().c_str());

		APawn* Pawn = GameMode->SpawnDefaultPawnAtTransform(Controller, StartSpot->GetTransform());

		if (!Pawn) {
			LOG_ERROR("SpawnDefaultPawnAtTransform returned null for controller %p", Controller);
			return nullptr;
		}

		LOG_INFO("Pawn spawned: %p", Pawn);
		return Pawn;
	}

	void (*TickFlush_OG)(UNetDriver* NetDriver, float DeltaSeconds);
	void TickFlush(UNetDriver* Driver, float DeltaSeconds)
	{
		static unsigned long long TickCount = 0;
		static bool LoggedFirstTick = false;
		static bool LoggedNoRepDriver = false;
		TickCount++;

		if (!Driver) {
			LOG_ERROR("TickFlush: Driver is null (tick #%llu)", TickCount);
			return;
		}

		if (!LoggedFirstTick) {
			LOG_INFO("TickFlush running for the first time (Driver=%p)", Driver);
			LoggedFirstTick = true;
		}

		if (Driver->ReplicationDriver) {
			Funcs::ServerReplicateActors(Driver->ReplicationDriver, DeltaSeconds);
		}
		else if (!LoggedNoRepDriver) {
			LOG_WARN("TickFlush: Driver has no ReplicationDriver - actors will not replicate");
			LoggedNoRepDriver = true;
		}

		if (TickCount % 900 == 0) {
			LOG_DEBUG("TickFlush heartbeat: tick #%llu, dt=%.4f, clients=%d",
				TickCount, DeltaSeconds, Driver->ClientConnections.Num());
		}

		return TickFlush_OG(Driver, DeltaSeconds);
	}
}

namespace Patches
{
	int ReturnTrue() {
		static bool Logged = false;
		if (!Logged) {
			LOG_DEBUG("Patches::ReturnTrue hit for the first time");
			Logged = true;
		}
		return 1;
	}
}