#include "pch.h"
#include "fix_core.h"
#include "hud_fix.h"
#include "game_addresses.h"

#include <Windows.h>
#include <mmsystem.h>

#include <cstdint>
#include <chrono>
#include <cmath>
#include <cstring>

#include "MinHook.h"

#pragma comment(lib, "libMinHook.x86.lib")
#pragma comment(lib, "winmm.lib")

Config cfg;

#pragma region FPS_fix

// CONFIG

//bool g_EnableFix = true;
int g_MaxDelta = 100;
volatile int* legacy_TargetFPS = reinterpret_cast<volatile int*>(Addr::LegacyTargetFPS);

bool g_Enabled = true;
double g_TargetFPS = 144.0;

// EMA smoothing factor
double g_SmoothingFactor = 0.10;

// ORIGINAL FUNCTION

typedef void(__cdecl* t_gmpSetElapsedTime)(
    int,
    float,
    float);

t_gmpSetElapsedTime Real_gmpSetElapsedTime = reinterpret_cast<t_gmpSetElapsedTime>(Addr::GMPSetElapsedTime);

// PATCH LOCATION

// CALL _gmpSetElapsedTime
uintptr_t g_CallPatch = Addr::GMPSetElapsedTimeCall;

// TIMING

LARGE_INTEGER g_QPCFreq;

LARGE_INTEGER g_LastFrameCounter;

bool g_TimerInitialized = false;

// smoothed frametime
double g_SmoothedMS = 0.0;

// fractional integer cadence accumulator
double g_FractionalAccumulator = 0.0;

// FPS LIMITER + FRAME TIMER

double WaitAndMeasureFrame()
{
    LARGE_INTEGER now;

    QueryPerformanceCounter(&now);

    if (!g_TimerInitialized)
    {
        g_LastFrameCounter = now;

        g_TimerInitialized = true;

        return 1000.0 / g_TargetFPS;
    }

    double targetMS = 1000.0 / g_TargetFPS;

    double elapsedMS;

    while (true)
    {
        QueryPerformanceCounter(&now);

        elapsedMS = ((double)(now.QuadPart - g_LastFrameCounter.QuadPart) / (double)g_QPCFreq.QuadPart) * 1000.0;

        if (elapsedMS >= targetMS)
            break;

        // better than Sleep(1)
        Sleep(0);
    }

    g_LastFrameCounter = now;

    // clamp broken spikes
    if (elapsedMS > 250.0)
        elapsedMS = targetMS;

    return elapsedMS;
}

// SMOOTH FRAME TIME

double SmoothFrameTime(double realDeltaMS)
{
    if (g_SmoothedMS <= 0.0)
    {
        g_SmoothedMS = realDeltaMS;
    }
    else
    {
        // EMA smoothing
        g_SmoothedMS = g_SmoothedMS * (1.0 - g_SmoothingFactor) + realDeltaMS * g_SmoothingFactor;
    }

    return g_SmoothedMS;
}

// HOOK

BYTE g_OriginalBytes[5];

uintptr_t g_HookAddress = 0x004407A0;

// STABLE INTEGER CADENCE

int GenerateNormalizedMS(double smoothMS)
{
    int baseMS = (int)floor(smoothMS);

    double frac = smoothMS - (double)baseMS;

    g_FractionalAccumulator += frac;

    if (g_FractionalAccumulator >= 1.0)
    {
        baseMS++;

        g_FractionalAccumulator -= 1.0;
    }

    if (baseMS < 1)
        baseMS = 1;

    return baseMS;
}

// REPLACEMENT CALL TARGET

void __cdecl My_gmpSetElapsedTime(
    int elapsedMS,
    float minStep,
    float maxStep)
{
    if (!g_Enabled)
    {
        Real_gmpSetElapsedTime(
            elapsedMS,
            minStep,
            maxStep);

        return;
    }

    // FPS limiter + real frame timing
    double realFrameMS = WaitAndMeasureFrame();

    // Smooth unstable frametimes
    double smoothMS = SmoothFrameTime(realFrameMS);

    // Convert to stable integer cadence
    int normalizedMS = GenerateNormalizedMS(smoothMS);

    // Feed normalized timestep into GMP
    Real_gmpSetElapsedTime(normalizedMS,minStep,maxStep);
}

