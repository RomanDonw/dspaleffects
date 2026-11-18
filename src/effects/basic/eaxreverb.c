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
    { .name = "density", .has_arg = required_argument, .val = 256, .flag = NULL },
    { .name = "diffusion", .has_arg = required_argument, .val = 257, .flag = NULL },
    { .name = "gain", .has_arg = required_argument, .val = 258, .flag = NULL },
    { .name = "gainHF", .has_arg = required_argument, .val = 259, .flag = NULL },
    { .name = "gainLF", .has_arg = required_argument, .val = 260, .flag = NULL },
    { .name = "decayTime", .has_arg = required_argument, .val = 261, .flag = NULL },
    { .name = "decayHFRatio", .has_arg = required_argument, .val = 262, .flag = NULL },
    { .name = "decayLFRatio", .has_arg = required_argument, .val = 263, .flag = NULL },
    { .name = "reflectionsGain", .has_arg = required_argument, .val = 264, .flag = NULL },
    { .name = "reflectionsDelay", .has_arg = required_argument, .val = 265, .flag = NULL },
    { .name = "lateReverbGain", .has_arg = required_argument, .val = 266, .flag = NULL },
    { .name = "lateReverbDelay", .has_arg = required_argument, .val = 267, .flag = NULL },
    { .name = "echoTime", .has_arg = required_argument, .val = 268, .flag = NULL },
    { .name = "echoDepth", .has_arg = required_argument, .val = 269, .flag = NULL },
    { .name = "modulationTime", .has_arg = required_argument, .val = 270, .flag = NULL },
    { .name = "modulationDepth", .has_arg = required_argument, .val = 271, .flag = NULL },
    { .name = "airAbsorptionGainHF", .has_arg = required_argument, .val = 272, .flag = NULL },
    { .name = "HFReference", .has_arg = required_argument, .val = 273, .flag = NULL },
    { .name = "LFReference", .has_arg = required_argument, .val = 274, .flag = NULL },
    { .name = "roomRolloffFactor", .has_arg = required_argument, .val = 275, .flag = NULL },
    
    { .name = "decayHFLimit", .has_arg = required_argument, .val = 276, .flag = NULL },
    
    { .name = "reflectionsPan", .has_arg = required_argument, .val = 277, .flag = NULL },
    { .name = "lateReverbPan", .has_arg = required_argument, .val = 278, .flag = NULL },
    {0}
};

#define MACRO_USEFLTARRAY
#include "macro.h"
static floatopt floatopts[20] = {0};
static intopt intopts[1] = {0};
static vec3opt vec3opts[2] = {0};

DSPModuleReturnCode effect_optcallback(DSPLoaderAPILogFunctionPrototype log, int optid)
{
    switch (optid)
    {
        PARSELONGFLOATOPT(256, 0, "density")
        PARSELONGFLOATOPT(257, 1, "diffusion")
        PARSELONGFLOATOPT(258, 2, "gain")
        PARSELONGFLOATOPT(259, 3, "gainHF")
        PARSELONGFLOATOPT(260, 4, "gainLF")
        PARSELONGFLOATOPT(261, 5, "decayTime")
        PARSELONGFLOATOPT(262, 6, "decayHFRatio")
        PARSELONGFLOATOPT(263, 7, "decayLFRatio")
        PARSELONGFLOATOPT(264, 8, "reflectionsGain")
        PARSELONGFLOATOPT(265, 9, "reflectionsDelay")
        PARSELONGFLOATOPT(266, 10, "lateReverbGain")
        PARSELONGFLOATOPT(267, 11, "lateReverbDelay")
        PARSELONGFLOATOPT(268, 12, "echoTime")
        PARSELONGFLOATOPT(269, 13, "echoDepth")
        PARSELONGFLOATOPT(270, 14, "modulationTime")
        PARSELONGFLOATOPT(271, 15, "modulationDepth")
        PARSELONGFLOATOPT(272, 16, "airAbsorptionGainHF")
        PARSELONGFLOATOPT(273, 17, "HFReference")
        PARSELONGFLOATOPT(274, 18, "LFReference")
        PARSELONGFLOATOPT(275, 19, "roomRolloffFactor")

        PARSELONGBOOLOPT(276, 0, "decayHFLimit")
        
        PARSELONGVEC3OPT(277, 0, "reflectionsPan")
        PARSELONGVEC3OPT(278, 1, "lateReverbPan")
    }
    return DSPModuleReturnCode_Continue;
}

