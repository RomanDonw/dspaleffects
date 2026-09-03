/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include <dspmodule.h>

#include <stddef.h>
#include <stdio.h>

#include <al.h>
#include <alc.h>
#include <alext.h>

const unsigned short dspmodule_requiredAPIversion = 1;

static LPALCRENDERSAMPLESSOFT alcRenderSamplesSOFT;
static ALCdevice *aldev;
static ALCcontext *alctx;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    if (!alcIsExtensionPresent(NULL, "ALC_SOFT_loopback"))
    { puts("required ALC_SOFT_loopback OpenAL extension doesn't supported on this platform"); return 1; }

    LPALCLOOPBACKOPENDEVICESOFT alcLoopbackOpenDeviceSOFT = alcGetProcAddress(NULL, "alcLoopbackOpenDeviceSOFT");
    if (!alcLoopbackOpenDeviceSOFT) { puts("failed to dynamicly load alcLoopbackOpenDeviceSOFT OpenAL function"); return 1; }
    if (!(alcRenderSamplesSOFT = alcGetProcAddress(NULL, "alcRenderSamplesSOFT")))
    { puts("failed to dynamicly load alcRenderSamplesSOFT OpenAL function"); return 1; }
    
    if (!(aldev = alcLoopbackOpenDeviceSOFT(NULL))) { puts("error creating/opening OpenAL loopback device"); return 1; }

    ALCint attrs[] =
    {
        ALC_FORMAT_CHANNELS_SOFT, ALC_STEREO_SOFT,
        ALC_FORMAT_TYPE_SOFT, ALC_FLOAT_SOFT,
        ALC_FREQUENCY, 48000,
        0
    };
    if (!(alctx = alcCreateContext(aldev, attrs))) { puts("error creating OpenAL context for loopback device"); return 1; }
    alcMakeContextCurrent(alctx);

    

    return 1;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long duration, unsigned long rate, unsigned long long nsectime)
{
    return 1;
}

void dspmodule_cleanup(void)
{
    alcMakeContextCurrent(NULL);
    alcDestroyContext(alctx);
    alcCloseDevice(aldev);
}
