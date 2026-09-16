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

#include <al.h>
#include <alc.h>
#include <alext.h>
#include <efx-presets.h>

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include "albase/general.h"
#include "albase/EFX.h"
#include "jsonutil/jsonutil.h"

const unsigned short dspmodule_requiredAPIversion = 1;

static float origgain = 1, reverbgain = 1;

#define GETFLTVEC3CONFOPTHELPER(strname, alname) \
    if (!json_object_object_get_ex(configroot, strname, &tmp2_jobj)) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    if (json_object_get_type(tmp2_jobj) != json_type_array) { puts("value by key \"" strname "\" must have an array type"); return 1; }\
    \
    if (!(tmp_jobj = json_object_array_get_idx(tmp2_jobj, 0))) { puts("value by index 0 in array with key \"" strname "\" doesn't exist"); return 1; }\
    if (jsonutil_getfloat(tmp_jobj, tmp_floats)) { puts("unable to parse value by index 0 in array with key \"" strname "\" (required float)"); return 1; }\
    \
    if (!(tmp_jobj = json_object_array_get_idx(tmp2_jobj, 1))) { puts("value by index 1 in array with key \"" strname "\" doesn't exist"); return 1; }\
    if (jsonutil_getfloat(tmp_jobj, tmp_floats + 1)) { puts("unable to parse value by index 1 in array with key \"" strname "\" (required float)"); return 1; }\
    \
    if (!(tmp_jobj = json_object_array_get_idx(tmp2_jobj, 2))) { puts("value by index 2 in array with key \"" strname "\" doesn't exist"); return 1; }\
    if (jsonutil_getfloat(tmp_jobj, tmp_floats + 2)) { puts("unable to parse value by index 2 in array with key \"" strname "\" (required float)"); return 1; }\
    \
    alEffectfv(effect, alname, tmp_floats);

#define GETFLTCONFOPTHELPER(strname, alname) \
    if (!json_object_object_get_ex(configroot, strname, &tmp_jobj)) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    if (jsonutil_getfloat(tmp_jobj, tmp_floats)) { puts("parsing \"" strname "\" config option failed (required float)"); return 1; }\
    alEffectf(effect, alname, *tmp_floats);

#define GETBOOLCONFOPTHELPER(strname, alname) \
    if (!json_object_object_get_ex(configroot, strname, &tmp_jobj)) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    if (jsonutil_getbool(tmp_jobj, &tmp_bool)) { puts("parsing \"" strname "\" config option failed (required boolean)"); return 1; }\
    alEffecti(effect, alname, tmp_bool);

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{   
    const char *configfilename = NULL;
    struct json_object *configroot;
    {
        int p;
        while ((p = getopt(argc, argv, "g:G:f:a:v:A:V:")) != -1)
        {
            switch (p)
            {
                case 'a':
                    if (sscanf(optarg, "%f", &albase_inampmod) < 1) { puts("error parsing option -a"); return 1; }
                    break;

                case 'A':
                    if (sscanf(optarg, "%f", &albase_outampmod) < 1) { puts("error parsing option -A"); return 1; }
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &albase_involmod) < 1) { puts("error parsing option -v"); return 1; }
                    break;

                case 'V':
                    if (sscanf(optarg, "%f", &albase_outvolmod) < 1) { puts("error parsing option -V"); return 1; }
                    break;

                case 'g':
                    if (sscanf(optarg, "%f", &origgain) < 1) { puts("error parsing option -g"); return 1; }
                    origgain = clampf(origgain, 0, 1);
                    break;

                case 'G':
                    if (sscanf(optarg, "%f", &reverbgain) < 1) { puts("error parsing option -G"); return 1; }
                    reverbgain = clampf(reverbgain, 0, 1);
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

    if (albase_init(lapi)) return 1;

    // ===============================

    ALuint effect;
    alGenEffects(1, &effect);
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

    if (configfilename)
    {
        bool tmp_bool;
        float tmp_floats[3];
        struct json_object *tmp_jobj, *tmp2_jobj;

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
    }

    // ===============================
    
    ALuint filter, slot;
    alGenFilters(1, &filter);
    alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    alFilterf(filter, AL_LOWPASS_GAINHF, 1);
    alFilterf(filter, AL_LOWPASS_GAIN, reverbgain);
    
    alGenAuxiliaryEffectSlots(1, &slot);
    alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, effect);
    alDeleteEffects(1, &effect);

    alSource3i(albase_source, AL_AUXILIARY_SEND_FILTER, slot, 0, filter);

    alFilterf(filter, AL_LOWPASS_GAIN, origgain);
    alSourcei(albase_source, AL_DIRECT_FILTER, filter);
    alDeleteFilters(1, &filter);
    
    printf("inampmod: %f\ninvolmod: %f\noriggain: %f\nreverbgain: %f\noutampmod: %f\noutvolmod: %f\n",
        albase_inampmod, albase_involmod, origgain, reverbgain, albase_outampmod, albase_outvolmod);
    if (configfilename) printf("configfilename: %s\n", configfilename);
    else puts("config file not specified");
    *sysname = "eaxreverb";
    *dispname = "OpenAL EAX Reverb.";
    return 0;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long duration, unsigned long rate, unsigned long long nsectime)
{ return albase_process(lapi, duration, rate); }