DSPModuleReturnCode effect_poststartup(DSPLoaderAPILogFunctionPrototype log, const struct json_object *configroot, bool strongconfoptcheck, ALuint effect, const char **sysname, const char **dispname)
{
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

    if (configroot)
    {
        struct json_object *jobj;
        float vec3f[3];
        bool flag;

        GETFLTCONFOPTHELPER(0, "density", AL_EAXREVERB_DENSITY);
        GETFLTCONFOPTHELPER(1, "diffusion", AL_EAXREVERB_DIFFUSION);
        GETFLTCONFOPTHELPER(2, "gain", AL_EAXREVERB_GAIN);
        GETFLTCONFOPTHELPER(3, "gainHF", AL_EAXREVERB_GAINHF);
        GETFLTCONFOPTHELPER(4, "gainLF", AL_EAXREVERB_GAINLF);
        GETFLTCONFOPTHELPER(5, "decayTime", AL_EAXREVERB_DECAY_TIME);
        GETFLTCONFOPTHELPER(6, "decayHFRatio", AL_EAXREVERB_DECAY_HFRATIO);
        GETFLTCONFOPTHELPER(7, "decayLFRatio", AL_EAXREVERB_DECAY_LFRATIO);
        GETFLTCONFOPTHELPER(8, "reflectionsGain", AL_EAXREVERB_REFLECTIONS_GAIN);
        GETFLTCONFOPTHELPER(9, "reflectionsDelay", AL_EAXREVERB_REFLECTIONS_DELAY);
        GETFLTVEC3CONFOPTHELPER(0, "reflectionsPan", AL_EAXREVERB_REFLECTIONS_PAN);
        GETFLTCONFOPTHELPER(10, "lateReverbGain", AL_EAXREVERB_LATE_REVERB_GAIN);
        GETFLTCONFOPTHELPER(11, "lateReverbDelay", AL_EAXREVERB_LATE_REVERB_DELAY);
        GETFLTVEC3CONFOPTHELPER(1, "lateReverbPan", AL_EAXREVERB_LATE_REVERB_PAN);
        GETFLTCONFOPTHELPER(12, "echoTime", AL_EAXREVERB_ECHO_TIME);
        GETFLTCONFOPTHELPER(13, "echoDepth", AL_EAXREVERB_ECHO_DEPTH);
        GETFLTCONFOPTHELPER(14, "modulationTime", AL_EAXREVERB_MODULATION_TIME);
        GETFLTCONFOPTHELPER(15, "modulationDepth", AL_EAXREVERB_MODULATION_DEPTH);
        GETFLTCONFOPTHELPER(16, "airAbsorptionGainHF", AL_EAXREVERB_AIR_ABSORPTION_GAINHF);
        GETFLTCONFOPTHELPER(17, "HFReference", AL_EAXREVERB_HFREFERENCE);
        GETFLTCONFOPTHELPER(18, "LFReference", AL_EAXREVERB_LFREFERENCE);
        GETFLTCONFOPTHELPER(19, "roomRolloffFactor", AL_EAXREVERB_ROOM_ROLLOFF_FACTOR);
        GETBOOLCONFOPTHELPER(0, "decayHFLimit", AL_EAXREVERB_DECAY_HFLIMIT);
    }
    
    SETEFFFLOATPROPFROMOPT(0, AL_EAXREVERB_DENSITY);
    SETEFFFLOATPROPFROMOPT(1, AL_EAXREVERB_DIFFUSION);
    SETEFFFLOATPROPFROMOPT(2, AL_EAXREVERB_GAIN);
    SETEFFFLOATPROPFROMOPT(3, AL_EAXREVERB_GAINHF);
    SETEFFFLOATPROPFROMOPT(4, AL_EAXREVERB_GAINLF);
    SETEFFFLOATPROPFROMOPT(5, AL_EAXREVERB_DECAY_TIME);
    SETEFFFLOATPROPFROMOPT(6, AL_EAXREVERB_DECAY_HFRATIO);
    SETEFFFLOATPROPFROMOPT(7, AL_EAXREVERB_DECAY_LFRATIO);
    SETEFFFLOATPROPFROMOPT(8, AL_EAXREVERB_REFLECTIONS_GAIN);
    SETEFFFLOATPROPFROMOPT(9, AL_EAXREVERB_REFLECTIONS_DELAY);
    SETEFFFLOATPROPFROMOPT(10, AL_EAXREVERB_LATE_REVERB_GAIN);
    SETEFFFLOATPROPFROMOPT(11, AL_EAXREVERB_LATE_REVERB_DELAY);
    SETEFFFLOATPROPFROMOPT(12, AL_EAXREVERB_ECHO_TIME);
    SETEFFFLOATPROPFROMOPT(13, AL_EAXREVERB_ECHO_DEPTH);
    SETEFFFLOATPROPFROMOPT(14, AL_EAXREVERB_MODULATION_TIME);
    SETEFFFLOATPROPFROMOPT(15, AL_EAXREVERB_MODULATION_DEPTH);
    SETEFFFLOATPROPFROMOPT(16, AL_EAXREVERB_AIR_ABSORPTION_GAINHF);
    SETEFFFLOATPROPFROMOPT(17, AL_EAXREVERB_HFREFERENCE);
    SETEFFFLOATPROPFROMOPT(18, AL_EAXREVERB_LFREFERENCE);
    SETEFFFLOATPROPFROMOPT(19, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR);

    SETEFFINTPROPFROMOPT(0, AL_EAXREVERB_DECAY_HFLIMIT);

    SETEFFVEC3PROPFROMOPT(0, AL_EAXREVERB_REFLECTIONS_PAN);
    SETEFFVEC3PROPFROMOPT(1, AL_EAXREVERB_LATE_REVERB_PAN);

    *sysname = "OALeaxreverbEffect";
    *dispname = "OpenAL EAX Reverb. Effect";
    return DSPModuleReturnCode_Continue;
}

