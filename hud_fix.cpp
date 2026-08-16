#include "pch.h"

#include "hud_fix.h"
#include "game_addresses.h"
#include "fix_core.h"

#include <Windows.h>
#include <intrin.h>

#include <cstdint>
#include <cstring>
#include <cmath>

#include "MinHook.h"

#pragma comment(lib, "libMinHook.x86.lib")

#pragma intrinsic(_ReturnAddress)

// ============================================================
// RC Cars 2003
//
// COMBINED HUD FIX
//
// TARGET ASPECT:
//     4:3
//
// ============================================================
//
// CURRENTLY FIXED:
//
// MENU / UI:
//     FUN_0043EB10
//     - GMR_Cockpit
//     - GMR_Enum
//     - GMR_Edit
//     - GMR_Static
//     - GMR_Button
//     - GMR_HotKey
//     - GMR_Slider
//     - GMR_Progress
//     - GMR_MapRacer
//     - GMR_MsgBoxButton
//     - GMR_Header
//     - GMR_ShotList
//     - GMR_Table
//     - GMR_AnimPreview
//     - GMR_ChatConsole
//
// EXCLUDED FOR NOW:
//     GMR_VSrollBar
//
// RACE COCKPIT:
//     LAB_004AF060
//         message 0x16
//         temporary DAT_005647F0 = virtual 4:3 width
//
//     FUN_00471610
//         RacePos / LapPos / LapNumPos
//
//     FUN_004B2090
//         speedometer text path
//
//     FUN_004B2270
//         speedometer graphics / textures / bars
//
//     FUN_00470E30
//         center compensation for rectangles that are genuinely
//         centered around X=0.5
//
// MESSAGE BOX:
//
//     LAB_004CBD20
//         temporary 4:3 width while GMR_MsgBoxButton handler
//         processes its render message.
//
// ============================================================
//
// NOT FIXED YET:
//
//     timeTotalPos
//     timeLapPos
//     central msg texture
//     special Button_back hitbox
//     second-player ShotList/Canvas duplication
//     3D arrow
//
// ============================================================


// ============================================================
// ADDRESSES
// ============================================================

static constexpr uintptr_t ADDR_FUN_0043EB10 =  0x0043EB10;
static constexpr uintptr_t ADDR_LAB_004AF060 =  0x004AF060;
static constexpr uintptr_t ADDR_FUN_00471610 =  0x00471610;
static constexpr uintptr_t ADDR_FUN_004B2090 =  0x004B2090;
static constexpr uintptr_t ADDR_FUN_004B2270 =  0x004B2270;
static constexpr uintptr_t ADDR_FUN_00470E30 =  0x00470E30;

// Registered GMR_MsgBoxButton handler.
static constexpr uintptr_t ADDR_LAB_004CBD20 =  0x004CBD20;
static constexpr uintptr_t ADDR_DAT_005647F0 =  0x005647F0;
static constexpr uintptr_t ADDR_DAT_005647F4 =  0x005647F4;

// ============================================================
// FUN_004AFDC0 CALLER RANGE
// ============================================================

static constexpr uintptr_t RANGE_AFDC0_BEGIN =  0x004AFDC0;
static constexpr uintptr_t RANGE_AFDC0_END =    0x004B0265;

// ============================================================
// RENDER MESSAGE
// ============================================================

static constexpr uint32_t MSG_RENDER =  0x16;

// ============================================================
// TARGET HUD ASPECT
// ============================================================

static constexpr float HUD_ASPECT_X =   4.0f;
static constexpr float HUD_ASPECT_Y =   3.0f;

// ============================================================
// ENABLE
// ============================================================

static bool g_EnableFix = true;

// ============================================================
// GMR CLASS IDs
// ============================================================

