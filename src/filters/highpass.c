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
    {0}
};

#include "macro.h"
static floatopt floatopts[1] = {0};

unsigned short filter_optcallback(int optid)
{
    switch (optid)
    {
        PARSELONGFLOATOPT(256, 0, "gainLF")
    }
    return 0;
}

unsigned short filter_poststartup(ALuint filter, float filtergain, const char **sysname, const char **dispname)
{
    alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_HIGHPASS);
    alFilterf(filter, AL_HIGHPASS_GAIN, filtergain);
    
    SETFILTERFLOATPROPFROMOPT(0, AL_HIGHPASS_GAINLF)

    *sysname = "OALhighpassFilter";
    *dispname = "OpenAL Highpass Filter";
    return 0;
}

void filter_printprops(ALuint filter)
{
    float v;
    alGetFilterf(filter, AL_HIGHPASS_GAINLF, &v);
    printf("  gainLF: %f\n", v);
}