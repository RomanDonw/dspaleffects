/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "base/base.h"

#include <stdio.h>

const struct option filter_longopts[] =
{
    { .name = "gainLF", .has_arg = required_argument, .val = 256, .flag = NULL },
    { .name = "gainHF", .has_arg = required_argument, .val = 257, .flag = NULL },
    {0}
};

#include "macro.h"
static floatopt floatopts[2] = {0};

DSPModuleReturnCode filter_optcallback(DSPLoaderAPILogFunctionPrototype log, int optid)
{
    switch (optid)
    {
        PARSELONGFLOATOPT(256, 0, "gainLF")
        PARSELONGFLOATOPT(257, 1, "gainHF")
    }
    return DSPModuleReturnCode_Continue;
}

DSPModuleReturnCode filter_poststartup(DSPLoaderAPILogFunctionPrototype log, ALuint filter, float filtergain, const char **sysname, const char **dispname)
{
    alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_BANDPASS);
    alFilterf(filter, AL_BANDPASS_GAIN, filtergain);
    
    SETFILTERFLOATPROPFROMOPT(0, AL_BANDPASS_GAINLF)
    SETFILTERFLOATPROPFROMOPT(1, AL_BANDPASS_GAINHF)

    *sysname = "OALbandpassFilter";
    *dispname = "OpenAL Bandpass Filter";
    return DSPModuleReturnCode_Continue;
}

void filter_printprops(DSPLoaderAPILogFunctionPrototype log, ALuint filter)
{
    float gainLF, gainHF;
    alGetFilterf(filter, AL_BANDPASS_GAINLF, &gainLF);
    alGetFilterf(filter, AL_BANDPASS_GAINHF, &gainHF);
    log(DSPLogLevel_Info, "  gainLF: %f\n  gainHF: %f", gainLF, gainHF);
}