// INSTALL CALL PATCH
void InstallPatch()
{
    BYTE* call = (BYTE*)g_CallPatch;

    DWORD oldProtect;

    VirtualProtect(call, 5, PAGE_EXECUTE_READWRITE, &oldProtect);

    // CALL My_gmpSetElapsedTime
    call[0] = 0xE8;

    uintptr_t relative = (uintptr_t)My_gmpSetElapsedTime - (g_CallPatch + 5);

    *(uintptr_t*)(call + 1) = relative;

    VirtualProtect(call, 5, oldProtect, &oldProtect);

    FlushInstructionCache(GetCurrentProcess(), call, 5);
}

#pragma endregion

// LOAD CONFIG
void LoadConfig()
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);

    char* slash = strrchr(path, '\\');
    if (slash) *(slash + 1) = '\0';

    strcat_s(path, "RCCars_WidescreenFix.ini");

    cfg.Width = GetPrivateProfileIntA("MAIN", "Width", 1920, path);
    cfg.Height = GetPrivateProfileIntA("MAIN", "Height", 1080, path);

    if (cfg.Width <= 0)
        cfg.Width = GetSystemMetrics(SM_CXSCREEN);
    if (cfg.Height <= 0)
        cfg.Height = GetSystemMetrics(SM_CYSCREEN);

    cfg.Aspect = GetPrivateProfileIntA("MAIN", "Aspect", 1, path);
    cfg.FixFOV = GetPrivateProfileIntA("MAIN", "FixFOV", 1, path);
    cfg.CustomFOV = GetPrivateProfileIntA("MAIN", "CustomFOV", 80, path);
    cfg.FixHUD = GetPrivateProfileIntA("MAIN", "FixHUD", 1, path);
    cfg.LegacyFixHUD = GetPrivateProfileIntA("MAIN", "LegacyFixHUD", 1, path);

    //cfg.Patch_4GB = GetPrivateProfileIntA("MISC", "Patch_4GB", 1, path);
    cfg.NoMinimize = GetPrivateProfileIntA("DEBUG", "NoMinimize", 0, path);

    //g_Enabled = GetPrivateProfileIntA("Fix","Enabled",1, path) != 0;

    g_TargetFPS = (double)GetPrivateProfileIntA("FPS", "Limit", 60, path);

    DEVMODE dm = { 0 };
    dm.dmSize = sizeof(DEVMODE);
    double displayHz = 60;
    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
        displayHz = (double)dm.dmDisplayFrequency;

    if (g_TargetFPS == 0)
        g_TargetFPS = displayHz;
    if (g_TargetFPS < 0 || g_TargetFPS > 600)
        g_TargetFPS = 60;
}

void WriteBytes(void* address,const void* data,size_t size)
{
    DWORD oldProtect = 0;
    if (!VirtualProtect(address,size,PAGE_EXECUTE_READWRITE,&oldProtect))
    {
        return;
    }
    std::memcpy(address,data,size);
    DWORD dummy = 0;
    VirtualProtect(address,size,oldProtect,&dummy);
}

void ApplyResolution()
{
    const uint16_t width =  static_cast<uint16_t>(cfg.Width);
    const uint16_t height = static_cast<uint16_t>(cfg.Height);

    uint16_t* width1 =  reinterpret_cast<uint16_t*>(Addr::XResolution1);
    uint16_t* width2 =  reinterpret_cast<uint16_t*>(Addr::XResolution2);
    uint16_t* height1 = reinterpret_cast<uint16_t*>(Addr::YResolution1);
    uint16_t* height2 = reinterpret_cast<uint16_t*>(Addr::YResolution2);

    DWORD oldProtect = 0;

    VirtualProtect(width1,sizeof(uint16_t),PAGE_EXECUTE_READWRITE,&oldProtect);

    *width1 = width;
    *width2 = width;

    VirtualProtect(height1,sizeof(uint16_t),PAGE_EXECUTE_READWRITE,&oldProtect);

    *height1 = height;
    *height2 = height;
}

