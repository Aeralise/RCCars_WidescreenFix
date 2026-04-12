#include <Windows.h>
#include <cstdint>
#include "MinHook.h"
#include <chrono>

struct Config 
{
    int Width = 1920;
    int Height = 1080;
    int Aspect = 1;
    int FOV = 100;
};

LARGE_INTEGER gFrequency;
LARGE_INTEGER gLastTime;

double gTargetFrameTime = 0.0;

bool gSpeedhackEnabled = true;
double gSpeedMultiplier = 1.0;
double BaseFPS = 60.0;

LARGE_INTEGER gFakeTime = {};
LARGE_INTEGER gStartReal = {};

Config cfg;

//Хук QueryPerformanceCounter
typedef BOOL(WINAPI* QPC_t)(LARGE_INTEGER*);
QPC_t oQPC = nullptr;

BOOL WINAPI hkQPC(LARGE_INTEGER* lpPerformanceCount) 
{
    BOOL result = oQPC(lpPerformanceCount);

    if (!gSpeedhackEnabled)
        return result;

    LARGE_INTEGER realNow = *lpPerformanceCount;

    double realDelta = (double)(realNow.QuadPart - gStartReal.QuadPart);

    double scaledDelta = realDelta * gSpeedMultiplier;

    lpPerformanceCount->QuadPart = gStartReal.QuadPart + (LONGLONG)scaledDelta;

    return result;
}

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
}

//Хук установки
void HookTime() 
{
    HMODULE hKernel = GetModuleHandleA("kernel32.dll");

    void* addr = GetProcAddress(hKernel, "QueryPerformanceCounter");

    MH_CreateHook(addr, &hkQPC, (void**)&oQPC);
    MH_EnableHook(addr);

    QueryPerformanceCounter(&gStartReal);
}

void InitFPSLimiter(int fps) 
{
    QueryPerformanceFrequency(&gFrequency);
    QueryPerformanceCounter(&gLastTime);

    gTargetFrameTime = 1.0 / (double)fps;
}

void FrameLimiter() 
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    double elapsed = (double)(currentTime.QuadPart - gLastTime.QuadPart) / gFrequency.QuadPart;

    double currentFPS = 1.0 / elapsed;

    // динамический множитель
    gSpeedMultiplier = currentFPS / (double)BaseFPS;

    if (elapsed < gTargetFrameTime) {
        double remaining = gTargetFrameTime - elapsed;

        if (remaining > 0.002) {
            DWORD sleepTime = (DWORD)((remaining - 0.001) * 1000);
            Sleep(sleepTime);
        }

        do {
            QueryPerformanceCounter(&currentTime);
            elapsed = (double)(currentTime.QuadPart - gLastTime.QuadPart) / gFrequency.QuadPart;
        } while (elapsed < gTargetFrameTime);
    }

    gLastTime = currentTime;
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
    void* addr1 = (void*)0x405D65;
    void* addr2 = (void*)0x4DD915;
    void* addr3 = (void*)0x4DD9B6;

    // Значения
    if (cfg.Aspect == 1) { // 16:9
        uint8_t val = 0x10;
        WriteBytes(addr1, &val, 1);
        WriteBytes(addr2, &val, 1);
        WriteBytes(addr3, &val, 1);
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

void ApplyFOV()
{
    void* addr = (void*)0x405D6F;

    int value = cfg.FOV;
    WriteBytes(addr, &value, 1);
}

void ApplyResolution() 
{
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

DWORD WINAPI InitThread(LPVOID) 
{
    //Sleep(500);

    LoadConfig();

    ApplyResolution();
    ApplyAspect();
    ApplyFOV();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD reason,
    LPVOID) 
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
    }
    return TRUE;
}

