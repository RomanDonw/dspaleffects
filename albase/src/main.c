/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "albase.h"
#define EFX_IMPL
#include "EFX.h"

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#include <alc.h>
#include <alext.h>

// ===============================

static ALCdevice *aldev;
static ALCcontext *alctx;

static LPALCRENDERSAMPLESSOFT alcRenderSamplesSOFT;
static LPALCRESETDEVICESOFT alcResetDeviceSOFT;

static bool inited = false;
static unsigned long currrate;
static bool enableerrorlog;

#define ERRORLOG(str) { if (enableerrorlog) fputs((str), stderr); }

char albase_init(unsigned long initrate, bool errorlog)
{
    if (inited) return 1;
    enableerrorlog = errorlog;

    if (!alcIsExtensionPresent(NULL, "ALC_SOFT_loopback"))
    { ERRORLOG("required \"ALC_SOFT_loopback\" OpenAL extension doesn't supported on this platform"); return 1; }
    LPALCLOOPBACKOPENDEVICESOFT alcLoopbackOpenDeviceSOFT = alcGetProcAddress(NULL, "alcLoopbackOpenDeviceSOFT");
    if (!alcLoopbackOpenDeviceSOFT) { ERRORLOG("failed loading alcLoopbackOpenDeviceSOFT OpenAL function"); return 1; }
    if (!(aldev = alcLoopbackOpenDeviceSOFT(NULL))) { ERRORLOG("error creating/opening OpenAL loopback device"); return 1; }
    
    // ===============================

    if (!alcIsExtensionPresent(aldev, "ALC_SOFT_output_limiter"))
    { ERRORLOG("required \"ALC_SOFT_output_limiter\" OpenAL extension doesn't supported by loopback device on this platform"); return 1; }
    if (!alcIsExtensionPresent(aldev, "ALC_EXT_EFX"))
    { ERRORLOG("required \"ALC_EXT_EFX\" OpenAL extension (OpenAL EFX) doesn't supported by loopback device on this platform"); return 1; }
    if (!alcIsExtensionPresent(aldev, "ALC_SOFT_HRTF"))
    { ERRORLOG("required \"ALC_SOFT_HRTF\" OpenAL extension doesn't supported by loopback device on this platform"); return 1; }

    if (!(alcRenderSamplesSOFT = alcGetProcAddress(aldev, "alcRenderSamplesSOFT")))
    { ERRORLOG("failed loading alcRenderSamplesSOFT OpenAL function"); return 1; }
    if (!(alcResetDeviceSOFT = alcGetProcAddress(aldev, "alcResetDeviceSOFT")))
    { ERRORLOG("failed loading alcResetDeviceSOFT OpenAL function"); return 1; }
    
    {
        const ALint attrs[] =
        {
            ALC_FORMAT_CHANNELS_SOFT, ALC_STEREO_SOFT,
            ALC_FORMAT_TYPE_SOFT, ALC_FLOAT_SOFT,
            ALC_FREQUENCY, initrate,
            ALC_OUTPUT_LIMITER_SOFT, AL_FALSE,
            0
        };
        if (!(alctx = alcCreateContext(aldev, attrs))) { ERRORLOG("error creating OpenAL context for loopback device"); return 1; }
        currrate = initrate;
    }
    alcMakeContextCurrent(alctx);

    // ===============================

    if (!(alGenAuxiliaryEffectSlots = alGetProcAddress("alGenAuxiliaryEffectSlots")))
    { ERRORLOG("failed loading alGenAuxiliaryEffectSlots OpenAL function"); return 1; } 
    if (!(alAuxiliaryEffectSloti = alGetProcAddress("alAuxiliaryEffectSloti")))
    { ERRORLOG("failed loading alAuxiliaryEffectSloti OpenAL function"); return 1; }

    if (!(alGenEffects = alGetProcAddress("alGenEffects"))) { ERRORLOG("failed loading alGenEffects OpenAL function"); return 1; }
    if (!(alDeleteEffects = alGetProcAddress("alDeleteEffects"))) { ERRORLOG("failed loading alDeleteEffects OpenAL function"); return 1; }
    if (!(alEffecti = alGetProcAddress("alEffecti"))) { ERRORLOG("failed loading alEffecti OpenAL function"); return 1; }
    if (!(alEffectf = alGetProcAddress("alEffectf"))) { ERRORLOG("failed loading alEffectf OpenAL function"); return 1; }
    if (!(alEffectfv = alGetProcAddress("alEffectfv"))) { ERRORLOG("failed loading alEffectfv OpenAL function"); return 1; }
    
    if (!(alGenFilters = alGetProcAddress("alGenFilters"))) { ERRORLOG("failed loading alGenFilters OpenAL function"); return 1; }
    if (!(alDeleteFilters = alGetProcAddress("alDeleteFilters"))) { ERRORLOG("failed loading alDeleteFilters OpenAL function"); return 1; }
    if (!(alFilteri = alGetProcAddress("alFilteri"))) { ERRORLOG("failed loading alFilteri OpenAL function"); return 1; }
    if (!(alFilterf = alGetProcAddress("alFilterf"))) { ERRORLOG("failed loading alFilterf OpenAL function"); return 1; }

    inited = true;
    return 0;
}

char albase_render(float interleaved[], unsigned long duration, unsigned long rate)
{
    if (!(inited && duration)) return 1;
    
    if (currrate != rate)
    {
        const ALint attrs[] =
        {
            ALC_FORMAT_CHANNELS_SOFT, ALC_STEREO_SOFT,
            ALC_FORMAT_TYPE_SOFT, ALC_FLOAT_SOFT,
            ALC_FREQUENCY, rate,
            ALC_OUTPUT_LIMITER_SOFT, AL_FALSE,
            0
        };
        if (!alcResetDeviceSOFT(aldev, attrs)) { ERRORLOG("failed to change sample rate of loopback device"); return 1; }
        currrate = rate;
    }
    
    alcRenderSamplesSOFT(aldev, interleaved, duration);
    return 0;
}

char albase_quit(void)
{
    if (!inited) return 1;

    alcMakeContextCurrent(NULL);
    alcDestroyContext(alctx);
    alcCloseDevice(aldev);

    inited = false;
    return 0;
}