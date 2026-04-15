#include <Windows.h>
#include <cstdint>

struct Config 
{
    int Width = 1920;
    int Height = 1080;
    int Aspect = 1;
    int FOV = 100;
    int FixHUD = 1;

    int NoMinimize = 0;
};

Config cfg;

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

    cfg.NoMinimize = GetPrivateProfileIntA("DEBUG", "NoMinimize", 0, path);
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
    //void* addrH11 = = (void*)0x   //map
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
            val0 = 0x69;
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

DWORD WINAPI InitThread(LPVOID) {
    //Sleep(500);

    LoadConfig();

    ApplyResolution();
    ApplyAspect();
    ApplyFOV();

    ApplyDebug();

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