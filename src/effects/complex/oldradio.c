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
#include <math.h>

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include "alutil/general.h"
#include "alutil/EFX.h"

const unsigned short dspmodule_requiredAPIversion = 4;

static float origgain = 1, effectgain = 1, inampmod = 0, involmod = 1, outampmod = 0, outvolmod = 1, noisevolume = 0.1;
static void *inleftport, *inrightport, *outport;
static ALuint source, buffer;
static bool allowidlerenders = false, noisesquelch = true;

DSPModuleReturnCode dspmodule_startup(const DSPLoaderAPI * restrict lapi, DSPModuleStartupFunctionOptions * restrict opts)
{
    {
        int p;
        static const struct option longopts[] =
        {
            { .name = "noiseVolume", .has_arg = required_argument, .val = 256, .flag = NULL },
            { .name = "disableNoiseSquelch", .has_arg = no_argument, .val = 257, .flag = NULL },
            {0}
        };
        while ((p = getopt_long(opts->argc, opts->argv, "a:A:v:V:i", longopts, NULL)) != -1)
        {
            switch (p)
            {
                case 'a':
                    if (sscanf(optarg, "%f", &inampmod) < 1) goto parsefltshortoptfail;
                    break;

                case 'A':
                    if (sscanf(optarg, "%f", &outampmod) < 1) goto parsefltshortoptfail;
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &involmod) < 1) goto parsefltshortoptfail;
                    break;

                case 'V':
                    if (sscanf(optarg, "%f", &outvolmod) < 1) goto parsefltshortoptfail;
                    break;

                case 'i':
                    allowidlerenders = true;
                    break;

                case 256:
                    if (sscanf(optarg, "%f", &noisevolume) < 1)
                    { lapi->log(DSPLogLevel_Fatal, "error parsing option --noiseVolume (required float)"); return DSPModuleReturnCode_Stop; }
                    break;

                case 257:
                    noisesquelch = false;
                    break;

                parsefltshortoptfail:
                    lapi->log(DSPLogLevel_Fatal, "failed parsing option -%c (required float)", (char)p);
                return DSPModuleReturnCode_Stop;
            }
        }
    }

    if (alutil_init(lapi->log, 48000, false, ALC_MONO_SOFT)) return DSPModuleReturnCode_Stop;
    if (!alIsExtensionPresent("AL_SOFT_effect_target"))
    { lapi->log(DSPLogLevel_Fatal, "required \"AL_SOFT_effect_target\" OpenAL extension doesn't supported on this platform"); return DSPModuleReturnCode_Stop; }
    if (alutil_loadEFX(lapi->log)) return DSPModuleReturnCode_Stop;

    // ===============================

    if (!(inleftport = lapi->addport("input_left", NULL, DSPPortDirection_Input, 0)))
    { lapi->log(DSPLogLevel_Fatal, "error adding port for input left channel"); return DSPModuleReturnCode_Stop; }
    if (!(inrightport = lapi->addport("input_right", NULL, DSPPortDirection_Input, 0)))
    { lapi->log(DSPLogLevel_Fatal, "error adding port for input right channel"); return DSPModuleReturnCode_Stop; }
    
    if (!(outport = lapi->addport("output", NULL, DSPPortDirection_Output, 0)))
    { lapi->log(DSPLogLevel_Fatal, "error adding mono-output port"); return DSPModuleReturnCode_Stop; }
    
    // ===============================
    
    alGenBuffers(1, &buffer);
    alGenSources(1, &source);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcef(source, AL_ROLLOFF_FACTOR, 0);

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
    alEffectf(effect, AL_DISTORTION_EDGE, 0.3);
    alEffectf(effect, AL_DISTORTION_GAIN, 0.6);
    alAuxiliaryEffectSloti(slots[0], AL_EFFECTSLOT_EFFECT, effect);
    alAuxiliaryEffectSloti(slots[0], AL_EFFECTSLOT_TARGET_SOFT, slots[1]);

    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EQUALIZER);
    alEffectf(effect, AL_EQUALIZER_LOW_CUTOFF, 300);
    alEffectf(effect, AL_EQUALIZER_LOW_GAIN, AL_EQUALIZER_MIN_LOW_GAIN);
    
    alEffectf(effect, AL_EQUALIZER_MID1_CENTER, 1650);
    alEffectf(effect, AL_EQUALIZER_MID1_GAIN, 1);
    alEffectf(effect, AL_EQUALIZER_MID1_WIDTH, 1);
    
    alEffectf(effect, AL_EQUALIZER_MID2_CENTER, 3000);
    alEffectf(effect, AL_EQUALIZER_MID2_GAIN, AL_EQUALIZER_MIN_MID2_GAIN);
    alEffectf(effect, AL_EQUALIZER_MID2_WIDTH, 0.5);
    
    alEffectf(effect, AL_EQUALIZER_HIGH_CUTOFF, 4000);
    alEffectf(effect, AL_EQUALIZER_HIGH_GAIN, AL_EQUALIZER_MIN_HIGH_GAIN);

    alAuxiliaryEffectSloti(slots[1], AL_EFFECTSLOT_EFFECT, effect);
    alDeleteEffects(1, &effect);

    alSource3i(source, AL_AUXILIARY_SEND_FILTER, slots[0], 0, AL_FILTER_NULL);
    
    // ===============================

    lapi->log(DSPLogLevel_Info, "inampmod: %f\ninvolmod: %f\noutampmod: %f\noutvolmod: %f\nidle renders: %s\n"
        "effectprops:\n  noisesquelch: %s\n  noisevolume: %f",
        inampmod, involmod, outampmod, outvolmod, allowidlerenders ? "allowed" : "not allowed", noisesquelch ? "true" : "false", noisevolume);

    opts->sysname = "OALoldradioEffect";
    opts->dispname = "OpenAL Old Radio Effect";
    return DSPModuleReturnCode_Continue;
}

