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
    { .name = "lowGain", .has_arg = required_argument, .val = 256, .flag = NULL },
    { .name = "lowCutoff", .has_arg = required_argument, .val = 257, .flag = NULL },
    { .name = "mid1Gain", .has_arg = required_argument, .val = 258, .flag = NULL },
    { .name = "mid1Center", .has_arg = required_argument, .val = 259, .flag = NULL },
    { .name = "mid1width", .has_arg = required_argument, .val = 260, .flag = NULL },
    { .name = "mid2Gain", .has_arg = required_argument, .val = 261, .flag = NULL },
    { .name = "mid2Center", .has_arg = required_argument, .val = 262, .flag = NULL },
    { .name = "mid2Width", .has_arg = required_argument, .val = 263, .flag = NULL },
    { .name = "highGain", .has_arg = required_argument, .val = 264, .flag = NULL },
    { .name = "highCutoff", .has_arg = required_argument, .val = 265, .flag = NULL },
    {0}
};

#include "macro.h"
static floatopt floatopts[10] = {0};

DSPModuleReturnCode effect_optcallback(DSPLoaderAPILogFunctionPrototype log, int optid)
{
    switch (optid)
    {
        PARSELONGFLOATOPT(256, 0, "lowGain")
        PARSELONGFLOATOPT(257, 1, "lowCutoff")
        PARSELONGFLOATOPT(258, 2, "mid1Gain")
        PARSELONGFLOATOPT(259, 3, "mid1Center")
        PARSELONGFLOATOPT(260, 4, "mid1Width")
        PARSELONGFLOATOPT(261, 5, "mid2Gain")
        PARSELONGFLOATOPT(262, 6, "mid2Center")
        PARSELONGFLOATOPT(263, 7, "mid2Width")
        PARSELONGFLOATOPT(264, 8, "highGain")
        PARSELONGFLOATOPT(265, 9, "highCutoff")
    }
    return DSPModuleReturnCode_Continue;
}

DSPModuleReturnCode effect_poststartup(DSPLoaderAPILogFunctionPrototype log, const struct json_object *configroot, bool strongconfoptcheck, ALuint effect, const char **sysname, const char **dispname)
{
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EQUALIZER);

    if (configroot)
    {
        struct json_object *jobj;
        float f;

        GETFLTCONFOPTHELPER(0, "lowGain", AL_EQUALIZER_LOW_GAIN);
        GETFLTCONFOPTHELPER(1, "lowCutoff", AL_EQUALIZER_LOW_CUTOFF);
        GETFLTCONFOPTHELPER(2, "mid1Gain", AL_EQUALIZER_MID1_GAIN);
        GETFLTCONFOPTHELPER(3, "mid1Center", AL_EQUALIZER_MID1_CENTER);
        GETFLTCONFOPTHELPER(4, "mid1Width", AL_EQUALIZER_MID1_WIDTH);
        GETFLTCONFOPTHELPER(5, "mid2Gain", AL_EQUALIZER_MID2_GAIN);
        GETFLTCONFOPTHELPER(6, "mid2Center", AL_EQUALIZER_MID2_CENTER);
        GETFLTCONFOPTHELPER(7, "mid2Width", AL_EQUALIZER_MID2_WIDTH);
        GETFLTCONFOPTHELPER(8, "highGain", AL_EQUALIZER_HIGH_GAIN);
        GETFLTCONFOPTHELPER(9, "highCutoff", AL_EQUALIZER_HIGH_CUTOFF);
    }
    
    SETEFFFLOATPROPFROMOPT(0, AL_EQUALIZER_LOW_GAIN);
    SETEFFFLOATPROPFROMOPT(1, AL_EQUALIZER_LOW_CUTOFF);
    SETEFFFLOATPROPFROMOPT(2, AL_EQUALIZER_MID1_GAIN);
    SETEFFFLOATPROPFROMOPT(3, AL_EQUALIZER_MID1_CENTER);
    SETEFFFLOATPROPFROMOPT(4, AL_EQUALIZER_MID1_WIDTH);
    SETEFFFLOATPROPFROMOPT(5, AL_EQUALIZER_MID2_GAIN);
    SETEFFFLOATPROPFROMOPT(6, AL_EQUALIZER_MID2_CENTER);
    SETEFFFLOATPROPFROMOPT(7, AL_EQUALIZER_MID2_WIDTH);
    SETEFFFLOATPROPFROMOPT(8, AL_EQUALIZER_HIGH_GAIN);
    SETEFFFLOATPROPFROMOPT(9, AL_EQUALIZER_HIGH_CUTOFF);

    *sysname = "OALequalizerEffect";
    *dispname = "OpenAL Equalizer Effect";
    return DSPModuleReturnCode_Continue;
}

void effect_printprops(DSPLoaderAPILogFunctionPrototype log, ALuint effect)
{
    float v;
    alGetEffectf(effect, AL_EQUALIZER_LOW_GAIN, &v); log(DSPLogLevel_Info, "  lowGain: %f", v);
    alGetEffectf(effect, AL_EQUALIZER_LOW_CUTOFF, &v); log(DSPLogLevel_Info, "  lowCutoff: %f", v);
    alGetEffectf(effect, AL_EQUALIZER_MID1_GAIN, &v); log(DSPLogLevel_Info, "  mid1Gain: %f", v);
    alGetEffectf(effect, AL_EQUALIZER_MID1_CENTER, &v); log(DSPLogLevel_Info, "  mid1Center: %f", v);
    alGetEffectf(effect, AL_EQUALIZER_MID1_WIDTH, &v); log(DSPLogLevel_Info, "  mid1Width: %f", v);
    alGetEffectf(effect, AL_EQUALIZER_MID2_GAIN, &v); log(DSPLogLevel_Info, "  mid2Gain: %f", v);
    alGetEffectf(effect, AL_EQUALIZER_MID2_CENTER, &v); log(DSPLogLevel_Info, "  mid2Center: %f", v);
    alGetEffectf(effect, AL_EQUALIZER_MID2_WIDTH, &v); log(DSPLogLevel_Info, "  mid2Width: %f", v);
    alGetEffectf(effect, AL_EQUALIZER_HIGH_GAIN, &v); log(DSPLogLevel_Info, "  highGain: %f", v);
    alGetEffectf(effect, AL_EQUALIZER_HIGH_CUTOFF, &v); log(DSPLogLevel_Info, "  highCutoff: %f", v);
}