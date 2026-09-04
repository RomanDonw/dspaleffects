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

#include <al.h>
#include <alc.h>
#include <alext.h>
#include <efx-presets.h>

const unsigned short dspmodule_requiredAPIversion = 1;

static void *inleftport, *inrightport, *outleftport, *outrightport;

static LPALCRENDERSAMPLESSOFT alcRenderSamplesSOFT;
static ALCdevice *aldev;
static ALCcontext *alctx;
#define BUFFERSCOUNT 2
static ALuint slot, buffers[BUFFERSCOUNT], source;

static float origgain = 1, reverbgain = 1;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    {
        int p;
        while ((p = getopt(argc, argv, "v:V:")) != -1)
        {
            switch (p)
            {
                case 'v':
                    if (sscanf(optarg, "%f", &origgain) < 1) { puts("error parsing option -v"); return 1; }
                    break;

                case 'V':
                    if (sscanf(optarg, "%f", &reverbgain) < 1) { puts("error parsing option -V"); return 1; }
                    break;
            }
        }
    }

    if (!alcIsExtensionPresent(NULL, "ALC_SOFT_loopback"))
    { puts("required ALC_SOFT_loopback OpenAL extension doesn't supported on this platform"); return 1; }
    if (!alcIsExtensionPresent(NULL, "ALC_EXT_EFX"))
    { puts("required ALC_EXT_EFX OpenAL extension (OpenAL EFX) doesn't supported on this platform"); return 1; }

    // ===============================

    if (!(inleftport = lapi->addport("input_left", NULL, DSPPortDirection_Input, 0)))
    { puts("error adding port for input left channel"); return 1; }
    if (!(inrightport = lapi->addport("input_right", NULL, DSPPortDirection_Input, 0)))
    { puts("error adding port for input right channel"); return 1; }

    if (!(outleftport = lapi->addport("output_left", NULL, DSPPortDirection_Output, 0)))
    { puts("error adding port for output left channel"); return 1; }
    if (!(outrightport = lapi->addport("output_right", NULL, DSPPortDirection_Output, 0)))
    { puts("error adding port for output right channel"); return 1; }

    // ===============================

    LPALCLOOPBACKOPENDEVICESOFT alcLoopbackOpenDeviceSOFT = alcGetProcAddress(NULL, "alcLoopbackOpenDeviceSOFT");
    if (!alcLoopbackOpenDeviceSOFT) { puts("failed to dynamicly load alcLoopbackOpenDeviceSOFT OpenAL function"); return 1; }
    if (!(alcRenderSamplesSOFT = alcGetProcAddress(NULL, "alcRenderSamplesSOFT")))
    { puts("failed to dynamicly load alcRenderSamplesSOFT OpenAL function"); return 1; }
    
    LPALGENEFFECTS alGenEffects = alGetProcAddress("alGenEffects");
    if (!alGenEffects) { puts("failed to load alGenEffects OpenAL function"); return 1; }
    LPALDELETEEFFECTS alDeleteEffects = alGetProcAddress("alDeleteEffects");
    if (!alDeleteEffects) { puts("failed to load alDeleteEffects OpenAL function"); return 1; }

    LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = alGetProcAddress("alGenAuxiliaryEffectSlots");
    if (!alGenAuxiliaryEffectSlots) { puts("failed to load alGenAuxiliaryEffectSlots OpenAL function"); return 1; }
    //LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = alGetProcAddress("alDeleteAuxiliaryEffectSlots");
    //if (!alDeleteAuxiliaryEffectSlots) { puts("failed to load alDeleteAuxiliaryEffectSlots OpenAL function"); return 1; }
    LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = alGetProcAddress("alAuxiliaryEffectSloti");
    if (!alAuxiliaryEffectSloti) { puts("failed to load alAuxiliaryEffectSloti OpenAL function"); return 1; }

    LPALEFFECTI alEffecti = alGetProcAddress("alEffecti");
    if (!alEffecti) { puts("failed to load alEffecti OpenAL function"); return 1; }
    LPALEFFECTF alEffectf = alGetProcAddress("alEffectf");
    if (!alEffectf) { puts("failed to load alEffectf OpenAL function"); return 1; }
    LPALEFFECTFV alEffectfv = alGetProcAddress("alEffectfv");
    if (!alEffectfv) { puts("failed to load alEffectfv OpenAL function"); return 1; }

    LPALGENFILTERS alGenFilters = alGetProcAddress("alGenFilters");
    if (!alGenFilters) { puts("failed to load alGenFilters OpenAL function"); return 1; }
    LPALDELETEFILTERS alDeleteFilters = alGetProcAddress("alDeleteFilters");
    if (!alDeleteFilters) { puts("failed to load alDeleteFilters OpenAL function"); return 1; }

    LPALFILTERI alFilteri = alGetProcAddress("alFilteri");
    if (!alFilteri) { puts("failed to load alFilteri OpenAL function"); return 1; }
    LPALFILTERF alFilterf = alGetProcAddress("alFilterf");
    if (!alFilterf) { puts("failed to load alFilterf OpenAL function"); return 1; }

    // ===============================

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

    // ===============================

    ALuint effect;
    alGenEffects(1, &effect);
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

    static const EFXEAXREVERBPROPERTIES preset = EFX_REVERB_PRESET_SEWERPIPE;
    /*
    alEffectf(effect, AL_EAXREVERB_DENSITY, 1);
    alEffectf(effect, AL_EAXREVERB_DIFFUSION, 1);
    alEffectf(effect, AL_EAXREVERB_GAIN, 1);
    alEffectf(effect, AL_EAXREVERB_GAINHF, 0.9);
    alEffectf(effect, AL_EAXREVERB_GAINLF, 0.1);
    alEffectf(effect, AL_EAXREVERB_DECAY_TIME, 3);
    alEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, 1);
    alEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, 0.2);
    alEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, preset.flReflectionsGain);
    alEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, preset.flReflectionsDelay);
    //alEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, preset.flReflectionsPan);
    alEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, 1);
    alEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, preset.flLateReverbDelay);
    //alEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, preset.flLateReverbPan);
    alEffectf(effect, AL_EAXREVERB_ECHO_TIME, preset.flEchoTime);
    alEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, preset.flEchoDepth);
    alEffectf(effect, AL_EAXREVERB_MODULATION_TIME, preset.flModulationTime);
    alEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, preset.flModulationDepth);
    alEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, preset.flAirAbsorptionGainHF);
    alEffectf(effect, AL_EAXREVERB_HFREFERENCE, preset.flHFReference);
    alEffectf(effect, AL_EAXREVERB_LFREFERENCE, preset.flLFReference);
    alEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, preset.flRoomRolloffFactor);
    alEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, preset.iDecayHFLimit);
    */

    alEffectf(effect, AL_EAXREVERB_DENSITY, preset.flDensity);
    alEffectf(effect, AL_EAXREVERB_DIFFUSION, preset.flDiffusion);
    alEffectf(effect, AL_EAXREVERB_GAIN, preset.flGain);
    alEffectf(effect, AL_EAXREVERB_GAINHF, preset.flGainHF);
    alEffectf(effect, AL_EAXREVERB_GAINLF, preset.flGainLF);
    alEffectf(effect, AL_EAXREVERB_DECAY_TIME, preset.flDecayTime);
    alEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, preset.flDecayHFRatio);
    alEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, preset.flDecayLFRatio);
    alEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, preset.flReflectionsGain);
    alEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, preset.flReflectionsDelay);
    alEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, preset.flReflectionsPan);
    alEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, preset.flLateReverbGain);
    alEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, preset.flLateReverbDelay);
    alEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, preset.flLateReverbPan);
    alEffectf(effect, AL_EAXREVERB_ECHO_TIME, preset.flEchoTime);
    alEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, preset.flEchoDepth);
    alEffectf(effect, AL_EAXREVERB_MODULATION_TIME, preset.flModulationTime);
    alEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, preset.flModulationDepth);
    alEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, preset.flAirAbsorptionGainHF);
    alEffectf(effect, AL_EAXREVERB_HFREFERENCE, preset.flHFReference);
    alEffectf(effect, AL_EAXREVERB_LFREFERENCE, preset.flLFReference);
    alEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, preset.flRoomRolloffFactor);
    alEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, preset.iDecayHFLimit);
    
    alGenAuxiliaryEffectSlots(1, &slot);
    alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, effect);
    alDeleteEffects(1, &effect);
    
    ALuint filter;
    alGenFilters(1, &filter);
    alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    alFilterf(filter, AL_LOWPASS_GAINHF, 1);
    alFilterf(filter, AL_LOWPASS_GAIN, reverbgain);

    // ===============================

    alGenBuffers(BUFFERSCOUNT, buffers);

    alGenSources(1, &source);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(source, AL_ROLLOFF_FACTOR, 0);
    alSource3i(source, AL_AUXILIARY_SEND_FILTER, slot, 0, filter);

    alFilterf(filter, AL_LOWPASS_GAIN, origgain);
    alSourcei(source, AL_DIRECT_FILTER, filter);
    alDeleteFilters(1, &filter);
    
    printf("origgain: %f\nreverbgain: %f\n", origgain, reverbgain);
    *sysname = "eaxreverb";
    *dispname = "OpenAL EAX Reverb.";
    return 0;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long duration, unsigned long rate, unsigned long long nsectime)
{
    if (rate != 48000) { puts("sample rate isn't equal to fixed 48000 Hz. stopping..."); return 1; }

    float *outleft = lapi->getportbuffer(outleftport, duration);
    float *outright = lapi->getportbuffer(outrightport, duration);
    if (!(outleft || outright)) return 0;

    const float *inleft = lapi->getportbuffer(inleftport, duration);
    const float *inright = lapi->getportbuffer(inrightport, duration);

    static float *intlvaudio = NULL;
    static size_t intlvaudiosize = 0;
    if (intlvaudiosize != duration * sizeof(float) * 2)
    {
        void *new_intlvaudio = realloc(intlvaudio, duration * sizeof(float) * 2);
        if (!new_intlvaudio) { puts("memory allocation failed"); return 1; }
        intlvaudio = new_intlvaudio;
        intlvaudiosize = duration * sizeof(float) * 2;
    }

    for (size_t i = 0; i < (size_t)duration << 1; i++)
        intlvaudio[i] = i & 1 ? (inright ? inright[i >> 1] : 0) : (inleft ? inleft[i >> 1] : 0);
    
    ALint procbuffs, queuedbuffs;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &procbuffs);
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queuedbuffs);

    ALuint emptybuff;
    while (procbuffs-- > 0)
    {
        alSourceUnqueueBuffers(source, 1, &emptybuff);
        alBufferData(emptybuff, AL_FORMAT_STEREO_FLOAT32, intlvaudio, intlvaudiosize, rate);
        alSourceQueueBuffers(source, 1, &emptybuff);
    }
    while (queuedbuffs < BUFFERSCOUNT)
    {
        emptybuff = buffers[queuedbuffs++];
        alBufferData(emptybuff, AL_FORMAT_STEREO_FLOAT32, intlvaudio, intlvaudiosize, rate);
        alSourceQueueBuffers(source, 1, &emptybuff);
    }

    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) alSourcePlay(source);

    alcRenderSamplesSOFT(aldev, intlvaudio, duration);
    
    for (size_t i = 0; i < (size_t)duration << 1; i++)
    {
        if (i & 1 && outright) outright[i >> 1] = intlvaudio[i];
        else if (outleft) outleft[i >> 1] = intlvaudio[i];
    }

    return 0;
}

void dspmodule_cleanup(void)
{
    alcMakeContextCurrent(NULL);
    alcDestroyContext(alctx);
    alcCloseDevice(aldev);
}
