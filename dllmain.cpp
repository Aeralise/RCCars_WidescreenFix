#include <Windows.h>
#include <cstdint>
#include <mmsystem.h>
#include <chrono>
#include <cmath>


#pragma comment(lib, "winmm.lib")

struct Config 
{
    int Width = 1920;
    int Height = 1080;
    int Aspect = 1;
    int FOV = 193;
    int FixHUD = 1;

    //int Patch_4GB = 1;

    int NoMinimize = 0;
};

Config cfg;

// =====================================================
// CONFIG
// =====================================================

//bool g_EnableFix = true;
int g_MaxDelta = 100;
volatile int* legacy_TargetFPS = (int*)0x0148FAD0;
bool g_Enabled = true;
double g_TargetFPS = 144.0;

// EMA smoothing factor
double g_SmoothingFactor = 0.10;

// =====================================================
// ORIGINAL FUNCTION
// =====================================================

typedef void(__cdecl* t_gmpSetElapsedTime)(
    int,
    float,
    float);

t_gmpSetElapsedTime Real_gmpSetElapsedTime = (t_gmpSetElapsedTime)0x004407A0;

// =====================================================
// PATCH LOCATION
// =====================================================

// CALL _gmpSetElapsedTime
static uintptr_t g_CallPatch = 0x0048BD41;

// =====================================================
// TIMING
// =====================================================

LARGE_INTEGER g_QPCFreq;

LARGE_INTEGER g_LastFrameCounter;

bool g_TimerInitialized = false;

// smoothed frametime
double g_SmoothedMS = 0.0;

// fractional integer cadence accumulator
double g_FractionalAccumulator = 0.0;

// =====================================================
// FPS LIMITER + FRAME TIMER
// =====================================================

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

// =====================================================
// SMOOTH FRAME TIME
// =====================================================

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

// =====================================================
// HOOK
// =====================================================

BYTE g_OriginalBytes[5];

uintptr_t g_HookAddress = 0x004407A0;


// =====================================================
// STABLE INTEGER CADENCE
// =====================================================

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

// =====================================================
// REPLACEMENT CALL TARGET
// =====================================================

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

    // -------------------------------------------------
    // FPS limiter + real frame timing
    // -------------------------------------------------

    double realFrameMS = WaitAndMeasureFrame();

    // -------------------------------------------------
    // Smooth unstable frametimes
    // -------------------------------------------------

    double smoothMS = SmoothFrameTime(realFrameMS);

    // -------------------------------------------------
    // Convert to stable integer cadence
    // -------------------------------------------------

    int normalizedMS = GenerateNormalizedMS(smoothMS);

    // -------------------------------------------------
    // Feed normalized timestep into GMP
    // -------------------------------------------------

    Real_gmpSetElapsedTime(
        normalizedMS,
        minStep,
        maxStep);
}

// =====================================================
// INSTALL CALL PATCH
// =====================================================

void InstallPatch()
{
    BYTE* call = (BYTE*)g_CallPatch;

    DWORD oldProtect;

    VirtualProtect(call,5,PAGE_EXECUTE_READWRITE,&oldProtect);

    // CALL My_gmpSetElapsedTime
    call[0] = 0xE8;

    uintptr_t relative = (uintptr_t)My_gmpSetElapsedTime - (g_CallPatch + 5);

    *(uintptr_t*)(call + 1) = relative;

    VirtualProtect(call,5,oldProtect,&oldProtect);

    FlushInstructionCache(GetCurrentProcess(),call,5);
}

// =====================================================
// LOAD CONFIG
// =====================================================

void LoadConfig() 
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);

    char* slash = strrchr(path, '\\');
    if (slash) *(slash + 1) = '\0';

    strcat_s(path, "RCCars_WidescreenFix.ini");

    cfg.Width = GetPrivateProfileIntA("MAIN", "Width", 1920, path);
    cfg.Height = GetPrivateProfileIntA("MAIN", "Height", 1080, path);
    cfg.Aspect = GetPrivateProfileIntA("MAIN", "Aspect", 1, path);
    cfg.FOV = GetPrivateProfileIntA("MAIN", "FOV", 100, path);
    cfg.FixHUD = GetPrivateProfileIntA("MAIN", "FixHUD", 1, path);

    //cfg.Patch_4GB = GetPrivateProfileIntA("MISC", "Patch_4GB", 1, path);
    cfg.NoMinimize = GetPrivateProfileIntA("DEBUG", "NoMinimize", 0, path);

    //g_Enabled = GetPrivateProfileIntA("Fix","Enabled",1, path) != 0;

    g_TargetFPS =(double)GetPrivateProfileIntA("FPS","Limit",60,path);

    if (g_TargetFPS < 1.0)
        g_TargetFPS = 60.0;
}