static constexpr uint32_t CLASS_GMR_COCKPIT =       0x2643434B;
static constexpr uint32_t CLASS_GMR_ENUM =          0x26454E4D;
static constexpr uint32_t CLASS_GMR_EDIT =          0x26454454;
static constexpr uint32_t CLASS_GMR_STATIC =        0x26535454;
static constexpr uint32_t CLASS_GMR_BUTTON =        0x2642544E;
static constexpr uint32_t CLASS_GMR_HOTKEY =        0x26484B59;
static constexpr uint32_t CLASS_GMR_SLIDER =        0x26534C44;
static constexpr uint32_t CLASS_GMR_PROGRESS =      0x26504753;
static constexpr uint32_t CLASS_GMR_MAPRACER =      0x264D5052;
static constexpr uint32_t CLASS_GMR_MSGBOXBUTTON =  0x264D4242;
static constexpr uint32_t CLASS_GMR_HEADER =        0x26484452;
static constexpr uint32_t CLASS_GMR_SHOTLIST =      0x26534853;
static constexpr uint32_t CLASS_GMR_TABLE =         0x2654424C;
static constexpr uint32_t CLASS_GMR_VSROLLBAR =     0x26565342;
static constexpr uint32_t CLASS_GMR_ANIMPREVIEW =   0x26415056;
static constexpr uint32_t CLASS_GMR_CHATCONSOLE =   0x26434841;


// ============================================================
// ORIGINAL FUNCTION TYPES
// ============================================================


// ------------------------------------------------------------
// FUN_0043EB10
// ------------------------------------------------------------

using tFUN_0043EB10 = uint32_t(__cdecl*)(int,uint32_t*);

static tFUN_0043EB10 g_Original_43EB10 = nullptr;


// ------------------------------------------------------------
// LAB_004AF060
// ------------------------------------------------------------

using tLAB_004AF060 = uint32_t(__cdecl*)(uint32_t,uint32_t,uint32_t,int,int*,uint32_t*);

static tLAB_004AF060 g_Original_AF060 = nullptr;


// ------------------------------------------------------------
// FUN_00471610
// ------------------------------------------------------------

using tFUN_00471610 =
void(__cdecl*)(
    float*,
    uint32_t,
    uint32_t,
    uint8_t,
    uint32_t*
    );

static tFUN_00471610
g_Original_71610 =
nullptr;


// ------------------------------------------------------------
// FUN_004B2090
// ------------------------------------------------------------

using tFUN_004B2090 =
void(__cdecl*)(
    float*,
    uint32_t,
    uint32_t,
    uint32_t
    );

static tFUN_004B2090
g_Original_B2090 =
nullptr;


// ------------------------------------------------------------
// FUN_004B2270
// ------------------------------------------------------------

using tFUN_004B2270 =
void(__cdecl*)(
    float,
    float,
    float,
    float,
    float,
    float*,
    uint32_t,
    uint32_t
    );

static tFUN_004B2270
g_Original_B2270 =
nullptr;


// ------------------------------------------------------------
// FUN_00470E30
// ------------------------------------------------------------

using tFUN_00470E30 =
uint32_t(__cdecl*)(
    uint32_t,
    uint32_t,
    float*,
    uint32_t,
    uint32_t
    );

static tFUN_00470E30
g_Original_70E30 =
nullptr;


// ------------------------------------------------------------
// LAB_004CBD20
//
// GMR class handlers use the common GMP callback convention.
// LAB_004AF060 has this exact shape in the dump; the MsgBox
// handler is registered through the same gmpRegisterClass
// mechanism.
//
// ------------------------------------------------------------

using tLAB_004CBD20 =
uint32_t(__cdecl*)(
    uint32_t,
    uint32_t,
    uint32_t,
    int,
    int*,
    uint32_t*
    );

static tLAB_004CBD20 g_Original_CBD20 = nullptr;


// ============================================================
// COCKPIT SCOPE
// ============================================================

struct HUDScope
{
    bool active = false;

    uint32_t realWidth = 0;
    uint32_t realHeight = 0;

    uint32_t hudWidth = 0;
};

static thread_local HUDScope
g_CockpitScope;


// ============================================================
// MSGBOX SCOPE
// ============================================================

static thread_local bool
g_MsgBoxScopeActive = false;

static thread_local uint32_t
g_MsgBoxRealWidth = 0;

static thread_local uint32_t
g_MsgBoxHudWidth = 0;


// ============================================================
// BASIC MEMORY HELPERS
// ============================================================

static uint32_t ReadU32(
    uintptr_t address)
{
    return *reinterpret_cast<
        volatile uint32_t*
    >(address);
}


static void WriteU32(
    uintptr_t address,
    uint32_t value)
{
    *reinterpret_cast<
        volatile uint32_t*
    >(address) = value;
}


static float BitsToFloat(
    uint32_t bits)
{
    float value;

    std::memcpy(
        &value,
        &bits,
        sizeof(value)
    );

    return value;
}


