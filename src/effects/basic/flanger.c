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
    { .name = "delay", .has_arg = required_argument, .val = 256, .flag = NULL },
    { .name = "depth", .has_arg = required_argument, .val = 257, .flag = NULL },
    { .name = "feedback", .has_arg = required_argument, .val = 258, .flag = NULL },
    { .name = "rate", .has_arg = required_argument, .val = 259, .flag = NULL },
    { .name = "phase", .has_arg = required_argument, .val = 260, .flag = NULL },
    { .name = "waveform", .has_arg = required_argument, .val = 261, .flag = NULL },
    {0}
};

#include "macro.h"
static floatopt floatopts[4] = {0};
static intopt intopts[2] = {0};

unsigned short effect_optcallback(DSPLoaderAPILogFunctionPrototype log, int optid)
{
    switch (optid)
    {
        PARSELONGFLOATOPT(256, 0, "delay")
        PARSELONGFLOATOPT(257, 1, "depth")
        PARSELONGFLOATOPT(258, 2, "feedback")
        PARSELONGFLOATOPT(259, 3, "rate")

        case 260:
            if (sscanf(optarg, "%i", &intopts[0].value) < 1) { log(DSPLogLevel_Fatal, "error parsing option --phase (required integer)"); return 1; }
            intopts[0].has = true;
            break;

        case 261:
            if (strfcmpl(optarg, "sinusoid")) intopts[1].value = AL_FLANGER_WAVEFORM_SINUSOID;
            else if (strfcmpl(optarg, "triangle")) intopts[1].value = AL_FLANGER_WAVEFORM_TRIANGLE;
            else
            {
                log(DSPLogLevel_Fatal, "incorrect --waveform enumeration option value (allowed: \"sinusoid\" or \"triangle\", got: \"%s\")", optarg);
                return 1;
            }
            intopts[1].has = true;
            break;    
    }
    return 0;
}

unsigned short effect_poststartup(DSPLoaderAPILogFunctionPrototype log, const struct json_object *configroot, bool strongconfoptcheck, ALuint effect, const char **sysname, const char **dispname)
{
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_FLANGER);

    if (configroot)
    {
        struct json_object *jobj;
        float f;
        
        GETFLTCONFOPTHELPER(0, "delay", AL_FLANGER_DELAY);
        GETFLTCONFOPTHELPER(1, "depth", AL_FLANGER_DEPTH);
        GETFLTCONFOPTHELPER(2, "feedback", AL_FLANGER_FEEDBACK);
        GETFLTCONFOPTHELPER(3, "rate", AL_FLANGER_RATE);

        if (!intopts[0].has)
        {
            if (json_object_object_get_ex(configroot, "phase", &jobj))
            {
                if (json_object_get_type(jobj) != json_type_int)
                { log(DSPLogLevel_Fatal, "parsing \"phase\" config option failed (required int)"); return 1; }

                alEffecti(effect, AL_FLANGER_PHASE, json_object_get_int(jobj));
            }
            else if (strongconfoptcheck) { log(DSPLogLevel_Fatal, "key \"phase\" doesnt found in config file"); return 1; }
        }

        if (!intopts[1].has)
        {
            if (json_object_object_get_ex(configroot, "waveform", &jobj))
            {
                if (json_object_get_type(jobj) != json_type_string)
                { log(DSPLogLevel_Fatal, "parsing \"waveform\" config option failed (required string)"); return 1; }

                const char *waveform = json_object_get_string(jobj);
                if (strfcmpl(waveform, "sinusoid")) alEffecti(effect, AL_FLANGER_WAVEFORM, AL_FLANGER_WAVEFORM_SINUSOID);
                else if (strfcmpl(waveform, "triangle")) alEffecti(effect, AL_FLANGER_WAVEFORM, AL_FLANGER_WAVEFORM_TRIANGLE);
                else
                {
                    log(DSPLogLevel_Fatal, "incorrect \"waveform\" enumeration option value (allowed: \"sinusoid\" or \"triangle\", got: \"%s\")",
                        waveform);
                    return 1;
                }
            }
            else if (strongconfoptcheck) { log(DSPLogLevel_Fatal, "key \"waveform\" doesnt found in config file"); return 1; }
        }
    }

    SETEFFFLOATPROPFROMOPT(0, AL_FLANGER_DELAY);
    SETEFFFLOATPROPFROMOPT(1, AL_FLANGER_DEPTH);
    SETEFFFLOATPROPFROMOPT(2, AL_FLANGER_FEEDBACK);
    SETEFFFLOATPROPFROMOPT(3, AL_FLANGER_RATE);
    
    SETEFFINTPROPFROMOPT(0, AL_FLANGER_PHASE);
    SETEFFINTPROPFROMOPT(1, AL_FLANGER_WAVEFORM);

    *sysname = "OALflangerEffect";
    *dispname = "OpenAL Flanger Effect";
    return 0;
}

void effect_printprops(DSPLoaderAPILogFunctionPrototype log, ALuint effect)
{
    union { float f; int i; } v;
    alGetEffectf(effect, AL_FLANGER_DELAY, &v.f); log(DSPLogLevel_Info, "  delay: %f", v.f);
    alGetEffectf(effect, AL_FLANGER_DEPTH, &v.f); log(DSPLogLevel_Info, "  depth: %f", v.f);
    alGetEffectf(effect, AL_FLANGER_FEEDBACK, &v.f); log(DSPLogLevel_Info, "  feedback: %f", v.f);
    alGetEffecti(effect, AL_FLANGER_PHASE, &v.i); log(DSPLogLevel_Info, "  phase: %i", v.i);
    alGetEffectf(effect, AL_FLANGER_RATE, &v.f); log(DSPLogLevel_Info, "  rate: %f", v.f);

    alGetEffecti(effect, AL_FLANGER_WAVEFORM, &v.i);
    const char *waveformstr;
    switch (v.i)
    {
        case AL_FLANGER_WAVEFORM_SINUSOID:
            waveformstr = "sinusoid";
            break;

        case AL_FLANGER_WAVEFORM_TRIANGLE:
            waveformstr = "triangle";
            break;

        default:
            waveformstr = "(undefined)";
    }
    log(DSPLogLevel_Info, "  waveform: %s", waveformstr);
}