void ApplyAspect()
{
    // Адреса
    // Camera & cars in menu
    void* addr1 = (void*)0x405D65;
    void* addr2 = (void*)0x4DD915;
    void* addr3 = (void*)0x4DD9B6;
    // HUD
    void* addr_racePos = (void*)0x56D0C2;     //racePos   0x23
    void* addr_lapLabel = (void*)0x56D0DA;     //lapLabel  0x23
    void* addr_splUzel = (void*)0x56D04A;     //splUzel   0x83
    void* addr_cockL1 = (void*)0x56D6E2;     //cockL1    0x97
    void* addr_cockL2 = (void*)0x56D6DB;     //cockL2    0xBC
    void* addr_cockR1 = (void*)0x56D6EA;     //cockR1    0x34
    void* addr_cockR2 = (void*)0x56D6F2;     //cockR2    0x83
    void* addr_overtR = (void*)0x56D78A;     //overtR    0x23
    void* addr_overtL = (void*)0x56D782;     //overtW    0xF5
    void* addr_enum = (void*)0x56D3F2;    //enum       0x62
    void* addr_map1 = (void*)0x56ECA2;    //map 1      0x54

    void* addr_map2 = (void*)0x56ECB2;    //map 2      0x54
    void* addr_msg_Pause = (void*)0x56D282;    //msgPause   0xA3
    void* addr_msg_Low = (void*)0x56D27A;    //msgLow     0x23
    void* addr_msg_Wrong = (void*)0x56D28A;    //msgWrong   0x23
    void* addr_msg_Hit = (void*)0x56D292;    //msgHit     0xA3
    void* addr_msg_SmashHit = (void*)0x56D29A;    //msgSmHit   0xA3
    void* addr_msg_3 = (void*)0x56D2A2;    //3          0x23
    void* addr_msg_2 = (void*)0x56D2AA;    //2          0x23
    void* addr_msg_1 = (void*)0x56D2B2;    //1          0xBD
    void* addr_msg_Finish = (void*)0x56D2BA;    //finish     0xC6
    void* addr_msg_Start = (void*)0x56D2C2;   //start       0xF5
    void* addr_msg_BestLap = (void*)0x56D2CA;   //bestLap     0x23
    void* addr_mm_logo = (void*)0x56ECBA;   //mm logo     0x57
    void* addr_mm_logo2 = (void*)0x56E982;   //mm logo2    0x57
    void* addr_logoCR = (void*)0x56B8EE;   //mm cr       0x4F
    void* addr_logo1C = (void*)0x56B8DE;   //mm 1c       0x43
    void* addr_MapPosX1 = (void*)0x56EC9A;    //MapPosX1
    void* addr_MapPosX2 = (void*)0x56ECAA;    //MapPosX2
    void* addr_logoRC1 = (void*)0x56B8BE;   //mm RC1      0x23
    void* addr_logoRC2 = (void*)0x56B8C6;   //mm RC2      0x4c

    void* addr_map1test = (void*)0x56ECA0;    //map 1      0x54

    //void* addr_load_map1 = (void*)0x5708A6;   4c  -   7f
    //void* addr_load_map1 = (void*)0x5708AE;   57  -   48

    float decAspect = (float)cfg.Width / cfg.Height;

    if (cfg.Aspect == 0)
    {
        if (decAspect >= 1.32f && decAspect <= 1.34f)       //16:9
            cfg.Aspect = 2;
        else if (decAspect >= 1.76f && decAspect <= 1.78f)  //4:3
            cfg.Aspect = 1;
        else if (decAspect >= 3.54f && decAspect <= 3.56f)  //32:9
            cfg.Aspect = 3;
        else if (decAspect >= 2.36f && decAspect <= 2.39f)  //21:9
            cfg.Aspect = 4;
        else if (decAspect >= 1.59f && decAspect <= 1.61f)  //16:10
            cfg.Aspect = 5;
        else if (decAspect >= 1.24f && decAspect <= 1.26f)  //5:4
            cfg.Aspect = 6;
    }

    // Значения
    if (cfg.Aspect == 1) { // 16:9

        uint8_t val = 0x10;

        WriteBytes(addr1, &val, 1);

        if (cfg.LegacyFixHUD == 1)
        {
            WriteBytes(addr2, &val, 1);
            WriteBytes(addr3, &val, 1);
            uint8_t val0 = 0x00;
            WriteBytes(addr_racePos, &val0, 1);
            WriteBytes(addr_lapLabel, &val0, 1);
            WriteBytes(addr_splUzel, &val0, 1);
            WriteBytes(addr_overtR, &val0, 1);
            WriteBytes(addr_msg_Low, &val0, 1);
            WriteBytes(addr_msg_Wrong, &val0, 1);
            WriteBytes(addr_msg_3, &val0, 1);
            WriteBytes(addr_msg_2, &val0, 1);
            WriteBytes(addr_msg_BestLap, &val0, 1);
            val0 = 0x70;
            WriteBytes(addr_cockL1, &val0, 1);
            val0 = 0xBB;
            WriteBytes(addr_cockL2, &val0, 1);
            val0 = 0x45;
            WriteBytes(addr_cockR1, &val0, 1);
            val0 = 0x81;
            WriteBytes(addr_cockR2, &val0, 1);
            val0 = 0xB8;
            WriteBytes(addr_overtL, &val0, 1);
            WriteBytes(addr_msg_Start, &val0, 1);
            val0 = 0x31;
            WriteBytes(addr_enum, &val0, 1);
            val0 = 0x80;
            WriteBytes(addr_msg_Pause, &val0, 1);
            WriteBytes(addr_msg_Hit, &val0, 1);
            WriteBytes(addr_msg_SmashHit, &val0, 1);
            val0 = 0x8E;
            WriteBytes(addr_msg_1, &val0, 1);
            val0 = 0x94;
            WriteBytes(addr_msg_Finish, &val0, 1);
            val0 = 0x20;
            WriteBytes(addr_map1, &val0, 1);
            WriteBytes(addr_map2, &val0, 1);
            val0 = 0x61;
            WriteBytes(addr_mm_logo, &val0, 1);
            WriteBytes(addr_mm_logo2, &val0, 1);
            val0 = 0x5A;
            WriteBytes(addr_logoCR, &val0, 1);

            val0 = 0x1d;
            WriteBytes(addr_logo1C, &val0, 1);

            uint8_t nop[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };    //disable function
            WriteBytes((uint16_t*)0x4CA835, nop, 6);
            WriteBytes((uint16_t*)0x4CA8B1, nop, 6);

            val0 = 0x54;
            WriteBytes(addr_MapPosX1, &val0, 1);
            WriteBytes(addr_MapPosX2, &val0, 1);

            val0 = 0x83;
            WriteBytes(addr_logoRC1, &val0, 1);
            val0 = 0x3c;
            WriteBytes(addr_logoRC2, &val0, 1);

            //Arrow
            val0 = 0x37;
            WriteBytes((uint16_t*)0x5547FA, &val0, 1);
        }
    }
    //else if (cfg.Aspect == 2) { // 4:3
    //    uint8_t val = 0x40;
    //    WriteBytes(reinterpret_cast<void*>(Addr::Aspect1), &val, 1);
    //    WriteBytes(reinterpret_cast<void*>(Addr::Aspect2), &val, 1);
    //    WriteBytes(reinterpret_cast<void*>(Addr::Aspect3), &val, 1);
    //}
    else if (cfg.Aspect == 3) { // 32:9
        uint8_t val[2] = { 0x90, 0x3E };
        WriteBytes(addr1, val, 2);

        if (cfg.LegacyFixHUD == 1)
        {
            WriteBytes(addr2, val, 2);
            WriteBytes(addr3, val, 2);
        }
    }
    else if (cfg.Aspect == 4) { // 21:9
        uint8_t val[2] = { 0xD7, 0x3E };
        WriteBytes(addr1, val, 2);

        if (cfg.LegacyFixHUD == 1)
        {
            WriteBytes(addr2, val, 2);
            WriteBytes(addr3, val, 2);
        }
    }
    else if (cfg.Aspect == 5) { // 16:10
        uint8_t val = 0x20;
        WriteBytes(addr1, &val, 1);

        if (cfg.LegacyFixHUD == 1)
        {
            WriteBytes(addr2, &val, 2);
            WriteBytes(addr3, &val, 2);
        }
    }
    else if (cfg.Aspect == 6) { // 5:4
        uint8_t val = 0x50;
        WriteBytes(addr1, &val, 1);

        if (cfg.LegacyFixHUD == 1)
        {
            WriteBytes(addr2, &val, 2);
            WriteBytes(addr3, &val, 2);
        }
    }
}

