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
    { .name = "edge", .has_arg = required_argument, .val = 256, .flag = NULL },
    { .name = "gain", .has_arg = required_argument, .val = 257, .flag = NULL },
    { .name = "lowpassCutoff", .has_arg = required_argument, .val = 258, .flag = NULL },
    { .name = "eqCenter", .has_arg = required_argument, .val = 259, .flag = NULL },
    { .name = "eqBandwidth", .has_arg = required_argument, .val = 260, .flag = NULL },
    {0}
};

#include "macro.h"
floatopt floatopts[5] = {0};

unsigned short effect_optcallback(int optid)
{
    switch (optid)
    {
        PARSELONGFLOATOPT(256, 0, "edge")
        PARSELONGFLOATOPT(257, 1, "gain")
        PARSELONGFLOATOPT(258, 2, "lowpassCutoff")
        PARSELONGFLOATOPT(259, 3, "eqCenter")
        PARSELONGFLOATOPT(260, 4, "eqBandwidth")
    }
    return 0;
}

unsigned short effect_poststartup(const struct json_object *configroot, bool strongconfoptcheck, ALuint effect, const char **sysname, const char **dispname)
{
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_DISTORTION);

    if (configroot)
    {
        struct json_object *jobj;
        float f;
        
        GETFLTCONFOPTHELPER(0, "edge", AL_DISTORTION_EDGE);
        GETFLTCONFOPTHELPER(1, "gain", AL_DISTORTION_GAIN);
        GETFLTCONFOPTHELPER(2, "lowpassCutoff", AL_DISTORTION_LOWPASS_CUTOFF);
        GETFLTCONFOPTHELPER(3, "eqCenter", AL_DISTORTION_EQCENTER);
        GETFLTCONFOPTHELPER(4, "eqBandwidth", AL_DISTORTION_EQBANDWIDTH);
    }

    SETEFFFLOATPROPFROMOPT(0, AL_DISTORTION_EDGE);
    SETEFFFLOATPROPFROMOPT(1, AL_DISTORTION_GAIN);
    SETEFFFLOATPROPFROMOPT(2, AL_DISTORTION_LOWPASS_CUTOFF);
    SETEFFFLOATPROPFROMOPT(3, AL_DISTORTION_EQCENTER);
    SETEFFFLOATPROPFROMOPT(4, AL_DISTORTION_EQBANDWIDTH);

    *sysname = "distortion";
    *dispname = "OpenAL Distortion";
    return 0;
}

void effect_printprops(ALuint effect)
{
    puts("effectprops:");

    float f;
    alGetEffectf(effect, AL_DISTORTION_EDGE, &f); printf("  edge: %f\n", f);
    alGetEffectf(effect, AL_DISTORTION_GAIN, &f); printf("  gain: %f\n", f);
    alGetEffectf(effect, AL_DISTORTION_LOWPASS_CUTOFF, &f); printf("  lowpassCutoff: %f\n", f);
    alGetEffectf(effect, AL_DISTORTION_EQCENTER, &f); printf("  eqCenter: %f\n", f);
    alGetEffectf(effect, AL_DISTORTION_EQBANDWIDTH, &f); printf("  eqBandwidth: %f\n", f);
}