/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "general.h"
#define ALUTIL_EFX_IMPL
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

char alutil_init(unsigned long initrate, bool errorlog, bool limitoutput)
{
    if (inited) return 1;
    enableerrorlog = errorlog;

    // ===============================

    if (!alcIsExtensionPresent(NULL, "ALC_SOFT_loopback"))
    { ERRORLOG("required \"ALC_SOFT_loopback\" OpenAL extension doesn't supported on this platform"); return 1; }
    LPALCLOOPBACKOPENDEVICESOFT alcLoopbackOpenDeviceSOFT = alcGetProcAddress(NULL, "alcLoopbackOpenDeviceSOFT");
    if (!alcLoopbackOpenDeviceSOFT) { ERRORLOG("failed loading alcLoopbackOpenDeviceSOFT OpenAL function"); return 1; }
    if (!(aldev = alcLoopbackOpenDeviceSOFT(NULL))) { ERRORLOG("error creating/opening OpenAL loopback device"); return 1; }
    
    // ===============================

    if (!alcIsExtensionPresent(aldev, "ALC_SOFT_output_limiter"))
    { ERRORLOG("required \"ALC_SOFT_output_limiter\" OpenAL extension doesn't supported by loopback device on this platform"); return 1; }
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
            ALC_OUTPUT_LIMITER_SOFT, limitoutput ? AL_TRUE : AL_FALSE,
            0
        };
        if (!(alctx = alcCreateContext(aldev, attrs))) { ERRORLOG("error creating OpenAL context for loopback device"); return 1; }
        currrate = initrate;
    }
    if (!alcMakeContextCurrent(alctx)) { ERRORLOG("unable to set new OpenAL context as current"); return 1; }

    // ===============================

    inited = true;
    return 0;
}

char alutil_render(float interleaved[], unsigned long duration, unsigned long rate)
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
    if (alcGetError(aldev) != ALC_NO_ERROR) { ERRORLOG("failed rendering sampels into buffer"); return 1; }

    return 0;
}

char alutil_quit(void)
{
    if (!inited) return 1;

    alcMakeContextCurrent(NULL);
    alcDestroyContext(alctx);
    alcCloseDevice(aldev);

    inited = false;
    return 0;
}