#define RNDF() (rand() / (float)RAND_MAX)
#define RNDD() (rand() / (double)RAND_MAX)
#define PI 3.14159265358979323846

DSPModuleReturnCode dspmodule_process(const DSPLoaderAPI * restrict lapi, DSPModuleProcessFunctionOptions * restrict opts)
{
    float *out = lapi->getportbuffer(outport, opts->duration);
    if (!(allowidlerenders || out)) return DSPModuleReturnCode_Continue;
    
    const float *inleft = lapi->getportbuffer(inleftport, opts->duration);
    const float *inright = lapi->getportbuffer(inrightport, opts->duration);

    // ===============================
    
    alSourceRewind(source);
    alSourcei(source, AL_BUFFER, 0);

    size_t buffsize = (opts->duration + 1) * sizeof(float);
    float buff[buffsize];
    for (unsigned long i = 0; i < opts->duration; i++)
    {
        float sig = adjf(((inleft ? inleft[i] : 0) + (inright ? inright[i] : 0)) * 0.5, inampmod) * involmod;

        // ===============================

        double time = (opts->position + i) / (double)opts->rate;
        
        float noise = (
            (RNDF() * 2 - 1) * ((RNDF() < 0.01 ? 2 : 0) + 0.15) +
            sin(time * 220 * 2 * PI) * RNDF() +
            sin(time * 50 * 2 * PI) * RNDF()
        ) * 0.1;
        
        if (noisesquelch)
        {
            static double sqstarttime = -1;
            static double sqendtime = -1; if (sqendtime < 0) sqendtime = time;
            static double sqnextoffset = -1; if (sqnextoffset < 0) sqnextoffset = 1 + RNDD() * 2;
            if (time - sqendtime > sqnextoffset)
            {
                sqstarttime = time;
                sqendtime = time + RNDD() * 0.09 + 0.01;
                sqnextoffset = 1 + RNDD() * 2;
            }

            float sqamp;
            if (sqendtime >= time && sqstarttime >= 0)
            {
                double factor = (time - sqstarttime) / (sqendtime - sqstarttime);
                sqamp = pow(sin(factor * PI), 20) * 1.5;
            }
            else sqamp = 0;

            noise = noise + noise * sqamp;
        }

        // ===============================

        buff[i] = sig + noise * noisevolume;
    }
    alBufferData(buffer, AL_FORMAT_MONO_FLOAT32, buff, buffsize, opts->rate);

    alSourcei(source, AL_BUFFER, buffer);
    alSourcePlay(source);

    // ===============================

    if (alutil_render(lapi->log, buff, opts->duration, opts->rate)) return DSPModuleReturnCode_Stop;
    if (out) for (unsigned long i = 0; i < opts->duration; i++) out[i] = adjf(buff[i], outampmod) * outvolmod;

    return DSPModuleReturnCode_Continue;
}