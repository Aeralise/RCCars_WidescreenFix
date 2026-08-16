#include <Windows.h>
#include "pch.h"
#include "fix_core.h"

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD reason,
    LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        CreateThread(
            nullptr,
            0,
            InitThread,
            nullptr,
            0,
            nullptr
        );
    }

    return TRUE;
}