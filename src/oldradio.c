/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include <dspmodule.h>

#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include "alutil/general.h"
#include "alutil/EFX.h"

const unsigned short dspmodule_requiredAPIversion = 1;

static float origgain = 1, effectgain = 1, inampmod = 0, involmod = 1, outampmod = 0, outvolmod = 1;
static void *inleftport, *inrightport, *outleftport, *outrightport;
static ALuint source, buffer;
static bool allowidlerenders = false;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{   
    if (alutil_init(48000, true, false)) return 1;
    if (!alIsExtensionPresent("AL_SOFT_effect_target"))
    { puts("required \"AL_SOFT_effect_target\" OpenAL extension doesn't supported on this platform"); return 1; }
    if (alutil_loadEFX()) return 1;

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
    
    alGenBuffers(1, &buffer);
    alGenSources(1, &source);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(source, AL_ROLLOFF_FACTOR, 0);

    ALuint filter;
    alGenFilters(1, &filter);

    alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    alFilterf(filter, AL_LOWPASS_MAX_GAINHF, 1);
    alFilterf(filter, AL_LOWPASS_GAIN, 0);

    alSourcei(source, AL_DIRECT_FILTER, filter);
    alDeleteFilters(1, &filter);

    // ===============================
    
    ALuint slots[2], effect;
    alGenAuxiliaryEffectSlots(2, slots);
    alGenEffects(1, &effect);

    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_DISTORTION);
    alEffectf(effect, AL_DISTORTION_EDGE, 0.2);
    alEffectf(effect, AL_DISTORTION_GAIN, 1);
    alAuxiliaryEffectSloti(slots[0], AL_EFFECTSLOT_EFFECT, effect);
    alAuxiliaryEffectSloti(slots[0], AL_EFFECTSLOT_TARGET_SOFT, slots[1]);

    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EQUALIZER);
    alEffectf(effect, AL_EQUALIZER_LOW_CUTOFF, 300);
    alEffectf(effect, AL_EQUALIZER_LOW_GAIN, 0);
    alEffectf(effect, AL_EQUALIZER_HIGH_CUTOFF, 3000);
    alEffectf(effect, AL_EQUALIZER_HIGH_GAIN, 0);
    alAuxiliaryEffectSloti(slots[1], AL_EFFECTSLOT_EFFECT, effect);
    alDeleteEffects(1, &effect);

    alSource3i(source, AL_AUXILIARY_SEND_FILTER, slots[0], 0, AL_FILTER_NULL);
    
    // ===============================

    printf("inampmod: %f\ninvolmod: %f\noriggain: %f\neffectgain: %f\noutampmod: %f\noutvolmod: %f\nidle renders: %s\n",
        inampmod, involmod, origgain, effectgain, outampmod, outvolmod, allowidlerenders ? "allowed" : "not allowed");

    *sysname = "oldradio";
    *dispname = "OpenAL Old Radio";
    return 0;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long duration, unsigned long rate, unsigned long long nsectime)
{
    float *outleft = lapi->getportbuffer(outleftport, duration);
    float *outright = lapi->getportbuffer(outrightport, duration);
    if (!(allowidlerenders || outleft || outright)) return 0;
    
    const float *inleft = lapi->getportbuffer(inleftport, duration);
    const float *inright = lapi->getportbuffer(inrightport, duration);

    // ===============================
    
    alSourceRewind(source);
    alSourcei(source, AL_BUFFER, 0);

    size_t buffsize = (duration + 1) * sizeof(float) * 2;
    float buff[buffsize];
    for (size_t i = 0; i < ((size_t)duration) << 1; i++)
    { buff[i] = i & 1 ? (inright ? inright[i >> 1] : 0) : (inleft ? inleft[i >> 1] : 0); }
    alBufferData(buffer, AL_FORMAT_STEREO_FLOAT32, buff, buffsize, rate);

    alSourcei(source, AL_BUFFER, buffer);
    alSourcePlay(source);

    // ===============================

    if (alutil_render(buff, duration, rate)) return 1;
    if (outleft || outright) for (unsigned long i = 0; i < duration; i++)
    {
        if (outleft) outleft[i] = buff[i * 2];
        if (outright) outright[i] = buff[i * 2 + 1];
    }

    return 0;
}