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

const unsigned short dspmodule_requiredAPIversion = 1;

static float origgain = 1, effectgain = 1, inampmod = 0, involmod = 1, outampmod = 0, outvolmod = 1;
static void *inleftport, *inrightport, *outleftport, *outrightport;
static ALuint sources[2], buffers[2];
static bool allowidlerenders = false, enablesquelch = true;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    {
        int p;
        while ((p = getopt(argc, argv, "a:A:v:V:il")) != -1)
        {
            switch (p)
            {
                case 'a':
                    if (sscanf(optarg, "%f", &inampmod) < 1) { puts("error parsing option -a (required float)"); return 1; }
                    break;

                case 'A':
                    if (sscanf(optarg, "%f", &outampmod) < 1) { puts("error parsing option -A (required float)"); return 1; }
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &involmod) < 1) { puts("error parsing option -v (required float)"); return 1; }
                    break;

                case 'V':
                    if (sscanf(optarg, "%f", &outvolmod) < 1) { puts("error parsing option -V (required float)"); return 1; }
                    break;

                case 'i':
                    allowidlerenders = true;
                    break;

                case 'l':
                    enablesquelch = false;
                    break;
            }
        }
    }

    if (alutil_init(48000, true, false, ALC_STEREO_SOFT)) return 1;
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
    
    alGenBuffers(2, buffers);
    alGenSources(2, sources);
    alSourcei(sources[0], AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(sources[0], AL_ROLLOFF_FACTOR, 0);
    alSourcei(sources[1], AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(sources[1], AL_ROLLOFF_FACTOR, 0);

    alSourcef(sources[0], AL_GAIN, 1);
    alSourcef(sources[1], AL_GAIN, 0.1);

    ALuint filter;
    alGenFilters(1, &filter);

    alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    alFilterf(filter, AL_LOWPASS_MAX_GAINHF, 1);
    alFilterf(filter, AL_LOWPASS_GAIN, 0);

    alSourcei(sources[0], AL_DIRECT_FILTER, filter);
    alSourcei(sources[1], AL_DIRECT_FILTER, filter);
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

    alSource3i(sources[0], AL_AUXILIARY_SEND_FILTER, slots[0], 0, AL_FILTER_NULL);
    alSource3i(sources[1], AL_AUXILIARY_SEND_FILTER, slots[0], 0, AL_FILTER_NULL);
    
    // ===============================

    printf("inampmod: %f\ninvolmod: %f\noutampmod: %f\noutvolmod: %f\nidle renders: %s\nenablesquelch: %s\n",
        inampmod, involmod, outampmod, outvolmod, allowidlerenders ? "allowed" : "not allowed", enablesquelch ? "true" : "false");

    *sysname = "oldradio";
    *dispname = "OpenAL Old Radio";
    return 0;
}

#define RNDF() (rand() / (float)RAND_MAX)
#define RNDD() (rand() / (double)RAND_MAX)
#define PI 3.14159265358979323846

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long duration, unsigned long rate, unsigned long long nsectime)
{
    float *outleft = lapi->getportbuffer(outleftport, duration);
    float *outright = lapi->getportbuffer(outrightport, duration);
    if (!(allowidlerenders || outleft || outright)) return 0;
    
    const float *inleft = lapi->getportbuffer(inleftport, duration);
    const float *inright = lapi->getportbuffer(inrightport, duration);

    // ===============================
    
    alSourceRewind(sources[0]);
    alSourcei(sources[0], AL_BUFFER, 0);

    size_t buffsize = (duration + 1) * sizeof(float);
    float buff[buffsize];
    for (unsigned long i = 0; i < duration; i++)
    { buff[i] = adjf(((inleft ? inleft[i] : 0) + (inright ? inright[i] : 0)) * 0.5, inampmod) * involmod; }
    alBufferData(buffers[0], AL_FORMAT_MONO_FLOAT32, buff, buffsize, rate);

    alSourcei(sources[0], AL_BUFFER, buffers[0]);
    alSourcePlay(sources[0]);

    // ===============================

    alSourceRewind(sources[1]);
    alSourcei(sources[1], AL_BUFFER, 0);

    for (unsigned long i = 0; i < duration; i++)
    {
        double time = (position + i) / (double)rate;
        
        float v = (
            (RNDF() * 2 - 1) * ((RNDF() < 0.01 ? 2 : 0) + 0.15) +
            sin(time * 220 * 2 * PI) * RNDF() +
            sin(time * 50 * 2 * PI) * RNDF()
        ) * 0.1;
        
        if (enablesquelch)
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

            buff[i] = clampf(v + v * sqamp, -1, 1);
        }
        else buff[i] = v;
    }

    alBufferData(buffers[1], AL_FORMAT_MONO_FLOAT32, buff, duration * sizeof(float), rate);
    alSourcei(sources[1], AL_BUFFER, buffers[1]);
    alSourcePlay(sources[1]);

    // ===============================

    if (alutil_render(buff, duration, rate)) return 1;
    if (outleft || outright) for (unsigned long i = 0; i < duration; i++)
    {
        if (outleft) outleft[i] = adjf(buff[i * 2], outampmod) * outvolmod;
        if (outright) outright[i] = adjf(buff[i * 2 + 1], outampmod) * outvolmod;
    }

    return 0;
}