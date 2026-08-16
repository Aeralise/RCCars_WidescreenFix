#pragma once

#include <cstdint>

namespace Addr
{   
    // GAME RESOLUTION    
    constexpr uintptr_t XResolution1 =  0x004A6A0D;
    constexpr uintptr_t XResolution2 =  0x004A69D3;
    constexpr uintptr_t YResolution1 =  0x004A6A08;
    constexpr uintptr_t YResolution2 =  0x004A69CE;
    
    // CORE ASPECT RATIO   
    constexpr uintptr_t CameraAspect =  0x00405D65;
    constexpr uintptr_t CarPreview_0 =  0x004DD915;
    constexpr uintptr_t CarPreview_1 =  0x004DD9B6;
    
    // FOV   
    constexpr uintptr_t FOV =   0x00405D6F;

    // GAME WIDTH / HEIGHT STATE
    constexpr uintptr_t ScreenWidth =   0x005647F0;
    constexpr uintptr_t ScreenHeight =  0x005647F4;
   
    // FPS / ELAPSED TIME   
    constexpr uintptr_t GMPSetElapsedTime = 0x004407A0;
    constexpr uintptr_t GMPSetElapsedTimeCall = 0x0048BD41;
    constexpr uintptr_t LegacyTargetFPS =   0x0148FAD0;
    
    // HUD FIX   
    constexpr uintptr_t FUN_0043EB10 =  0x0043EB10;
    constexpr uintptr_t LAB_004AF060 =  0x004AF060;
    constexpr uintptr_t FUN_00471610 =  0x00471610;
    constexpr uintptr_t FUN_004B2090 =  0x004B2090;
    constexpr uintptr_t FUN_004B2270 =  0x004B2270;
    constexpr uintptr_t FUN_00470E30 =  0x00470E30;
   
    // OTHER OLD HUD PATCHES
    //
    // These are kept because they are already part of your
    // existing widescreen fix.
    
    constexpr uintptr_t RacePos =   0x0056D0C2;
    constexpr uintptr_t LapLabel =  0x0056D0DA;
    constexpr uintptr_t SplitUzel = 0x0056D04A;
    constexpr uintptr_t CockpitL1 = 0x0056D6E2;
    constexpr uintptr_t CockpitL2 = 0x0056D6DB;
    constexpr uintptr_t CockpitR1 = 0x0056D6EA;
    constexpr uintptr_t CockpitR2 =        0x0056D6F2;
    constexpr uintptr_t OvertakingR =        0x0056D78A;
    constexpr uintptr_t OvertakingL =        0x0056D782;
    constexpr uintptr_t Enum =        0x0056D3F2;
    constexpr uintptr_t Map1 =        0x0056ECA2;
    constexpr uintptr_t Map2 =        0x0056ECB2;
    constexpr uintptr_t MsgPause =        0x0056D282;
    constexpr uintptr_t MsgLow =        0x0056D27A;
    constexpr uintptr_t MsgWrong =        0x0056D28A;
    constexpr uintptr_t MsgHit =        0x0056D292;
    constexpr uintptr_t MsgSmashHit =        0x0056D29A;
    constexpr uintptr_t Msg3 =        0x0056D2A2;
    constexpr uintptr_t Msg2 =        0x0056D2AA;
    constexpr uintptr_t Msg1 =        0x0056D2B2;
    constexpr uintptr_t MsgFinish =        0x0056D2BA;
    constexpr uintptr_t MsgStart =        0x0056D2C2;
    constexpr uintptr_t MsgBestLap =        0x0056D2CA;
    constexpr uintptr_t MMLogo =        0x0056ECBA;
    constexpr uintptr_t MMLogo2 =        0x0056E982;
    constexpr uintptr_t LogoCR =        0x0056B8EE;
    constexpr uintptr_t Logo1C =        0x0056B8DE;
    constexpr uintptr_t MapPosX1 =        0x0056EC9A;
    constexpr uintptr_t MapPosX2 =        0x0056ECAA;
    constexpr uintptr_t LogoRC1 =        0x0056B8BE;
    constexpr uintptr_t LogoRC2 =        0x0056B8C6;
    
    // LATE HUD PATCHES    
    constexpr uintptr_t LateLapPos1 = 0x0149DCEA;
    constexpr uintptr_t LateLapPos2 = 0x0149DCFA;
    constexpr uintptr_t LateLapPos3 = 0x0149DCF2;
    constexpr uintptr_t LateLapNumPos1 = 0x0149DD02;
    constexpr uintptr_t LateLapNumPos2 = 0x0149DD0A;
    constexpr uintptr_t LateLapNumPos3 = 0x0149DD12;
    constexpr uintptr_t LateOvertakingBRect = 0x0149E1FA;
    
    // OLD FUNCTION DISABLE PATCHES    
    constexpr uintptr_t DisablePatch1 = 0x004CA835;
    constexpr uintptr_t DisablePatch2 = 0x004CA8B1;
   
    // DEBUG   
    constexpr uintptr_t NoMinimizePatch = 0x00441177;
    
    // ARROW
    // Still retained as old patch address, but our new HUD
    // hooks do not modify the arrow yet.   
    constexpr uintptr_t ArrowSize = 0x005547FA;
}