char alutil_loadEFX(void)
{
    if (!inited) return 1;

    if (!alcIsExtensionPresent(aldev, "ALC_EXT_EFX"))
    { ERRORLOG("required \"ALC_EXT_EFX\" OpenAL extension (OpenAL EFX) doesn't supported by loopback device on this platform"); return 1; }

    if (!(alGenAuxiliaryEffectSlots = alGetProcAddress("alGenAuxiliaryEffectSlots"))) { ERRORLOG("failed loading alGenAuxiliaryEffectSlots OpenAL function"); return 1; }
    if (!(alDeleteAuxiliaryEffectSlots = alGetProcAddress("alDeleteAuxiliaryEffectSlots"))) { ERRORLOG("failed loading alDeleteAuxiliaryEffectSlots OpenAL function"); return 1; }
    if (!(alIsAuxiliaryEffectSlot = alGetProcAddress("alIsAuxiliaryEffectSlot"))) { ERRORLOG("failed loading alIsAuxiliaryEffectSlot OpenAL function"); return 1; }
    if (!(alAuxiliaryEffectSloti = alGetProcAddress("alAuxiliaryEffectSloti"))) { ERRORLOG("failed loading alAuxiliaryEffectSloti OpenAL function"); return 1; }
    if (!(alAuxiliaryEffectSlotiv = alGetProcAddress("alAuxiliaryEffectSlotiv"))) { ERRORLOG("failed loading alAuxiliaryEffectSlotiv OpenAL function"); return 1; }
    if (!(alAuxiliaryEffectSlotf = alGetProcAddress("alAuxiliaryEffectSlotf"))) { ERRORLOG("failed loading alAuxiliaryEffectSlotf OpenAL function"); return 1; }
    if (!(alAuxiliaryEffectSlotfv = alGetProcAddress("alAuxiliaryEffectSlotfv"))) { ERRORLOG("failed loading alAuxiliaryEffectSlotfv OpenAL function"); return 1; }
    if (!(alGetAuxiliaryEffectSloti = alGetProcAddress("alGetAuxiliaryEffectSloti"))) { ERRORLOG("failed loading alGetAuxiliaryEffectSloti OpenAL function"); return 1; }
    if (!(alGetAuxiliaryEffectSlotiv = alGetProcAddress("alGetAuxiliaryEffectSlotiv"))) { ERRORLOG("failed loading alGetAuxiliaryEffectSlotiv OpenAL function"); return 1; }
    if (!(alGetAuxiliaryEffectSlotf = alGetProcAddress("alGetAuxiliaryEffectSlotf"))) { ERRORLOG("failed loading alGetAuxiliaryEffectSlotf OpenAL function"); return 1; }
    if (!(alGetAuxiliaryEffectSlotfv = alGetProcAddress("alGetAuxiliaryEffectSlotfv"))) { ERRORLOG("failed loading alGetAuxiliaryEffectSlotfv OpenAL function"); return 1; }

    if (!(alGenEffects = alGetProcAddress("alGenEffects"))) { ERRORLOG("failed loading alGenEffects OpenAL function"); return 1; }
    if (!(alDeleteEffects = alGetProcAddress("alDeleteEffects"))) { ERRORLOG("failed loading alDeleteEffects OpenAL function"); return 1; }
    if (!(alIsEffect = alGetProcAddress("alIsEffect"))) { ERRORLOG("failed loading alIsEffect OpenAL function"); return 1; }
    if (!(alEffecti = alGetProcAddress("alEffecti"))) { ERRORLOG("failed loading alEffecti OpenAL function"); return 1; }
    if (!(alEffectiv = alGetProcAddress("alEffectiv"))) { ERRORLOG("failed loading alEffectiv OpenAL function"); return 1; }
    if (!(alEffectf = alGetProcAddress("alEffectf"))) { ERRORLOG("failed loading alEffectf OpenAL function"); return 1; }
    if (!(alEffectfv = alGetProcAddress("alEffectfv"))) { ERRORLOG("failed loading alEffectfv OpenAL function"); return 1; }
    if (!(alGetEffecti = alGetProcAddress("alGetEffecti"))) { ERRORLOG("failed loading alGetEffecti OpenAL function"); return 1; }
    if (!(alGetEffectiv = alGetProcAddress("alGetEffectiv"))) { ERRORLOG("failed loading alGetEffectiv OpenAL function"); return 1; }
    if (!(alGetEffectf = alGetProcAddress("alGetEffectf"))) { ERRORLOG("failed loading alGetEffectf OpenAL function"); return 1; }
    if (!(alGetEffectfv = alGetProcAddress("alGetEffectfv"))) { ERRORLOG("failed loading alGetEffectfv OpenAL function"); return 1; }
    
    if (!(alGenFilters = alGetProcAddress("alGenFilters"))) { ERRORLOG("failed loading alGenFilters OpenAL function"); return 1; }
    if (!(alDeleteFilters = alGetProcAddress("alDeleteFilters"))) { ERRORLOG("failed loading alDeleteFilters OpenAL function"); return 1; }
    if (!(alIsFilter = alGetProcAddress("alIsFilter"))) { ERRORLOG("failed loading alIsFilter OpenAL function"); return 1; }
    if (!(alFilteri = alGetProcAddress("alFilteri"))) { ERRORLOG("failed loading alFilteri OpenAL function"); return 1; }
    if (!(alFilteriv = alGetProcAddress("alFilteriv"))) { ERRORLOG("failed loading alFilteriv OpenAL function"); return 1; }
    if (!(alFilterf = alGetProcAddress("alFilterf"))) { ERRORLOG("failed loading alFilterf OpenAL function"); return 1; }
    if (!(alFilterfv = alGetProcAddress("alFilterfv"))) { ERRORLOG("failed loading alFilterfv OpenAL function"); return 1; }
    if (!(alGetFilteri = alGetProcAddress("alGetFilteri"))) { ERRORLOG("failed loading alGetFilteri OpenAL function"); return 1; }
    if (!(alGetFilteriv = alGetProcAddress("alGetFilteriv"))) { ERRORLOG("failed loading alGetFilteriv OpenAL function"); return 1; }
    if (!(alGetFilterf = alGetProcAddress("alGetFilterf"))) { ERRORLOG("failed loading alGetFilterf OpenAL function"); return 1; }
    if (!(alGetFilterfv = alGetProcAddress("alGetFilterfv"))) { ERRORLOG("failed loading alGetFilterfv OpenAL function"); return 1; }

    return 0;
}