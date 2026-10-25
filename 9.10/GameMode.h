#pragma once
#include "framework.h"

namespace GameMode 
{
	inline bool ReadyToStartMatch(AFortGameModeAthena* thisPtr) {
		static int NumberOfCalls = 0;

		if (!thisPtr) return false;
		
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)thisPtr->GameState;

		if (!GameState || !GameState->MapInfo)
			return false;

		if (NumberOfCalls == 0) {
			UFortPlaylistAthena* Playlist = UObject::FindObject<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo");

			if (!Playlist) {
				printf("[ERROR] Playlist not found!\n");
				return false;
			}
			
			GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
			GameState->CurrentPlaylistInfo.MarkArrayDirty();

			thisPtr->CurrentPlaylistId = Playlist->PlaylistId;
			thisPtr->CurrentPlaylistName = Playlist->PlaylistName;
			GameState->CurrentPlaylistId = Playlist->PlaylistId;

			GameState->OnRep_CurrentPlaylistInfo();
			GameState->OnRep_CurrentPlaylistId();

			thisPtr->WarmupRequiredPlayerCount = 1;

			NumberOfCalls++;
		}

		if (NumberOfCalls == 1) {
			auto GameNetDriver = UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");
			UNetDriver* NetDriver = Funcs::CreateNetDriver(UEngine::GetEngine(), UWorld::GetWorld(), GameNetDriver);

			if (!NetDriver) {
				printf("[ERROR] Failed to create NetDriver!\n");
				return false;
			}
			
			NetDriver->World = UWorld::GetWorld();
			NetDriver->NetDriverName = GameNetDriver;

			FURL URL{};
			URL.Port = 7777;

			FString ErrorString = FString();
			bool InitSuccess = Funcs::InitListen(NetDriver, UWorld::GetWorld(), URL, false, ErrorString);
			if (!InitSuccess) {
				printf("[ERROR] InitListen failed!\n");
				return false;
			}
			
			Funcs::SetWorld(NetDriver, UWorld::GetWorld());

			for (auto& LevelCollection : UWorld::GetWorld()->LevelCollections) {
				LevelCollection.NetDriver = NetDriver;
			}

			thisPtr->bWorldIsReady = true;

			SetConsoleTitleA("9.10 | Listening");
			NumberOfCalls++;
		}

		return thisPtr->AlivePlayers.Num() >= thisPtr->WarmupRequiredPlayerCount;
	}
	
	APawn* SpawnDefaultPawnFor(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* Controller, AActor* StartSpot) {
		if (!GameMode || !Controller || !StartSpot) {
			printf("[ERROR] SpawnDefaultPawnFor received null pointer!\n");
			return nullptr;
		}
		return GameMode->SpawnDefaultPawnAtTransform(Controller, StartSpot->GetTransform());
	}

	void (*TickFlush_OG)(UNetDriver* NetDriver, float DeltaSeconds);
	void TickFlush(UNetDriver* Driver, float DeltaSeconds)
	{
		if (!Driver) return;
		
		if (Driver->ReplicationDriver) {
			Funcs::ServerReplicateActors(Driver->ReplicationDriver, DeltaSeconds);
		}
		return TickFlush_OG(Driver, DeltaSeconds);
	}
}

namespace Patches
{
	int ReturnTrue() {
		return 1;
	}
}