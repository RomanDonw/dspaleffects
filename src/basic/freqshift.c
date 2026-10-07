/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "base/base.h"

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

const struct option effect_longopts[] =
{
    { .name = "frequency", .has_arg = required_argument, .val = 256, .flag = NULL },
    { .name = "leftDirection", .has_arg = required_argument, .val = 257, .flag = NULL },
    { .name = "rightDirection", .has_arg = required_argument, .val = 258, .flag = NULL },
    {0}
};

#include "macro.h"
static floatopt floatopts[1] = {0};
static intopt intopts[2] = {0};

#define PARSELONGDIROPT(optid, optindex, optname) \
    case optid:\
        if (!strcmp(optarg, "down")) intopts[optindex].value = AL_FREQUENCY_SHIFTER_DIRECTION_DOWN;\
        else if (!strcmp(optarg, "up")) intopts[optindex].value = AL_FREQUENCY_SHIFTER_DIRECTION_UP;\
        else if (!strcmp(optarg, "off")) intopts[optindex].value = AL_FREQUENCY_SHIFTER_DIRECTION_OFF;\
        else { printf("incorrect --" optname " enumeration option value (allowed: \"down\", \"up\" or \"off\", got: \"%s\")\n", optarg); return 1; }\
        intopts[optindex].has = true;\
        break;

#define GETDIRCONFOPTHELPER(intoptsidx, strname, alname) \
    {\
        if (!intopts[intoptsidx].has)\
        {\
            if (json_object_object_get_ex(configroot, strname, &jobj))\
            {\
                if (json_object_get_type(jobj) != json_type_string) { puts("parsing \"" strname "\" config option failed (required string)"); return 1; }\
                const char *direction = json_object_get_string(jobj);\
                if (!strcmp(direction, "down")) alEffecti(effect, alname, AL_FREQUENCY_SHIFTER_DIRECTION_DOWN);\
                else if (!strcmp(direction, "up")) alEffecti(effect, alname, AL_FREQUENCY_SHIFTER_DIRECTION_UP);\
                else if (!strcmp(direction, "off")) alEffecti(effect, alname, AL_FREQUENCY_SHIFTER_DIRECTION_OFF);\
                else { printf("incorrect \"" strname "\" enumeration option value (allowed: \"down\", \"up\" or \"off\", got: \"%s\")\n", direction); return 1; }\
            }\
            else if (strongconfoptcheck) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
        }\
    }

#include <AL/efx.h>

unsigned short effect_optcallback(int optid)
{
    switch (optid)
    {
        PARSELONGFLOATOPT(256, 0, "frequency")
        PARSELONGDIROPT(257, 0, "leftDirection")
        PARSELONGDIROPT(258, 1, "rightDirection")
    }
    return 0;
}

unsigned short effect_poststartup(const struct json_object *configroot, bool strongconfoptcheck, ALuint effect, const char **sysname, const char **dispname)
{
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_FREQUENCY_SHIFTER);

    if (configroot)
    {
        struct json_object *jobj;
        float f;

        GETFLTCONFOPTHELPER(0, "frequency", AL_FREQUENCY_SHIFTER_FREQUENCY);
        GETDIRCONFOPTHELPER(0, "leftDirection", AL_FREQUENCY_SHIFTER_LEFT_DIRECTION);
        GETDIRCONFOPTHELPER(1, "rightDirection", AL_FREQUENCY_SHIFTER_RIGHT_DIRECTION);
    }

    SETEFFFLOATPROPFROMOPT(0, AL_FREQUENCY_SHIFTER_FREQUENCY);
    SETEFFINTPROPFROMOPT(0, AL_FREQUENCY_SHIFTER_LEFT_DIRECTION);
    SETEFFINTPROPFROMOPT(1, AL_FREQUENCY_SHIFTER_RIGHT_DIRECTION);

    *sysname = "freqshift";
    *dispname = "OpenAL Frequency Shifter";
    return 0;
}

#define PRINTDIROPT(optname, alname) \
    {\
        alGetEffecti(effect, alname, &v.i);\
        printf("  " optname ": ");\
        switch (v.i)\
        {\
            case AL_FREQUENCY_SHIFTER_DIRECTION_DOWN:\
                puts("down");\
                break;\
            \
            case AL_FREQUENCY_SHIFTER_DIRECTION_UP:\
                puts("up");\
                break;\
            \
            case AL_FREQUENCY_SHIFTER_DIRECTION_OFF:\
                puts("off");\
                break;\
            \
            default:\
                puts("(undefined)");\
        }\
    }

void effect_printprops(ALuint effect)
{
    puts("effectprops:");

    union { float f; int i; } v;
    alGetEffectf(effect, AL_RING_MODULATOR_FREQUENCY, &v.f); printf("  frequency: %f\n", v.f);
    PRINTDIROPT("leftDirection", AL_FREQUENCY_SHIFTER_LEFT_DIRECTION);
    PRINTDIROPT("rightDirection", AL_FREQUENCY_SHIFTER_RIGHT_DIRECTION);
}