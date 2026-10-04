/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "base/base.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

const struct option effect_longopts[] =
{
    { .name = "frequency", .has_arg = required_argument, .val = 256, .flag = NULL },
    { .name = "highpassCutoff", .has_arg = required_argument, .val = 257, .flag = NULL },
    { .name = "waveform", .has_arg = required_argument, .val = 258, .flag = NULL },
    {0}
};

#include "macro.h"
static floatopt floatopts[2] = {0};
static intopt intopts[1] = {0};

unsigned short effect_optcallback(int optid)
{
    switch (optid)
    {
        PARSELONGFLOATOPT(256, 0, "frequency")
        PARSELONGFLOATOPT(257, 1, "highpassCutoff")

        case 258:
            if (!strcmp(optarg, "sinusoid")) intopts[0].value = AL_RING_MODULATOR_SINUSOID;
            else if (!strcmp(optarg, "sawtooth")) intopts[0].value = AL_RING_MODULATOR_SAWTOOTH;
            else if (!strcmp(optarg, "square")) intopts[0].value = AL_RING_MODULATOR_SQUARE;
            else { printf("incorrect --waveform enumeration option value (allowed: \"sinusoid\", \"sawtooth\" or \"square\", got: \"%s\")\n", optarg); return 1; }
            intopts[0].has = true;
            break;
    }
    return 0;
}

unsigned short effect_poststartup(const struct json_object *configroot, bool strongconfoptcheck, ALuint effect, const char **sysname, const char **dispname)
{
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_RING_MODULATOR);

    if (configroot)
    {
        struct json_object *jobj;
        float f;

        GETFLTCONFOPTHELPER(0, "frequency", AL_RING_MODULATOR_FREQUENCY);
        GETFLTCONFOPTHELPER(1, "highpassCutoff", AL_RING_MODULATOR_HIGHPASS_CUTOFF);
        
        if (!intopts[0].has)
        {
            if (json_object_object_get_ex(configroot, "waveform", &jobj))
            {
                if (json_object_get_type(jobj) != json_type_string) { puts("parsing \"waveform\" config option failed (required string)"); return 1; }
                const char *waveform = json_object_get_string(jobj);
                if (!strcmp(waveform, "sinusoid")) alEffecti(effect, AL_RING_MODULATOR_WAVEFORM, AL_RING_MODULATOR_SINUSOID);
                else if (!strcmp(waveform, "sawtooth")) alEffecti(effect, AL_RING_MODULATOR_WAVEFORM, AL_RING_MODULATOR_SAWTOOTH);
                else if (!strcmp(waveform, "square")) alEffecti(effect, AL_RING_MODULATOR_WAVEFORM, AL_RING_MODULATOR_SQUARE);
                else { printf("incorrect \"waveform\" enumeration option value (allowed: \"sinusoid\", \"sawtooth\" or \"square\", got: \"%s\")\n", waveform); return 1; }
            }
            else if (strongconfoptcheck) { puts("key \"waveform\" doesnt found in config file"); return 1; }
        }
    }

    SETEFFFLOATPROPFROMOPT(0, AL_RING_MODULATOR_FREQUENCY);
    SETEFFFLOATPROPFROMOPT(1, AL_RING_MODULATOR_HIGHPASS_CUTOFF);
    
    SETEFFINTPROPFROMOPT(0, AL_RING_MODULATOR_WAVEFORM);

    *sysname = "ringmod";
    *dispname = "OpenAL Ring Modulator";
    return 0;
}

void effect_printprops(ALuint effect)
{
    puts("effectprops:");

    union { float f; int i; } v;
    alGetEffectf(effect, AL_RING_MODULATOR_FREQUENCY, &v.f); printf("  frequency: %f\n", v.f);
    alGetEffectf(effect, AL_RING_MODULATOR_HIGHPASS_CUTOFF, &v.f); printf("  highpassCutoff: %f\n", v.f);

    alGetEffecti(effect, AL_RING_MODULATOR_WAVEFORM, &v.i);
    printf("  waveform: ");
    switch (v.i)
    {
        case AL_RING_MODULATOR_SINUSOID:
            puts("sinusoid");
            break;

        case AL_RING_MODULATOR_SAWTOOTH:
            puts("sawtooth");
            break;

        case AL_RING_MODULATOR_SQUARE:
            puts("square");
            break;

        default:
            puts("(undefined)");
    }
}