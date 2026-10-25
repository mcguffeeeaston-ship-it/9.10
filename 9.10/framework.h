#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include "SDK/SDK.hpp"
using namespace SDK;
#include "Minhook/MinHook.h"
#include "Addresses.h"
#include "Funcs.h"

static void Hook_Internal(uint64_t Address, PVOID Hook, void** Detour = nullptr)
{
	MH_CreateHook(PVOID(Address), Hook, Detour);
	MH_EnableHook(PVOID(Address));
}
#define Hook(...) Hook_Internal(__VA_ARGS__);