#include "framework.h"
#include "config.h"
#include "../Game/GameMode.h"

#define HOOK(Name, Offset, Detour) \
    do { \
        uintptr_t Target = ImageBase + (Offset); \
        LOG_INFO("Hooking %-18s @ %p (base+0x%llX)", Name, (void*)Target, (unsigned long long)(Offset)); \
        Hook(Target, Detour); \
        LOG_DEBUG("Hook installed: %s", Name); \
    } while (0)

DWORD WINAPI Main(LPVOID)
{
    AllocConsole();
    FILE* File = nullptr;
    freopen_s(&File, "CONOUT$", "w+", stdout);
    SetConsoleTitleA("9.10 | Starting...");

    LOG_INFO("=== 9.10 server starting ===");
    LOG_INFO("Build: %s %s", __DATE__, __TIME__);
    LOG_INFO("ImageBase: %p", (void*)ImageBase);

    MH_STATUS Status = MH_Initialize();
    if (Status != MH_OK) {
        LOG_ERROR("MH_Initialize failed: %s", MH_StatusToString(Status));
        SetConsoleTitleA("9.10 | MinHook init failed");
        return 1;
    }
    LOG_INFO("MinHook initialized");

    LOG_INFO("Installing patches...");
    if (Config::KickPlayerPatch)
        HOOK("KickPlayer", Addresses::KickPlayer, Patches::ReturnTrue);
    else
        LOG_INFO("KickPlayer patch disabled in config.h");
    if (Config::NetModePatch)
        HOOK("GetNetMode", Addresses::GetNetMode, Patches::ReturnTrue);
    else
        LOG_INFO("GetNetMode patch disabled in config.h");

    LOG_INFO("Setting GIsClient=false, GIsServer=true");
    *(bool*)(ImageBase + Addresses::GIsClient) = false;
    *(bool*)(ImageBase + Addresses::GIsServer) = true;
    LOG_DEBUG("GIsClient=%d, GIsServer=%d",
        *(bool*)(ImageBase + Addresses::GIsClient),
        *(bool*)(ImageBase + Addresses::GIsServer));

    LOG_INFO("Hooking %-18s @ %p (base+0x%llX)", "TickFlush", (void*)(ImageBase + Addresses::TickFlush), (unsigned long long)(Addresses::TickFlush));
    Hook(ImageBase + Addresses::TickFlush, GameMode::TickFlush, (void**)&GameMode::TickFlush_OG);
    LOG_DEBUG("Hook installed: %s", "TickFlush");
    if (!GameMode::TickFlush_OG)
        LOG_WARN("TickFlush_OG is null after hooking - original won't be callable");

    UWorld* World = UWorld::GetWorld();
    if (!World) {
        LOG_ERROR("UWorld::GetWorld() returned null - skipping map open");
    }
    else if (!World->OwningGameInstance) {
        LOG_ERROR("World (%p) has no OwningGameInstance - skipping map open", World);
    }
    else {
        LOG_INFO("World: %p, GameInstance: %p", World, World->OwningGameInstance);

        wchar_t OpenCmd[128];
        swprintf_s(OpenCmd, L"open %hs", Config::MapName);
        LOG_INFO("Opening map: %s", Config::MapName);
        UKismetSystemLibrary::ExecuteConsoleCommand(World, OpenCmd, nullptr);

        int LocalPlayerCount = World->OwningGameInstance->LocalPlayers.Num();
        LOG_INFO("LocalPlayers count: %d", LocalPlayerCount);

        if (LocalPlayerCount > 0) {
            World->OwningGameInstance->LocalPlayers.Remove(0);
            LOG_INFO("Removed LocalPlayer[0] (now %d)", World->OwningGameInstance->LocalPlayers.Num());
        }
        else {
            LOG_DEBUG("No LocalPlayers to remove");
        }
    }

    HOOK("ReadyToStartMatch", Addresses::ReadyToStartMatch, GameMode::ReadyToStartMatch);
    HOOK("SpawnDefaultPawnFor", Addresses::SpawnDefaultPawnFor, GameMode::SpawnDefaultPawnFor);

    LOG_INFO("All hooks installed - waiting for the map to load...");
    SetConsoleTitleA("9.10 | Waiting for map");

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
    {
        DisableThreadLibraryCalls(hModule);
        HANDLE Thread = CreateThread(0, 0, Main, 0, 0, 0);
        if (Thread)
            CloseHandle(Thread);
        break;
    }
    case DLL_PROCESS_DETACH:
        LOG_INFO("DLL detaching");
        break;
    }
    return TRUE;
}