void ApplyLate() 
{
    if (cfg.Aspect == 1 && cfg.LegacyFixHUD == 1)
    {
        uint8_t val0 = 0x52;
        //LapPos
        WriteBytes(reinterpret_cast<void*>(Addr::LateLapPos1), &val0, 1);
        WriteBytes(reinterpret_cast<void*>(Addr::LateLapPos2), &val0, 1);
        WriteBytes(reinterpret_cast<void*>(Addr::LateLapPos3), &val0, 1);

        val0 = 0x70;
        //LapNumPos
        WriteBytes(reinterpret_cast<void*>(Addr::LateLapNumPos1), &val0, 1);
        WriteBytes(reinterpret_cast<void*>(Addr::LateLapNumPos2), &val0, 1);
        WriteBytes(reinterpret_cast<void*>(Addr::LateLapNumPos3), &val0, 1);

        //OvertakingBRectOffset
        val0 = 0x50;
        WriteBytes(reinterpret_cast<void*>(Addr::LateOvertakingBRect), &val0, 1);
    }
}

void ApplyFOV()
{
    int value = static_cast<int>( cfg.CustomFOV * 2.0);

    if (cfg.FixFOV == 1)
    {
        if (cfg.Aspect == 1) 
            value = static_cast<int>(96.5 * 2);

        if (cfg.Aspect == 2)
            value = 80 * 2;

        if (cfg.Aspect == 3)
            value = 132 * 2;

        if (cfg.Aspect == 4)
            value = static_cast<int>( 112.5 * 2 );

        if (cfg.Aspect == 5)
            value = static_cast<int>( 90.5 * 2 );

        if (cfg.Aspect == 6)
            value = 80 * 2;
    }

    if (value > 255)
        value = 255;

    WriteBytes(reinterpret_cast<void*>(Addr::FOV), &value, 1);
}

void ApplyDebug()
{
    if (cfg.NoMinimize == 1)
    {
        const uint8_t value = 0x75;
        WriteBytes(reinterpret_cast<void*>(Addr::NoMinimizePatch),&value,1);
    }
}

DWORD WINAPI InitThread(LPVOID)
{
    LoadConfig();

    ApplyResolution();

    ApplyDebug();

    QueryPerformanceFrequency(&g_QPCFreq);

    timeBeginPeriod(1);

    InstallPatch();

    ApplyAspect();

    ApplyFOV();

    InstallHUDFix();

    while (true)
    {
        *legacy_TargetFPS = 0;

        const uint8_t lateMemValue = *reinterpret_cast<volatile uint8_t*>(Addr::LateLapPos1);

        if (lateMemValue == (uint8_t)0x00 || lateMemValue == (uint8_t)0x44)
            ApplyLate();

        Sleep(1);
    }

    return 0;
}