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

static float origgain = 1, chorusgain = 1;

#define GETFLTVEC3CONFOPTHELPER(strname, alname) \
    if (!json_object_object_get_ex(configroot, strname, &tmp_jobj)) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    if (jsonutil_getvec3f(tmp_jobj, tmp_floats)) { puts("failed parsing option \"" strname "\" (required vector/array of 3 floats)"); return 1; }\
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
    struct json_object *configroot = NULL;
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
                    if (sscanf(optarg, "%f", &chorusgain) < 1) { puts("error parsing option -G"); return 1; }
                    chorusgain = clampf(chorusgain, 0, 1);
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
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_CHORUS);

    if (configroot)
    {
        bool tmp_bool;
        float tmp_floats[3];
        struct json_object *tmp_jobj, *tmp2_jobj;

        GETFLTCONFOPTHELPER("delay", AL_CHORUS_DELAY);
        GETFLTCONFOPTHELPER("depth", AL_CHORUS_DEPTH);
        GETFLTCONFOPTHELPER("feedback", AL_CHORUS_FEEDBACK);
        GETFLTCONFOPTHELPER("phase", AL_CHORUS_PHASE);
        GETFLTCONFOPTHELPER("rate", AL_CHORUS_RATE);
        
        if (!json_object_object_get_ex(configroot, "waveform", &tmp_jobj)) { puts("key \"waveform\" doesnt found in config file"); return 1; }
        if (json_object_get_type(tmp_jobj) != json_type_string) { puts("parsing \"waveform\" config option failed (required string)"); return 1; }
        const char *waveform = json_object_get_string(tmp_jobj);
        if (!strcmp(waveform, "sinusoid")) alEffecti(effect, AL_CHORUS_WAVEFORM, AL_CHORUS_WAVEFORM_SINUSOID);
        else if (!strcmp(waveform, "triangle")) alEffecti(effect, AL_CHORUS_WAVEFORM, AL_CHORUS_WAVEFORM_TRIANGLE);
        else { printf("incorrect \"waveform\" option value (allowed: \"sinusoid\", \"triangle\", got: \"%s\")\n", waveform); return 1; }

        json_object_put(configroot);
    }

    // ===============================
    
    ALuint filter, slot;
    alGenFilters(1, &filter);
    alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    alFilterf(filter, AL_LOWPASS_GAINHF, 1);
    alFilterf(filter, AL_LOWPASS_GAIN, chorusgain);
    
    alGenAuxiliaryEffectSlots(1, &slot);
    alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, effect);
    alDeleteEffects(1, &effect);

    alSource3i(albase_source, AL_AUXILIARY_SEND_FILTER, slot, 0, filter);

    alFilterf(filter, AL_LOWPASS_GAIN, origgain);
    alSourcei(albase_source, AL_DIRECT_FILTER, filter);
    alDeleteFilters(1, &filter);
    
    printf("inampmod: %f\ninvolmod: %f\noriggain: %f\nchorusgain: %f\noutampmod: %f\noutvolmod: %f\n",
        albase_inampmod, albase_involmod, origgain, chorusgain, albase_outampmod, albase_outvolmod);
    if (configfilename) printf("configfilename: %s\n", configfilename);
    else puts("config file not specified");
    *sysname = "chorus";
    *dispname = "OpenAL Chorus";
    return 0;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long duration, unsigned long rate, unsigned long long nsectime)
{ return albase_process(lapi, duration, rate); }