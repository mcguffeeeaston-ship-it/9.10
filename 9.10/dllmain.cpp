#include "framework.h"
#include "GameMode.h"

DWORD WINAPI Main(LPVOID)
{
    AllocConsole();
    FILE* File = nullptr;
    freopen_s(&File, "CONOUT$", "w+", stdout);
    SetConsoleTitleA("9.10 | Starting...");
    MH_Initialize();

    Hook(ImageBase + Addresses::KickPlayer, Patches::ReturnTrue);
    Hook(ImageBase + Addresses::GetNetMode, Patches::ReturnTrue);

    *(bool*)(ImageBase + Addresses::GIsClient) = false;
    *(bool*)(ImageBase + Addresses::GIsServer) = true;

    Hook(ImageBase + Addresses::TickFlush, GameMode::TickFlush, (void**)&GameMode::TickFlush_OG);

    UWorld* World = UWorld::GetWorld();
    if (World && World->OwningGameInstance)
    {
        UKismetSystemLibrary::ExecuteConsoleCommand(World, L"open Athena_Terrain", nullptr);
        if (World->OwningGameInstance->LocalPlayers.Num() > 0)
        {
            World->OwningGameInstance->LocalPlayers.Remove(0);
        }
    }

    Hook(ImageBase + Addresses::ReadyToStartMatch, GameMode::ReadyToStartMatch);
    Hook(ImageBase + Addresses::SpawnDefaultPawnFor, GameMode::SpawnDefaultPawnFor);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, Main, 0, 0, 0);
        break;
    }
    return TRUE;
}