void WriteBytes(void* address, void* data, size_t size) 
{
    DWORD oldProtect;
    VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(address, data, size);
    VirtualProtect(address, size, oldProtect, &oldProtect);
}

void ApplyAspect() 
{
    // Адреса
    // Camera & cars in menu
    void* addr1 = (void*)0x405D65;
    void* addr2 = (void*)0x4DD915;
    void* addr3 = (void*)0x4DD9B6;
    // HUD
    void* addrH1 = (void*)0x56D0C2;     //racePos   23
    void* addrH2 = (void*)0x56D0DA;     //lapLabel  23
    void* addrH3 = (void*)0x56D04A;     //splUzel   83
    void* addrH4 = (void*)0x56D6E2;     //cockL1    97
    void* addrH5 = (void*)0x56D6DB;     //cockL2    BC
    void* addrH6 = (void*)0x56D6EA;     //cockR1    34
    void* addrH7 = (void*)0x56D6F2;     //cockR2    83
    void* addrH8 = (void*)0x56D78A;     //overtR    23
    void* addrH9 = (void*)0x56D782;     //overtW    F5
    void* addrH10 = (void*)0x56D3F2;    //enum      62
    void* addrH11 = (void*)0x56ECA2;    //map 1     54
    void* addrH12 = (void*)0x56ECB2;    //map 2     54
    void* addrHM1 = (void*)0x56D282;    //msgPause  A3
    void* addrHM2 = (void*)0x56D27A;    //msgLow    23
    void* addrHM3 = (void*)0x56D28A;    //msgWrong  23
    void* addrHM4 = (void*)0x56D292;    //msgHit    A3
    void* addrHM5 = (void*)0x56D29A;    //msgSmHit  A3
    void* addrHM6 = (void*)0x56D2A2;    //3         23
    void* addrHM7 = (void*)0x56D2AA;    //2         23
    void* addrHM8 = (void*)0x56D2B2;    //1         BD
    void* addrHM9 = (void*)0x56D2BA;    //finish    C6
    void* addrHM10 = (void*)0x56D2C2;   //start     F5
    void* addrHM11 = (void*)0x56D2CA;   //bestLap   23
    void* addrH13 = (void*)0x56ECBA;   //mm logo    57
    void* addrH14 = (void*)0x56B8EE;   //mm cr      4F
    void* addrH15 = (void*)0x56B8DE;   //mm 1c      43
    void* addrH16 = (void*)0x56EC9A;    //MapPosX1
    void* addrH17 = (void*)0x56ECAA;    //MapPosX2


    // Значения
    if (cfg.Aspect == 1) { // 16:9
        uint8_t val = 0x10;
        WriteBytes(addr1, &val, 1);
        WriteBytes(addr2, &val, 1);
        WriteBytes(addr3, &val, 1);

        if (cfg.FixHUD == 1) {
            uint8_t val0 = 0x00;
            WriteBytes(addrH1, &val0, 1);
            WriteBytes(addrH2, &val0, 1);
            WriteBytes(addrH3, &val0, 1);
            WriteBytes(addrH8, &val0, 1);
            WriteBytes(addrHM2, &val0, 1);
            WriteBytes(addrHM3, &val0, 1);
            WriteBytes(addrHM6, &val0, 1);
            WriteBytes(addrHM7, &val0, 1);
            WriteBytes(addrHM11, &val0, 1);
            val0 = 0x70;
            WriteBytes(addrH4, &val0, 1);
            val0 = 0xBB;
            WriteBytes(addrH5, &val0, 1);
            val0 = 0x45;
            WriteBytes(addrH6, &val0, 1);
            val0 = 0x81;
            WriteBytes(addrH7, &val0, 1);
            val0 = 0xB8;
            WriteBytes(addrH9, &val0, 1);
            WriteBytes(addrHM10, &val0, 1);
            val0 = 0x31;
            WriteBytes(addrH10, &val0, 1);
            val0 = 0x80;
            WriteBytes(addrHM1, &val0, 1);
            WriteBytes(addrHM4, &val0, 1);
            WriteBytes(addrHM5, &val0, 1);
            val0 = 0x8E;
            WriteBytes(addrHM8, &val0, 1);
            val0 = 0x94;
            WriteBytes(addrHM9, &val0, 1);
            val0 = 0x20;
            WriteBytes(addrH11, &val0, 1);
            WriteBytes(addrH12, &val0, 1);
            val0 = 0x61;
            WriteBytes(addrH13, &val0, 1);
            val0 = 0x5A;
            WriteBytes(addrH14, &val0, 1);
            val0 = 0x32;
            WriteBytes(addrH15, &val0, 1);
            uint8_t nop[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
            WriteBytes((uint16_t*)0x4CA835, nop, 6);
            WriteBytes((uint16_t*)0x4CA8B1, nop, 6);
            val0 = 0x54;
            WriteBytes(addrH16, &val0, 1);
            WriteBytes(addrH17, &val0, 1);                     
            
            //Arrow
            val0 = 0x37;
            WriteBytes((uint16_t*)0x5547FA, &val0, 1);

            Sleep(8000);

            val0 = 0x52;
            //LapPos
            WriteBytes((uint16_t*)0x149DCEA, &val0, 1);
            WriteBytes((uint16_t*)0x149DCFA, &val0, 1);
            WriteBytes((uint16_t*)0x149DCF2, &val0, 1);

            val0 = 0x70;
            //LapNumPos
            WriteBytes((uint16_t*)0x149DD02, &val0, 1);
            WriteBytes((uint16_t*)0x149DD0A, &val0, 1);
            WriteBytes((uint16_t*)0x149DD12, &val0, 1);

            //OvertakingBRectOffset
            val0 = 0x50;
            WriteBytes((uint16_t*)0x149E1FA, &val0, 1);
        }
    }
    else if (cfg.Aspect == 2) { // 4:3
        uint8_t val = 0x40;
        WriteBytes(addr1, &val, 1);
        WriteBytes(addr2, &val, 1);
        WriteBytes(addr3, &val, 1);
    }
    else if (cfg.Aspect == 3) { // 32:9
        uint8_t val[2] = { 0x90, 0x3E };
        WriteBytes(addr1, val, 2);
        WriteBytes(addr2, val, 2);
        WriteBytes(addr3, val, 2);
    }
}

void ApplyFOV() {
    void* addr = (void*)0x405D6F;

    int value = cfg.FOV;
    WriteBytes(addr, &value, 1);
}

void ApplyResolution() {
    uint16_t width = (uint16_t)cfg.Width;
    uint16_t height = (uint16_t)cfg.Height;

    uint16_t* width1 = (uint16_t*)0x4A6A0D;
    uint16_t* width2 = (uint16_t*)0x4A69D3;

    uint16_t* height1 = (uint16_t*)0x4A6A08;
    uint16_t* height2 = (uint16_t*)0x4A69CE;

    DWORD oldProtect;

    VirtualProtect(width1, sizeof(uint16_t), PAGE_EXECUTE_READWRITE, &oldProtect);
    *width1 = width;
    *width2 = width;

    VirtualProtect(height1, sizeof(uint16_t), PAGE_EXECUTE_READWRITE, &oldProtect);
    *height1 = height;
    *height2 = height;
}

void ApplyDebug() {
    if (cfg.NoMinimize == 1)
    {
        uint8_t val = 0x75;
        void* addr = (void*)0x441177;
        WriteBytes(addr, &val, 1);
    }
}

//void ApplyMisc() {
//    if (cfg.Patch_4GB == 1)
//    {
//        uint8_t val = 0x2F;
//        void* addr = (void*)0x40012E;
//        WriteBytes(addr, &val, 1);
//        uint8_t val2[2] = { 0x58,  0x60};
//        void* addr2 = (void*)0x400170;
//        WriteBytes(addr2, val2, 2);
//    }
//}

DWORD WINAPI InitThread(LPVOID) {


    LoadConfig();

    ApplyResolution();
    

    ApplyDebug();

    QueryPerformanceFrequency(&g_QPCFreq);

    timeBeginPeriod(1);
    InstallPatch();

    ApplyFOV();
    ApplyAspect();
    

    while (true)
    {
        *legacy_TargetFPS = 0;

        Sleep(1);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD reason,
    LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
    }
    return TRUE;
}