static uint32_t FloatToBits(
    float value)
{
    uint32_t bits;

    std::memcpy(
        &bits,
        &value,
        sizeof(bits)
    );

    return bits;
}


static uintptr_t GetCaller()
{
    return reinterpret_cast<uintptr_t>( _ReturnAddress() );
}


static bool IsAFDC0Caller(
    uintptr_t caller)
{
    return
        caller >= RANGE_AFDC0_BEGIN &&
        caller < RANGE_AFDC0_END;
}


// ============================================================
// MEMORY READ CHECK
// ============================================================

static bool IsReadable(
    uintptr_t address,
    size_t size)
{
    if (address == 0 ||
        size == 0)
    {
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi{};

    if (VirtualQuery(
        reinterpret_cast<LPCVOID>(
            address
            ),
        &mbi,
        sizeof(mbi)
    ) == 0)
    {
        return false;
    }

    if (mbi.State != MEM_COMMIT)
        return false;

    if ((mbi.Protect & PAGE_NOACCESS) != 0)
        return false;

    if ((mbi.Protect & PAGE_GUARD) != 0)
        return false;

    const uintptr_t begin =
        reinterpret_cast<uintptr_t>(
            mbi.BaseAddress
            );

    const uintptr_t end =
        begin +
        mbi.RegionSize;

    if (address < begin ||
        address > end)
    {
        return false;
    }

    return size <=
        end - address;
}


// ============================================================
// RESOLUTION
// ============================================================

static void GetResolution(
    uint32_t& width,
    uint32_t& height)
{
    width =
        ReadU32(
            ADDR_DAT_005647F0
        );

    height =
        ReadU32(
            ADDR_DAT_005647F4
        );
}

// ============================================================
// 4:3 WIDTH
// ============================================================

static uint32_t CalculateHUDWidth(
    uint32_t width,
    uint32_t height)
{
    if (width == 0 ||
        height == 0)
    {
        return width;
    }

    float hudWidth =
        static_cast<float>(
            height
            ) *
        HUD_ASPECT_X /
        HUD_ASPECT_Y;

    if (hudWidth >
        static_cast<float>(
            width
            ))
    {
        hudWidth =
            static_cast<float>(
                width
                );
    }

    return static_cast<uint32_t>(
        hudWidth + 0.5f
        );
}

// ============================================================
// NORMALIZED TRANSFORM
// ============================================================

static void CalculateNormalizedTransform(
    uint32_t width,
    uint32_t height,
    float& scale,
    float& offset)
{
    scale = 1.0f;
    offset = 0.0f;

    if (width == 0 ||
        height == 0)
    {
        return;
    }

    const uint32_t hudWidth =
        CalculateHUDWidth(
            width,
            height
        );

    scale =
        static_cast<float>(
            hudWidth
            ) /
        static_cast<float>(
            width
            );

    offset =
        (
            1.0f -
            scale
            ) *
        0.5f;
}


// ============================================================
// SCOPE TRANSFORM
//
// DAT_005647F0 already contains HUDWidth.
//
// Therefore scale = 1.
//
// We only add the real physical left margin converted into
// the virtual-HUD coordinate system.
//
// ============================================================

static void CalculateScopedTransform(
    float& scale,
    float& offset)
{
    scale = 1.0f;
    offset = 0.0f;

    if (!g_CockpitScope.active ||
        g_CockpitScope.hudWidth == 0)
    {
        return;
    }

    const float margin =
        (
            static_cast<float>(
                g_CockpitScope.realWidth
                ) -
            static_cast<float>(
                g_CockpitScope.hudWidth
                )
            ) *
        0.5f;

    offset =
        margin /
        static_cast<float>(
            g_CockpitScope.hudWidth
            );
}


// ============================================================
// MSGBOX SCOPE TRANSFORM
// ============================================================

static float GetMsgBoxOffset()
{
    if (!g_MsgBoxScopeActive ||
        g_MsgBoxHudWidth == 0)
    {
        return 0.0f;
    }

    const float margin =
        (
            static_cast<float>(
                g_MsgBoxRealWidth
                ) -
            static_cast<float>(
                g_MsgBoxHudWidth
                )
            ) *
        0.5f;

    return
        margin /
        static_cast<float>(
            g_MsgBoxHudWidth
            );
}


// ============================================================
// HOOK #1
//
// FUN_0043EB10
//
// Generic GMR layout transform.
//
// VSrollBar remains excluded.
//
// ============================================================

static uint32_t __cdecl
Hook_43EB10(
    int object,
    uint32_t* rect)
{
    if (object == 0 ||
        rect == nullptr ||
        cfg.FixHUD == 0)
    {
        return g_Original_43EB10(
            object,
            rect
        );
    }

    const uintptr_t objectAddress =
        static_cast<uintptr_t>(
            static_cast<uint32_t>(
                object
                )
            );

    if (!IsReadable(
        objectAddress,
        0x30
    ))
    {
        return g_Original_43EB10(
            object,
            rect
        );
    }

    const uint32_t classId =
        ReadU32(
            objectAddress + 4
        );


    // --------------------------------------------------------
    // VSrollBar is deliberately excluded.
    // --------------------------------------------------------

    if (classId ==
        CLASS_GMR_VSROLLBAR)
    {
        return g_Original_43EB10(
            object,
            rect
        );
    }


    // --------------------------------------------------------
    // Supported GMR class whitelist.
    // --------------------------------------------------------

    switch (classId)
    {
    case CLASS_GMR_COCKPIT:
    case CLASS_GMR_ENUM:
    case CLASS_GMR_EDIT:
    case CLASS_GMR_STATIC:
    case CLASS_GMR_BUTTON:
    case CLASS_GMR_HOTKEY:
    case CLASS_GMR_SLIDER:
    case CLASS_GMR_PROGRESS:
    case CLASS_GMR_MAPRACER:
    case CLASS_GMR_MSGBOXBUTTON:
    case CLASS_GMR_HEADER:
    case CLASS_GMR_SHOTLIST:
    case CLASS_GMR_TABLE:
    case CLASS_GMR_ANIMPREVIEW:
    case CLASS_GMR_CHATCONSOLE:
        break;

    case CLASS_GMR_VSROLLBAR:
    default:
        return g_Original_43EB10(
            object,
            rect
        );
    }


    // --------------------------------------------------------
    // Input rect.
    // --------------------------------------------------------

    const float x1 =    BitsToFloat(rect[0]);

    const float y1 =    BitsToFloat(rect[1]);

    const float x2 =    BitsToFloat(rect[2]);

    const float y2 =    BitsToFloat(rect[3]);


    uint32_t screenWidth = 0;
    uint32_t screenHeight = 0;

    GetResolution(screenWidth,screenHeight);

    float scale = 1.0f;
    float offset = 0.0f;

    CalculateNormalizedTransform(
        screenWidth,
        screenHeight,
        scale,
        offset
    );

    float newX1 =
        offset +
        x1 * scale;

    float newX2 =
        offset +
        x2 * scale;

    // --------------------------------------------------------
    // RIGHT-EDGE BUTTON HITBOX
    //
    // Existing fix:
    //
    // If original button extends beyond 1.0, preserve the
    // original left edge and make its hitbox reach physical
    // screen right edge.
    //
    // This fixed most right-side buttons.
    // Button_back is still a known exception.
    // --------------------------------------------------------

    if (classId ==
        CLASS_GMR_BUTTON)
    {
        if (x2 > 1.0f)
        {
            newX1 = x1;
            newX2 = 1.0f;
        }
    }


    // --------------------------------------------------------
    // Build FLOAT bit-preserving uint32 rectangle.
    // --------------------------------------------------------

    uint32_t transformed[4];

    transformed[0] =FloatToBits(newX1);

    transformed[1] =FloatToBits(y1);

    transformed[2] =FloatToBits(newX2);

    transformed[3] =FloatToBits(y2);

    return g_Original_43EB10(object,transformed);
}


// ============================================================
// HOOK #2
//
// LAB_004AF060
//
// Cockpit render scope.
//
// ============================================================

static uint32_t __cdecl
Hook_AF060(
    uint32_t param_1,
    uint32_t param_2,
    uint32_t param_3,
    int param_4,
    int* param_5,
    uint32_t* param_6)
{
    if (param_1 != MSG_RENDER ||
        cfg.FixHUD == 0)
    {
        return g_Original_AF060(
            param_1,
            param_2,
            param_3,
            param_4,
            param_5,
            param_6
        );
    }

    const uint32_t realWidth =
        ReadU32(
            ADDR_DAT_005647F0
        );

    const uint32_t realHeight =
        ReadU32(
            ADDR_DAT_005647F4
        );

    const uint32_t hudWidth =
        CalculateHUDWidth(
            realWidth,
            realHeight
        );


    if (g_CockpitScope.active)
    {
        return g_Original_AF060(
            param_1,
            param_2,
            param_3,
            param_4,
            param_5,
            param_6
        );
    }


    g_CockpitScope.active =
        true;

    g_CockpitScope.realWidth =
        realWidth;

    g_CockpitScope.realHeight =
        realHeight;

    g_CockpitScope.hudWidth =
        hudWidth;


    // --------------------------------------------------------
    // Virtual 4:3 width.
    // --------------------------------------------------------

    WriteU32(
        ADDR_DAT_005647F0,
        hudWidth
    );


    const uint32_t result =
        g_Original_AF060(
            param_1,
            param_2,
            param_3,
            param_4,
            param_5,
            param_6
        );


    // --------------------------------------------------------
    // Restore REAL width.
    // --------------------------------------------------------

    WriteU32(
        ADDR_DAT_005647F0,
        realWidth
    );


    g_CockpitScope =
        HUDScope{};


    return result;
}


// ============================================================
// HOOK #3
//
// FUN_00471610
//
// RacePos / LapPos / LapNumPos.
//
// ============================================================

static void __cdecl
Hook_71610(
    float* param_1,
    uint32_t param_2,
    uint32_t param_3,
    uint8_t param_4,
    uint32_t* param_5)
{
    if (param_1 == nullptr ||
        cfg.FixHUD == 0)
    {
        g_Original_71610(
            param_1,
            param_2,
            param_3,
            param_4,
            param_5
        );

        return;
    }


    const uintptr_t caller =
        GetCaller();


    if (!IsAFDC0Caller(
        caller
    ))
    {
        g_Original_71610(
            param_1,
            param_2,
            param_3,
            param_4,
            param_5
        );

        return;
    }


    float scale = 1.0f;
    float offset = 0.0f;


    if (g_CockpitScope.active)
    {
        CalculateScopedTransform(
            scale,
            offset
        );
    }
    else
    {
        uint32_t width = 0;
        uint32_t height = 0;

        GetResolution(
            width,
            height
        );

        CalculateNormalizedTransform(
            width,
            height,
            scale,
            offset
        );
    }


    float transformed[4];

    transformed[0] =
        offset +
        param_1[0] *
        scale;

    transformed[1] =
        param_1[1];

    transformed[2] =
        offset +
        param_1[2] *
        scale;

    transformed[3] =
        param_1[3];


    g_Original_71610(
        transformed,
        param_2,
        param_3,
        param_4,
        param_5
    );
}


// ============================================================
// HOOK #4
//
// FUN_004B2090
//
// Speedometer text path.
//
// ============================================================

static void __cdecl
Hook_B2090(
    float* param_1,
    uint32_t param_2,
    uint32_t param_3,
    uint32_t param_4)
{
    if (param_1 == nullptr ||
        cfg.FixHUD == 0)
    {
        g_Original_B2090(
            param_1,
            param_2,
            param_3,
            param_4
        );

        return;
    }


    float scale = 1.0f;
    float offset = 0.0f;


    if (g_CockpitScope.active)
    {
        CalculateScopedTransform(
            scale,
            offset
        );
    }
    else
    {
        uint32_t width = 0;
        uint32_t height = 0;

        GetResolution(
            width,
            height
        );

        CalculateNormalizedTransform(
            width,
            height,
            scale,
            offset
        );
    }


    float transformed[4];

    transformed[0] =
        offset +
        param_1[0] *
        scale;

    transformed[1] =
        param_1[1];

    transformed[2] =
        offset +
        param_1[2] *
        scale;

    transformed[3] =
        param_1[3];


    g_Original_B2090(
        transformed,
        param_2,
        param_3,
        param_4
    );
}


// ============================================================
// HOOK #5
//
// FUN_004B2270
//
// Speedometer textures / bars.
//
// ============================================================

static void __cdecl
Hook_B2270(
    float param_1,
    float param_2,
    float param_3,
    float param_4,
    float param_5,
    float* param_6,
    uint32_t param_7,
    uint32_t param_8)
{
    if (param_6 == nullptr ||
        cfg.FixHUD == 0)
    {
        g_Original_B2270(
            param_1,
            param_2,
            param_3,
            param_4,
            param_5,
            param_6,
            param_7,
            param_8
        );

        return;
    }


    float scale = 1.0f;
    float offset = 0.0f;


    if (g_CockpitScope.active)
    {
        CalculateScopedTransform(
            scale,
            offset
        );
    }
    else
    {
        uint32_t width = 0;
        uint32_t height = 0;

        GetResolution(
            width,
            height
        );

        CalculateNormalizedTransform(
            width,
            height,
            scale,
            offset
        );
    }


    float transformed[4];

    transformed[0] =
        offset +
        param_6[0] *
        scale;

    transformed[1] =
        param_6[1];

    transformed[2] =
        offset +
        param_6[2] *
        scale;

    transformed[3] =
        param_6[3];


    g_Original_B2270(
        param_1,
        param_2,
        param_3,
        param_4,
        param_5,
        transformed,
        param_7,
        param_8
    );
}


// ============================================================
// HOOK #6
//
// FUN_00470E30
//
// Center compensation only.
//
// This is deliberately conservative.
// ============================================================

static uint32_t __cdecl
Hook_70E30(
    uint32_t param_1,
    uint32_t param_2,
    float* param_3,
    uint32_t param_4,
    uint32_t param_5)
{
    if (param_3 == nullptr ||
        cfg.FixHUD == 0 ||
        !g_CockpitScope.active)
    {
        return g_Original_70E30(
            param_1,
            param_2,
            param_3,
            param_4,
            param_5
        );
    }


    float rect[4];

    std::memcpy(
        rect,
        param_3,
        sizeof(rect)
    );


    // --------------------------------------------------------
    // Detect genuinely centered logical rectangle.
    //
    // This is NOT intended to catch every centered HUD element.
    // It only catches objects already centered in normalized
    // logical coordinates.
    // --------------------------------------------------------

    const float center =
        (
            rect[0] +
            rect[2]
            ) *
        0.5f;


    constexpr float TOLERANCE =
        0.005f;


    if (std::fabs(
        center - 0.5f
    ) <= TOLERANCE)
    {
        const float offset =
            GetMsgBoxOffset();


        // GetMsgBoxOffset is the same physical-margin logic,
        // but for the normal cockpit scope we use the cockpit
        // state directly.

        float actualOffset =
            offset;


        if (actualOffset == 0.0f)
        {
            const float margin =
                (
                    static_cast<float>(
                        g_CockpitScope.realWidth
                        ) -
                    static_cast<float>(
                        g_CockpitScope.hudWidth
                        )
                    ) *
                0.5f;


            if (g_CockpitScope.hudWidth != 0)
            {
                actualOffset =
                    margin /
                    static_cast<float>(
                        g_CockpitScope.hudWidth
                        );
            }
        }


        rect[0] +=
            actualOffset;

        rect[2] +=
            actualOffset;
    }


    return g_Original_70E30(
        param_1,
        param_2,
        rect,
        param_4,
        param_5
    );
}


bool InstallHUDFix()
{
    MH_STATUS status =
        MH_Initialize();

    if (status != MH_OK &&
        status != MH_ERROR_ALREADY_INITIALIZED)
    {
        return false;
    }


    // ========================================================
    // FUN_0043EB10
    // GMR/UI layout
    // ========================================================

    status =
        MH_CreateHook(
            reinterpret_cast<LPVOID>(
                Addr::FUN_0043EB10
                ),

            reinterpret_cast<LPVOID>(
                Hook_43EB10
                ),

            reinterpret_cast<LPVOID*>(
                &g_Original_43EB10
                )
        );

    if (status != MH_OK)
    {
        return false;
    }


    // ========================================================
    // LAB_004AF060
    // Cockpit render scope
    // ========================================================

    status =
        MH_CreateHook(
            reinterpret_cast<LPVOID>(
                Addr::LAB_004AF060
                ),

            reinterpret_cast<LPVOID>(
                Hook_AF060
                ),

            reinterpret_cast<LPVOID*>(
                &g_Original_AF060
                )
        );

    if (status != MH_OK)
    {
        return false;
    }


    // ========================================================
    // FUN_00471610
    // RacePos / LapPos / LapNumPos
    // ========================================================

    status =
        MH_CreateHook(
            reinterpret_cast<LPVOID>(
                Addr::FUN_00471610
                ),

            reinterpret_cast<LPVOID>(
                Hook_71610
                ),

            reinterpret_cast<LPVOID*>(
                &g_Original_71610
                )
        );

    if (status != MH_OK)
    {
        return false;
    }


    // ========================================================
    // FUN_004B2090
    // Speedometer text
    // ========================================================

    status =
        MH_CreateHook(
            reinterpret_cast<LPVOID>(
                Addr::FUN_004B2090
                ),

            reinterpret_cast<LPVOID>(
                Hook_B2090
                ),

            reinterpret_cast<LPVOID*>(
                &g_Original_B2090
                )
        );

    if (status != MH_OK)
    {
        return false;
    }


    // ========================================================
    // FUN_004B2270
    // Speedometer textures / graphics
    // ========================================================

    status =
        MH_CreateHook(
            reinterpret_cast<LPVOID>(
                Addr::FUN_004B2270
                ),

            reinterpret_cast<LPVOID>(
                Hook_B2270
                ),

            reinterpret_cast<LPVOID*>(
                &g_Original_B2270
                )
        );

    if (status != MH_OK)
    {
        return false;
    }


    // ========================================================
    // FUN_00470E30
    // Centered cockpit elements
    // ========================================================

    status =
        MH_CreateHook(
            reinterpret_cast<LPVOID>(
                Addr::FUN_00470E30
                ),

            reinterpret_cast<LPVOID>(
                Hook_70E30
                ),

            reinterpret_cast<LPVOID*>(
                &g_Original_70E30
                )
        );

    if (status != MH_OK)
    {
        return false;
    }


    // ========================================================
    // ENABLE ALL HOOKS
    // ========================================================

    status =
        MH_EnableHook(
            reinterpret_cast<LPVOID>(
                Addr::FUN_0043EB10
                )
        );

    if (status != MH_OK)
    {
        return false;
    }


    status =
        MH_EnableHook(
            reinterpret_cast<LPVOID>(
                Addr::LAB_004AF060
                )
        );

    if (status != MH_OK)
    {
        return false;
    }


    status =
        MH_EnableHook(
            reinterpret_cast<LPVOID>(
                Addr::FUN_00471610
                )
        );

    if (status != MH_OK)
    {
        return false;
    }


    status =
        MH_EnableHook(
            reinterpret_cast<LPVOID>(
                Addr::FUN_004B2090
                )
        );

    if (status != MH_OK)
    {
        return false;
    }


    status =
        MH_EnableHook(
            reinterpret_cast<LPVOID>(
                Addr::FUN_004B2270
                )
        );

    if (status != MH_OK)
    {
        return false;
    }


    status =
        MH_EnableHook(
            reinterpret_cast<LPVOID>(
                Addr::FUN_00470E30
                )
        );

    if (status != MH_OK)
    {
        return false;
    }


    return true;
}

void RemoveHUDFix()
{
    // ========================================================
    // Disable
    // ========================================================

    MH_DisableHook(
        reinterpret_cast<LPVOID>(
            Addr::FUN_0043EB10
            )
    );

    MH_DisableHook(
        reinterpret_cast<LPVOID>(
            Addr::LAB_004AF060
            )
    );

    MH_DisableHook(
        reinterpret_cast<LPVOID>(
            Addr::FUN_00471610
            )
    );

    MH_DisableHook(
        reinterpret_cast<LPVOID>(
            Addr::FUN_004B2090
            )
    );

    MH_DisableHook(
        reinterpret_cast<LPVOID>(
            Addr::FUN_004B2270
            )
    );

    MH_DisableHook(
        reinterpret_cast<LPVOID>(
            Addr::FUN_00470E30
            )
    );


    // ========================================================
    // Remove
    // ========================================================

    MH_RemoveHook(
        reinterpret_cast<LPVOID>(
            Addr::FUN_0043EB10
            )
    );

    MH_RemoveHook(
        reinterpret_cast<LPVOID>(
            Addr::LAB_004AF060
            )
    );

    MH_RemoveHook(
        reinterpret_cast<LPVOID>(
            Addr::FUN_00471610
            )
    );

    MH_RemoveHook(
        reinterpret_cast<LPVOID>(
            Addr::FUN_004B2090
            )
    );

    MH_RemoveHook(
        reinterpret_cast<LPVOID>(
            Addr::FUN_004B2270
            )
    );

    MH_RemoveHook(
        reinterpret_cast<LPVOID>(
            Addr::FUN_00470E30
            )
    );

        // --------------------------------------------------------
        // Если HUD Fix был единственным пользователем MinHook,
        // MH_Uninitialize() можно вызвать здесь.
        // Но у тебя сейчас FPS-патч использует не MinHook, а
        // прямой CALL patch, поэтому это допустимо.
        // --------------------------------------------------------

        MH_Uninitialize();
}

// ============================================================
// MINHOOK
// ============================================================

static bool CreateHook(
    uintptr_t target,
    LPVOID detour,
    LPVOID* original)
{
    return
        MH_CreateHook(
            reinterpret_cast<LPVOID>(
                target
                ),
            detour,
            original
        ) == MH_OK;
}


static bool EnableHook(
    uintptr_t target)
{
    return
        MH_EnableHook(
            reinterpret_cast<LPVOID>(
                target
                )
        ) == MH_OK;
}


// ============================================================
// INSTALL
// ============================================================

static bool InstallHooks()
{
    MH_STATUS status =
        MH_Initialize();


    if (status != MH_OK &&
        status != MH_ERROR_ALREADY_INITIALIZED)
    {
        return false;
    }


    // --------------------------------------------------------
    // 1. Generic GMR rectangle.
    // --------------------------------------------------------

    if (!CreateHook(
        ADDR_FUN_0043EB10,
        reinterpret_cast<LPVOID>(
            Hook_43EB10
            ),
        reinterpret_cast<LPVOID*>(
            &g_Original_43EB10
            )
    ))
    {
        return false;
    }


    // --------------------------------------------------------
    // 2. Cockpit scope.
    // --------------------------------------------------------

    if (!CreateHook(
        ADDR_LAB_004AF060,
        reinterpret_cast<LPVOID>(
            Hook_AF060
            ),
        reinterpret_cast<LPVOID*>(
            &g_Original_AF060
            )
    ))
    {
        return false;
    }


    // --------------------------------------------------------
    // 3. RacePos / LapPos / LapNumPos.
    // --------------------------------------------------------

    if (!CreateHook(
        ADDR_FUN_00471610,
        reinterpret_cast<LPVOID>(
            Hook_71610
            ),
        reinterpret_cast<LPVOID*>(
            &g_Original_71610
            )
    ))
    {
        return false;
    }


    // --------------------------------------------------------
    // 4. Speedometer text.
    // --------------------------------------------------------

    if (!CreateHook(
        ADDR_FUN_004B2090,
        reinterpret_cast<LPVOID>(
            Hook_B2090
            ),
        reinterpret_cast<LPVOID*>(
            &g_Original_B2090
            )
    ))
    {
        return false;
    }


    // --------------------------------------------------------
    // 5. Speedometer graphics.
    // --------------------------------------------------------

    if (!CreateHook(ADDR_FUN_004B2270,
        reinterpret_cast<LPVOID>(Hook_B2270),
        reinterpret_cast<LPVOID*>(&g_Original_B2270)))
    {
        return false;
    }


    // --------------------------------------------------------
    // 6. Centered cockpit text rectangles.
    // --------------------------------------------------------

    if (!CreateHook(ADDR_FUN_00470E30,
        reinterpret_cast<LPVOID>(Hook_70E30),
        reinterpret_cast<LPVOID*>(&g_Original_70E30)))
    {
        return false;
    }

    // --------------------------------------------------------
    // Enable.
    // --------------------------------------------------------

    if (!EnableHook(ADDR_FUN_0043EB10))
    {
        return false;
    }

    if (!EnableHook(ADDR_LAB_004AF060))
    {
        return false;
    }

    if (!EnableHook(ADDR_FUN_00471610))
    {
        return false;
    }

    if (!EnableHook(ADDR_FUN_004B2090))
    {
        return false;
    }

    if (!EnableHook(ADDR_FUN_004B2270))
    {
        return false;
    }

    if (!EnableHook(ADDR_FUN_00470E30))
    {
        return false;
    }

    if (!EnableHook(ADDR_LAB_004CBD20))
    {
        return false;
    }

    return true;
}
