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

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include "albase/main.h"
#include "albase/EFX.h"
#include "jsonutil/jsonutil.h"

const unsigned short dspmodule_requiredAPIversion = 1;

static float origgain = 1, effectgain = 1, inampmod = 0, involmod = 1, outampmod = 0, outvolmod = 1;
static void *inleftport, *inrightport, *outleftport, *outrightport;
static ALuint source, buffer;

#define GETFLTVEC3CONFOPTHELPER(strname, alname) \
    if (!json_object_object_get_ex(configroot, strname, &jobj)) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    if (jsonutil_getvec3f(jobj, vec3f)) { puts("failed parsing option \"" strname "\" (required vector/array of 3 floats)"); return 1; }\
    alEffectfv(effect, alname, vec3f);

#define GETFLTCONFOPTHELPER(strname, alname) \
    if (!json_object_object_get_ex(configroot, strname, &jobj)) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    if (jsonutil_getfloat(jobj, vec3f)) { puts("parsing \"" strname "\" config option failed (required float)"); return 1; }\
    alEffectf(effect, alname, *vec3f);

#define GETBOOLCONFOPTHELPER(strname, alname) \
    if (!json_object_object_get_ex(configroot, strname, &jobj)) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    if (jsonutil_getbool(jobj, &flag)) { puts("parsing \"" strname "\" config option failed (required boolean)"); return 1; }\
    alEffecti(effect, alname, flag);

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{   
    const char *configfilename = NULL;
    struct json_object *configroot = NULL;
    {
        int p;
        while ((p = getopt(argc, argv, "g:G:f:a:v:A:V:")) != -1)
        {
            switch (p)
            {
                case 'a':
                    if (sscanf(optarg, "%f", &inampmod) < 1) { puts("error parsing option -a"); return 1; }
                    break;

                case 'A':
                    if (sscanf(optarg, "%f", &outampmod) < 1) { puts("error parsing option -A"); return 1; }
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &involmod) < 1) { puts("error parsing option -v"); return 1; }
                    break;

                case 'V':
                    if (sscanf(optarg, "%f", &outvolmod) < 1) { puts("error parsing option -V"); return 1; }
                    break;

                case 'g':
                    if (sscanf(optarg, "%f", &origgain) < 1) { puts("error parsing option -g"); return 1; }
                    origgain = clampf(origgain, 0, 1);
                    break;

                case 'G':
                    if (sscanf(optarg, "%f", &effectgain) < 1) { puts("error parsing option -G"); return 1; }
                    effectgain = clampf(effectgain, 0, 1);
                    break;

                case 'f':
                {
                    FILE *f = fopen(optarg, "r");
                    if (!f) { puts("unable to open config file"); return 1; }

                    if (fseek(f, 0, SEEK_END)) { puts("failed to seek to end of config file"); goto errorquit_afterfopen; }
                    long size = ftell(f);
                    if (size < 0) { puts("failed to tell size of config file"); goto errorquit_afterfopen; }
                    if (fseek(f, 0, SEEK_SET)) { puts("failed to seek to start of config file"); goto errorquit_afterfopen; }

                    char *contents = malloc(size + 1);
                    if (!contents) { puts("memory allocation failed"); goto errorquit_afterfopen; }
                    bool success = fread(contents, size, 1, f);
                    fclose(f);
                    if (!success) { puts("failed reading config file"); goto errorquit_afteralloc; }
                    contents[size] = '\0';

                    enum json_tokener_error jerr;
                    configroot = json_tokener_parse_verbose(contents, &jerr);
                    free(contents);
                    if (jerr != json_tokener_success) { printf("JSON parsing error: %s\n", json_tokener_error_desc(jerr)); goto errorquit_afteralloc;  }

                    configfilename = optarg;
                    break;
                    errorquit_afteralloc:
                        free(contents);
                    errorquit_afterfopen:
                        fclose(f);
                    return 1;
                }
            }
        }
    }

    if (albase_init(48000, true)) return 1;
    if (albase_loadEFX()) return 1;

    // ===============================

    ALuint effect;
    alGenEffects(1, &effect);
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

    if (configroot)
    {
        bool flag;
        float vec3f[3];
        struct json_object *jobj;

        GETFLTCONFOPTHELPER("density", AL_EAXREVERB_DENSITY);
        GETFLTCONFOPTHELPER("diffusion", AL_EAXREVERB_DIFFUSION);
        GETFLTCONFOPTHELPER("gain", AL_EAXREVERB_GAIN);
        GETFLTCONFOPTHELPER("gainHF", AL_EAXREVERB_GAINHF);
        GETFLTCONFOPTHELPER("gainLF", AL_EAXREVERB_GAINLF);
        GETFLTCONFOPTHELPER("decayTime", AL_EAXREVERB_DECAY_TIME);
        GETFLTCONFOPTHELPER("decayHFRatio", AL_EAXREVERB_DECAY_HFRATIO);
        GETFLTCONFOPTHELPER("decayLFRatio", AL_EAXREVERB_DECAY_LFRATIO);
        GETFLTCONFOPTHELPER("reflectionsDelay", AL_EAXREVERB_REFLECTIONS_DELAY);
        GETFLTCONFOPTHELPER("reflectionsGain", AL_EAXREVERB_REFLECTIONS_GAIN);
        GETFLTVEC3CONFOPTHELPER("reflectionsPan", AL_EAXREVERB_REFLECTIONS_PAN);
        GETFLTCONFOPTHELPER("lateReverbDelay", AL_EAXREVERB_LATE_REVERB_DELAY);
        GETFLTCONFOPTHELPER("lateReverbGain", AL_EAXREVERB_LATE_REVERB_GAIN);
        GETFLTVEC3CONFOPTHELPER("lateReverbPan", AL_EAXREVERB_LATE_REVERB_PAN);
        GETFLTCONFOPTHELPER("lateReverbDelay", AL_EAXREVERB_LATE_REVERB_DELAY);
        GETFLTCONFOPTHELPER("echoDepth", AL_EAXREVERB_ECHO_DEPTH);
        GETFLTCONFOPTHELPER("echoTime", AL_EAXREVERB_ECHO_TIME);
        GETFLTCONFOPTHELPER("modulationDepth", AL_EAXREVERB_MODULATION_DEPTH);
        GETFLTCONFOPTHELPER("modulationTime", AL_EAXREVERB_MODULATION_TIME);
        GETFLTCONFOPTHELPER("airAbsorptionGainHF", AL_EAXREVERB_AIR_ABSORPTION_GAINHF);
        GETFLTCONFOPTHELPER("HFReference", AL_EAXREVERB_HFREFERENCE);
        GETFLTCONFOPTHELPER("LFReference", AL_EAXREVERB_LFREFERENCE);
        GETFLTCONFOPTHELPER("roomRolloffFactor", AL_EAXREVERB_ROOM_ROLLOFF_FACTOR);
        GETBOOLCONFOPTHELPER("decayHFLimit", AL_EAXREVERB_DECAY_HFLIMIT);

        json_object_put(configroot);
    }

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

    alGenSources(1, &source);
    
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(source, AL_ROLLOFF_FACTOR, 0);
    
    ALuint filter;
    alGenFilters(1, &filter);
    alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    alFilterf(filter, AL_LOWPASS_GAINHF, 1);
    alFilterf(filter, AL_LOWPASS_GAIN, origgain);
    alSourcei(source, AL_DIRECT_FILTER, filter);
    
    // ===============================
    
    ALuint slot;
    alGenAuxiliaryEffectSlots(1, &slot);
    alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, effect);
    alDeleteEffects(1, &effect);
    
    alFilterf(filter, AL_LOWPASS_GAIN, effectgain);
    alSource3i(source, AL_AUXILIARY_SEND_FILTER, slot, 0, filter);
    alDeleteFilters(1, &filter);
    
    // ===============================

    alGenBuffers(1, &buffer);

    // ===============================
    
    printf("inampmod: %f\ninvolmod: %f\noriggain: %f\neffectgain: %f\noutampmod: %f\noutvolmod: %f\n",
        inampmod, involmod, origgain, effectgain, outampmod, outvolmod);
    if (configfilename) printf("configfilename: %s\n", configfilename);
    else puts("config file not specified");
    *sysname = "eaxreverb";
    *dispname = "OpenAL EAX Reverb.";
    return 0;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long duration, unsigned long rate, unsigned long long nsectime)
{
    float *outleft = lapi->getportbuffer(outleftport, duration);
    if (!outleft) return 0;
    float *outright = lapi->getportbuffer(outrightport, duration);
    if (!outright) return 0;
    
    const float *inleft = lapi->getportbuffer(inleftport, duration);
    const float *inright = lapi->getportbuffer(inrightport, duration);
    
    alSourceRewind(source);
    alSourcei(source, AL_BUFFER, 0);

    size_t buffsize = (duration + 1) * sizeof(float) * 2;
    float buff[buffsize];
    for (size_t i = 0; i < ((size_t)duration) << 1; i++)
    { buff[i] = i & 1 ? (inright ? inright[i >> 1] : 0) : (inleft ? inleft[i >> 1] : 0); }
    alBufferData(buffer, AL_FORMAT_STEREO_FLOAT32, buff, buffsize, rate);

    alSourcei(source, AL_BUFFER, buffer);
    alSourcePlay(source);

    if (albase_render(buff, duration, rate)) return 1;
    if (outleft || outright) for (unsigned long i = 0; i < duration; i++)
    {
        if (outleft) outleft[i] = buff[i * 2];
        if (outright) outright[i] = buff[i * 2 + 1];
    }

    return 0;
}