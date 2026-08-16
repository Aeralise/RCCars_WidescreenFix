#pragma once

#include <Windows.h>
#include <cstddef>

struct Config
{
    int Width = 1920;
    int Height = 1080;

    int Aspect = 1;

    int FixFOV = 1;
    double CustomFOV = 193;

    int FixHUD = 1;
    int LegacyFixHUD = 0;

    int NoMinimize = 0;
};

extern Config cfg;

// GLOBAL INITIALIZATION

DWORD WINAPI InitThread(LPVOID);

// CONFIG / PATCH HELPERS

void LoadConfig();

void WriteBytes(void* address,const void* data,size_t size);

void ApplyResolution();

void ApplyAspect();

void ApplyFOV();

void ApplyDebug();

void ShutdownFix();