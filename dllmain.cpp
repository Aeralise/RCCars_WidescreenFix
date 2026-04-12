#include <Windows.h>
//#include <d3d8.h>
#include <d3d9.h>
#include <cstdint>
#include <chrono>
#include "MinHook.h"
#pragma comment (lib, "winmm.lib")


// ================= CONFIG =================

struct Config {
    int Width = 1920;
    int Height = 1080;
    int Aspect = 1;
    int FOV = 100;

    int FPSLimit = 60;

    int SpeedEnable = 1;
    int BaseFPS = 60;
};

Config cfg;

// ================= FPS LIMITER =================

LARGE_INTEGER gFreq, gLast;
double gTargetFrameTime = 0.0;

void InitFPS() 
{
    QueryPerformanceFrequency(&gFreq);
    QueryPerformanceCounter(&gLast);
    gTargetFrameTime = 1.0 / cfg.FPSLimit;
}

//double FrameLimiter() 
//{
//    LARGE_INTEGER now;
//    QueryPerformanceCounter(&now);
//
//    double elapsed = (double)(now.QuadPart - gLast.QuadPart) / gFreq.QuadPart;
//
//    if (elapsed < gTargetFrameTime) {
//        double remain = gTargetFrameTime - elapsed;
//
//        if (remain > 0.002)
//            Sleep((DWORD)((remain - 0.001) * 1000));
//
//        do {
//            QueryPerformanceCounter(&now);
//            elapsed = (double)(now.QuadPart - gLast.QuadPart) / gFreq.QuadPart;
//        } while (elapsed < gTargetFrameTime);
//    }
//
//    gLast = now;
//    return elapsed;
//}

// ================= SPEEDHACK =================

typedef BOOL(WINAPI* QPC_t)(LARGE_INTEGER*);
QPC_t oQPC = nullptr;

double gSpeedMultiplier = 1.0;
LARGE_INTEGER gStartReal;

BOOL WINAPI hkQPC(LARGE_INTEGER* lp) 
{
    BOOL ret = oQPC(lp);

    if (!cfg.SpeedEnable)
        return ret;

    LARGE_INTEGER now = *lp;

    double delta = (double)(now.QuadPart - gStartReal.QuadPart);
    delta *= gSpeedMultiplier;

    lp->QuadPart = gStartReal.QuadPart + (LONGLONG)delta;

    return ret;
}

void HookTime() 
{
    MH_CreateHook(GetProcAddress(GetModuleHandleA("kernel32.dll"),
        "QueryPerformanceCounter"),
        &hkQPC, (void**)&oQPC);

    MH_EnableHook(MH_ALL_HOOKS);

    QueryPerformanceCounter(&gStartReal);
}

// ================= D3D HOOKS =================

//typedef IDirect3D9* (WINAPI* Direct3DCreate9_t)(UINT);
//Direct3DCreate9_t oDirect3DCreate9 = nullptr;
//
//typedef HRESULT(__stdcall* Present9_t)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
//Present9_t oPresent9 = nullptr;
//
//// ---- DX9
//HRESULT __stdcall hkPresent9(IDirect3DDevice9* dev, const RECT* a, const RECT* b, HWND c, const RGNDATA* d) {
//    double dt = FrameLimiter();
//
//    if (cfg.SpeedEnable) {
//        double fps = 1.0 / dt;
//        gSpeedMultiplier = fps / (double)cfg.BaseFPS;
//    }
//
//    return oPresent9(dev, a, b, c, d);
//}
//
//
//typedef HRESULT(__stdcall* CreateDevice_t)(
//    IDirect3D9*,
//    UINT,
//    D3DDEVTYPE,
//    HWND,
//    DWORD,
//    D3DPRESENT_PARAMETERS*,
//    IDirect3DDevice9**
//    );
//
//CreateDevice_t oCreateDevice9 = nullptr;
//
//HRESULT __stdcall hkCreateDevice9(
//    IDirect3D9* self,
//    UINT Adapter,
//    D3DDEVTYPE Type,
//    HWND hFocusWindow,
//    DWORD BehaviorFlags,
//    D3DPRESENT_PARAMETERS* pPresentationParameters,
//    IDirect3DDevice9** ppReturnedDeviceInterface)
//{
//    HRESULT hr = oCreateDevice9(
//        self, Adapter, Type, hFocusWindow,
//        BehaviorFlags, pPresentationParameters,
//        ppReturnedDeviceInterface
//    );
//
//    if (SUCCEEDED(hr) && ppReturnedDeviceInterface && *ppReturnedDeviceInterface) {
//        void** vtable = *reinterpret_cast<void***>(*ppReturnedDeviceInterface);
//
//        MH_CreateHook(vtable[17], &hkPresent9, (void**)&oPresent9);
//        MH_EnableHook(vtable[17]);
//    }
//
//    return hr;
//}
//
//IDirect3D9* WINAPI hkDirect3DCreate9(UINT SDKVersion) {
//    IDirect3D9* d3d = oDirect3DCreate9(SDKVersion);
//
//    if (d3d) {
//        void** vtable = *reinterpret_cast<void***>(d3d);
//
//        MH_CreateHook(vtable[16], &hkCreateDevice9, (void**)&oCreateDevice9);
//        MH_EnableHook(vtable[16]);
//    }
//
//    return d3d;
//}