void effect_printprops(DSPLoaderAPILogFunctionPrototype log, ALuint effect)
{
    union { float f3[3]; int i; } v;
    alGetEffectf(effect, AL_EAXREVERB_DENSITY, v.f3); log(DSPLogLevel_Info, "  density: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_DIFFUSION, v.f3); log(DSPLogLevel_Info, "  diffusion: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_GAIN, v.f3); log(DSPLogLevel_Info, "  gain: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_GAINHF, v.f3); log(DSPLogLevel_Info, "  gainHF: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_GAINLF, v.f3); log(DSPLogLevel_Info, "  gainLF: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_DECAY_TIME, v.f3); log(DSPLogLevel_Info, "  decayTime: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, v.f3); log(DSPLogLevel_Info, "  decayHFRatio: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, v.f3); log(DSPLogLevel_Info, "  decayLFRatio: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, v.f3); log(DSPLogLevel_Info, "  reflectionsGain: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, v.f3); log(DSPLogLevel_Info, "  reflectionsDelay: %f", *v.f3);
    alGetEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, v.f3); log(DSPLogLevel_Info, "  reflectionsPan: [%f, %f, %f]", v.f3[0], v.f3[1], v.f3[2]);
    alGetEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, v.f3); log(DSPLogLevel_Info, "  lateReverbGain: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, v.f3); log(DSPLogLevel_Info, "  lateReverbDelay: %f", *v.f3);
    alGetEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, v.f3); log(DSPLogLevel_Info, "  lateReverbPan: [%f, %f, %f]", v.f3[0], v.f3[1], v.f3[2]);
    alGetEffectf(effect, AL_EAXREVERB_ECHO_TIME, v.f3); log(DSPLogLevel_Info, "  echoTime: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, v.f3); log(DSPLogLevel_Info, "  echoDepth: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_MODULATION_TIME, v.f3); log(DSPLogLevel_Info, "  modulationTime: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, v.f3); log(DSPLogLevel_Info, "  modulationDepth: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, v.f3); log(DSPLogLevel_Info, "  airAbsorptionGainHF: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_HFREFERENCE, v.f3); log(DSPLogLevel_Info, "  HFReference: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_LFREFERENCE, v.f3); log(DSPLogLevel_Info, "  LFReference: %f", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, v.f3); log(DSPLogLevel_Info, "  roomRolloffFactor: %f", *v.f3);
    alGetEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, &v.i); log(DSPLogLevel_Info, "  decayHFLimit: %s", v.i ? "true" : "false");
}