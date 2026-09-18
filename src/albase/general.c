/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "general.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alc.h>
#include <alext.h>

#define EFX_IMPL
#include "EFX.h"

// ===============================

float albase_inampmod = 0, albase_involmod = 1, albase_outampmod = 0, albase_outvolmod = 1;
ALuint albase_source;

// ===============================

static ALCdevice *aldev;
static ALCcontext *alctx;
static ALCint alctx_attrs[] =
{
    ALC_FORMAT_CHANNELS_SOFT, ALC_STEREO_SOFT,
    ALC_FORMAT_TYPE_SOFT, ALC_FLOAT_SOFT,
    ALC_FREQUENCY, 48000,
    ALC_OUTPUT_LIMITER_SOFT, AL_FALSE,
    0
};
#define BUFFERSCOUNT 2
static ALuint buffers[BUFFERSCOUNT];

static LPALCRENDERSAMPLESSOFT alcRenderSamplesSOFT;
static LPALCRESETDEVICESOFT alcResetDeviceSOFT;

static void *inleftport, *inrightport, *outleftport, *outrightport;

// ===============================

char albase_init(const DSPLoaderAPI *lapi)
{
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

    // ===============================
    
    if (!(inleftport = lapi->addport("input_left", NULL, DSPPortDirection_Input, 0)))
    { puts("error adding port for input left channel"); return 1; }
    if (!(inrightport = lapi->addport("input_right", NULL, DSPPortDirection_Input, 0)))
    { puts("error adding port for input right channel"); return 1; }
    
    if (!(outleftport = lapi->addport("output_left", NULL, DSPPortDirection_Output, 0)))
    { puts("error adding port for output left channel"); return 1; }
    if (!(outrightport = lapi->addport("output_right", NULL, DSPPortDirection_Output, 0)))
    { puts("error adding port for output right channel"); return 1; }

    // ===============================

    alGenBuffers(BUFFERSCOUNT, buffers);

    alGenSources(1, &albase_source);
    alSourcei(albase_source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(albase_source, AL_ROLLOFF_FACTOR, 0);

    return 0;
}

char albase_process(const DSPLoaderAPI *lapi, unsigned long duration, unsigned long rate)
{
    float *outleft = lapi->getportbuffer(outleftport, duration);
    float *outright = lapi->getportbuffer(outrightport, duration);

    const float *inleft = lapi->getportbuffer(inleftport, duration);
    const float *inright = lapi->getportbuffer(inrightport, duration);

    if (alctx_attrs[5] != rate)
    {
        if (rate > INT32_MAX) { printf("new sample rate is too large (new: %lu, max.: 2 ^ 31 - 1)\n", rate); return 1; }

        alSourceStop(albase_source);
        {
            ALint queuedbuffs;
            alGetSourcei(albase_source, AL_BUFFERS_QUEUED, &queuedbuffs);
            ALuint buff;
            while (queuedbuffs-- > 0) alSourceUnqueueBuffers(albase_source, 1, &buff);
        }
        alctx_attrs[5] = rate;
        if (!alcResetDeviceSOFT(aldev, alctx_attrs)) { puts("failed changing OpenAL loopback device sample rate"); return 1; }
        
        if (outleft) inleft ? memcpy(outleft, inleft, duration * sizeof(float)) : memset(outleft, 0, duration * sizeof(float));
        if (outright) inright ? memcpy(outright, inright, duration * sizeof(float)) : memset(outright, 0, duration * sizeof(float));
        return 0;
    }

    static float *intlvaudio = NULL;
    static size_t intlvaudiosize = 0;
    if (intlvaudiosize != duration * sizeof(float) * 2)
    {
        void *new_intlvaudio = realloc(intlvaudio, duration * sizeof(float) * 2);
        if (!new_intlvaudio) { puts("memory allocation failed"); return 1; }
        intlvaudio = new_intlvaudio;
        intlvaudiosize = duration * sizeof(float) * 2;
    }

    for (size_t i = 0; i < (size_t)duration << 1; i++)
        intlvaudio[i] = adjf(i & 1 ? (inright ? inright[i >> 1] : 0) : (inleft ? inleft[i >> 1] : 0), albase_inampmod) * albase_involmod;
    
    ALint procbuffs, queuedbuffs;
    alGetSourcei(albase_source, AL_BUFFERS_PROCESSED, &procbuffs);
    alGetSourcei(albase_source, AL_BUFFERS_QUEUED, &queuedbuffs);

    ALuint emptybuff;
    while (procbuffs-- > 0)
    {
        alSourceUnqueueBuffers(albase_source, 1, &emptybuff);
        alBufferData(emptybuff, AL_FORMAT_STEREO_FLOAT32, intlvaudio, intlvaudiosize, rate);
        alSourceQueueBuffers(albase_source, 1, &emptybuff);
    }
    while (queuedbuffs < BUFFERSCOUNT)
    {
        emptybuff = buffers[queuedbuffs++];
        alBufferData(emptybuff, AL_FORMAT_STEREO_FLOAT32, intlvaudio, intlvaudiosize, rate);
        alSourceQueueBuffers(albase_source, 1, &emptybuff);
    }

    ALint state;
    alGetSourcei(albase_source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) alSourcePlay(albase_source);

    alcRenderSamplesSOFT(aldev, intlvaudio, duration);
    
    for (size_t i = 0; i < (size_t)duration << 1; i++)
    {
        if (i & 1 && outright) outright[i >> 1] = adjf(intlvaudio[i], albase_outampmod) * albase_outvolmod;
        else if (outleft) outleft[i >> 1] = adjf(intlvaudio[i], albase_outampmod) * albase_outvolmod;
    }

    return 0;
}

void albase_quit(void)
{
    alcMakeContextCurrent(NULL);
    alcDestroyContext(alctx);
    alcCloseDevice(aldev);
}