//void HookD3D9Real() {
//    HMODULE hD3D9 = GetModuleHandleA("d3d9.dll");
//    if (!hD3D9) return;
//
//    void* addr = GetProcAddress(hD3D9, "Direct3DCreate9");
//
//    MH_CreateHook(addr, &hkDirect3DCreate9, (void**)&oDirect3DCreate9);
//    MH_EnableHook(addr);
//}
//
//void HookD3D9Present() {
//    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
//    if (!d3d) return;
//
//    D3DPRESENT_PARAMETERS pp = {};
//    pp.Windowed = TRUE;
//    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
//    pp.hDeviceWindow = GetForegroundWindow();
//
//    IDirect3DDevice9* dev = nullptr;
//
//    if (FAILED(d3d->CreateDevice(
//        D3DADAPTER_DEFAULT,
//        D3DDEVTYPE_HAL,
//        pp.hDeviceWindow,
//        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
//        &pp,
//        &dev)))
//    {
//        d3d->Release();
//        return;
//    }
//
//    void** vtable = *reinterpret_cast<void***>(dev);
//
//    void* presentAddr = vtable[17];
//
//    // 🔥 ВАЖНО: глобальный хук
//    MH_CreateHook(presentAddr, &hkPresent9, (void**)&oPresent9);
//    MH_EnableHook(presentAddr);
//
//    dev->Release();
//    d3d->Release();
//}

void GlobalLimiter() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    double elapsed = (double)(now.QuadPart - gLast.QuadPart) / gFreq.QuadPart;

    if (elapsed < gTargetFrameTime) {
        double remain = gTargetFrameTime - elapsed;

        if (remain > 0.002)
            Sleep((DWORD)((remain - 0.001) * 1000));

        do {
            QueryPerformanceCounter(&now);
            elapsed = (double)(now.QuadPart - gLast.QuadPart) / gFreq.QuadPart;
        } while (elapsed < gTargetFrameTime);
    }

    gLast = now;

    // Speedhack
    if (cfg.SpeedEnable) {
        double fps = 1.0 / elapsed;
        gSpeedMultiplier = fps / (double)cfg.BaseFPS;
    }
}

DWORD WINAPI LimiterThread(LPVOID) {
    while (true) {
        GlobalLimiter();
    }
}


// ================= MEMORY PATCH =================

void Write(void* addr, void* data, size_t size) 
{
    DWORD old;
    VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &old);
    memcpy(addr, data, size);
    VirtualProtect(addr, size, old, &old);
}

void ApplyFixes() 
{
    // Resolution
    uint16_t w = (uint16_t)cfg.Width;
    uint16_t h = (uint16_t)cfg.Height;

    Write((void*)0x4A6A0D, &w, 2);
    Write((void*)0x4A69D3, &w, 2);
    Write((void*)0x4A6A08, &h, 2);
    Write((void*)0x4A69CE, &h, 2);

    // Aspect
    uint8_t val = (cfg.Aspect == 1) ? 0x10 : (cfg.Aspect == 2) ? 0x40 : 0x10;

    Write((void*)0x405D65, &val, 1);
    Write((void*)0x4DD915, &val, 1);
    Write((void*)0x4DD9B6, &val, 1);

    // FOV
    Write((void*)0x405D6F, &cfg.FOV, 1);
}

// ================= INIT =================

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

    cfg.FPSLimit = GetPrivateProfileIntA("FPS", "Limit", 60, path);

    cfg.SpeedEnable = GetPrivateProfileIntA("SPEED", "Enable", 1, path);
    cfg.BaseFPS = GetPrivateProfileIntA("SPEED", "BaseFPS", 60, path);
}

DWORD WINAPI InitThread(LPVOID) 
{
    timeBeginPeriod(1);
    

    LoadConfig();

    ApplyFixes();
    
    //Sleep(100);

    InitFPS();

    MH_Initialize();

    HookTime();
    
    CreateThread(0, 0, LimiterThread, 0, 0, 0);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE h, DWORD r, LPVOID) 
{
    if (r == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        CreateThread(0, 0, InitThread, 0, 0, 0);
    }
    return TRUE;
}