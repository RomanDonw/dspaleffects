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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <alc.h>
#include <alext.h>

// ===============================

static ALCint alctx_attrs[] =
{
    ALC_FORMAT_CHANNELS_SOFT, ALC_STEREO_SOFT,
    ALC_FORMAT_TYPE_SOFT, ALC_FLOAT_SOFT,
    ALC_FREQUENCY, 0,
    ALC_OUTPUT_LIMITER_SOFT, AL_FALSE,
    0
};

static void *tmpbuffdata = NULL;
static size_t tmpbuffsize = 0;

// ===============================

static ALCdevice *aldev;
static ALCcontext *alctx;

static LPALCRENDERSAMPLESSOFT alcRenderSamplesSOFT;
static LPALCRESETDEVICESOFT alcResetDeviceSOFT;

// ===============================

static bool inited = false;

char albase_init(unsigned long firstrate)
{
    if (inited) return 1;

    if (!alcIsExtensionPresent(NULL, "ALC_SOFT_loopback"))
    { puts("required \"ALC_SOFT_loopback\" OpenAL extension doesn't supported on this platform"); return 1; }
    LPALCLOOPBACKOPENDEVICESOFT alcLoopbackOpenDeviceSOFT = alcGetProcAddress(NULL, "alcLoopbackOpenDeviceSOFT");
    if (!alcLoopbackOpenDeviceSOFT) { puts("failed to dynamicly load alcLoopbackOpenDeviceSOFT OpenAL function"); return 1; }
    if (!(aldev = alcLoopbackOpenDeviceSOFT(NULL))) { puts("error creating/opening OpenAL loopback device"); return 1; }
    
    // ===============================

    if (!alcIsExtensionPresent(aldev, "ALC_SOFT_output_limiter"))
    { puts("required \"ALC_SOFT_output_limiter\" OpenAL extension doesn't supported by loopback device on this platform"); return 1; }
    if (!alcIsExtensionPresent(aldev, "ALC_EXT_EFX"))
    { puts("required \"ALC_EXT_EFX\" OpenAL extension (OpenAL EFX) doesn't supported by loopback device on this platform"); return 1; }
    if (!(alcRenderSamplesSOFT = alcGetProcAddress(aldev, "alcRenderSamplesSOFT")))
    { puts("required alcRenderSamplesSOFT OpenAL function doesn't supported by loopback device on this platform"); return 1; }
    if (!(alcResetDeviceSOFT = alcGetProcAddress(aldev, "alcResetDeviceSOFT")))
    { puts("required alcResetDeviceSOFT OpenAL function doesn't supported by loopback device on this platform"); return 1; }
    
    alctx_attrs[5] = firstrate;
    if (!(alctx = alcCreateContext(aldev, alctx_attrs))) { puts("error creating OpenAL context for loopback device"); return 1; }
    alcMakeContextCurrent(alctx);

    // ===============================

    if (!(alGenAuxiliaryEffectSlots = alGetProcAddress("alGenAuxiliaryEffectSlots")))
    { puts("failed to load alGenAuxiliaryEffectSlots OpenAL function"); return 1; } 
    if (!(alAuxiliaryEffectSloti = alGetProcAddress("alAuxiliaryEffectSloti")))
    { puts("failed to load alAuxiliaryEffectSloti OpenAL function"); return 1; }

    if (!(alGenEffects = alGetProcAddress("alGenEffects"))) { puts("failed to load alGenEffects OpenAL function"); return 1; }
    if (!(alDeleteEffects = alGetProcAddress("alDeleteEffects"))) { puts("failed to load alDeleteEffects OpenAL function"); return 1; }
    if (!(alEffecti = alGetProcAddress("alEffecti"))) { puts("failed to load alEffecti OpenAL function"); return 1; }
    if (!(alEffectf = alGetProcAddress("alEffectf"))) { puts("failed to load alEffectf OpenAL function"); return 1; }
    if (!(alEffectfv = alGetProcAddress("alEffectfv"))) { puts("failed to load alEffectfv OpenAL function"); return 1; }
    
    if (!(alGenFilters = alGetProcAddress("alGenFilters"))) { puts("failed to load alGenFilters OpenAL function"); return 1; }
    if (!(alDeleteFilters = alGetProcAddress("alDeleteFilters"))) { puts("failed to load alDeleteFilters OpenAL function"); return 1; }
    if (!(alFilteri = alGetProcAddress("alFilteri"))) { puts("failed to load alFilteri OpenAL function"); return 1; }
    if (!(alFilterf = alGetProcAddress("alFilterf"))) { puts("failed to load alFilterf OpenAL function"); return 1; }

    inited = true;
    return 0;
}

char albase_render(float left[], float right[], unsigned long duration, unsigned long rate)
{
    if (!inited) return 1;
    
    if (alctx_attrs[5] != rate)
    {
        alctx_attrs[5] = rate;
        if (!alcResetDeviceSOFT(aldev, alctx_attrs)) return 1;
    }
    
    if (tmpbuffsize != sizeof(float) * duration * 2)
    {
        void *new_tmpbuffdata = realloc(tmpbuffdata, sizeof(float) * duration * 2);
        if (!new_tmpbuffdata) return 1;
        tmpbuffdata = new_tmpbuffdata;
        tmpbuffsize = sizeof(float) * duration * 2;
    }

    alcRenderSamplesSOFT(aldev, tmpbuffdata, duration);
    
    for (size_t i = 0; i < (size_t)duration << 1; i++)
    {
        if (i & 1 && right) right[i >> 1] = ((float *)tmpbuffdata)[i];
        else if (left) left[i >> 1] = ((float *)tmpbuffdata)[i];
    }

    return 0;
}

char albase_quit(void)
{
    if (!inited) return 1;

    alcMakeContextCurrent(NULL);
    alcDestroyContext(alctx);
    alcCloseDevice(aldev);

    free(tmpbuffdata);
    tmpbuffdata = NULL;
    tmpbuffsize = 0;

    inited = false;
    return 0;
}