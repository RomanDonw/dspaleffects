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
#include <stdbool.h>

#include <al.h>
#include <alc.h>
#include <alext.h>
#include <efx-presets.h>

#include <AL/alext.h>

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include "jsonutil.h"

const unsigned short dspmodule_requiredAPIversion = 1;

static void *inleftport, *inrightport, *outleftport, *outrightport;

static LPALCRENDERSAMPLESSOFT alcRenderSamplesSOFT;
static ALCdevice *aldev;
static ALCcontext *alctx;
#define BUFFERSCOUNT 2
static ALuint slot, buffers[BUFFERSCOUNT], source;

static float origgain = 1, reverbgain = 1;

/*
#define GETFLTVEC3CONFOPTHELPER(strname, alname) \
    if (!json_object_object_get_ex(configroot, strname, &tmp2_jobj)) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    \
    if (json_getfloat(tmp_jobj, tmp_floats)) { puts("parsing \"" strname "\" config option failed (required float)"); return 1; }\
    alEffectf(effect, alname, *tmp_floats);
*/

#define GETFLTCONFOPTHELPER(strname, alname) \
    if (!json_object_object_get_ex(configroot, strname, &tmp_jobj)) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    if (json_getfloat(tmp_jobj, tmp_floats)) { puts("parsing \"" strname "\" config option failed (required float)"); return 1; }\
    alEffectf(effect, alname, *tmp_floats);

#define GETBOOLCONFOPTHELPER(strname, alname) \
    if (!json_object_object_get_ex(configroot, strname, &tmp_jobj)) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    if (json_getbool(tmp_jobj, &tmp_bool)) { puts("parsing \"" strname "\" config option failed (required boolean)"); return 1; }\
    alEffecti(effect, alname, tmp_bool);

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    if (!alcIsExtensionPresent(NULL, "ALC_SOFT_loopback"))
    { puts("required ALC_SOFT_loopback OpenAL extension doesn't supported on this platform"); return 1; }
    if (!alcIsExtensionPresent(NULL, "ALC_EXT_EFX"))
    { puts("required ALC_EXT_EFX OpenAL extension (OpenAL EFX) doesn't supported on this platform"); return 1; }

    // ===============================
    
    const char *configfilename = NULL;
    struct json_object *configroot;
    {
        int p;
        while ((p = getopt(argc, argv, "v:V:")) != -1)
        {
            switch (p)
            {
                case 'v':
                    if (sscanf(optarg, "%f", &origgain) < 1) { puts("error parsing option -v"); return 1; }
                    origgain = clampf(origgain, 0, 1);
                    break;

                case 'V':
                    if (sscanf(optarg, "%f", &reverbgain) < 1) { puts("error parsing option -V"); return 1; }
                    reverbgain = clampf(reverbgain, 0, 1);
                    break;

                case 'f':
                {
                    FILE *f = fopen(optarg, "r");
                    if (!f) { puts("unable to open config file"); return 1; }

                    if (fseek(f, 0, SEEK_END)) { puts("failed to seek to end of config file"); goto errorquit_afterfopen; }
                    long size = ftell(f);
                    if (size < 0) { puts("failed to tell size of config file"); goto errorquit_afterfopen; }
                    if (fseek(f, 0, SEEK_SET)) { puts("failed to seek to start of config file"); goto errorquit_afterfopen; }

                    char *contents = malloc(size + 1);
                    if (!contents) { puts("memory allocation failed"); goto errorquit_afterfopen; }
                    bool success = fread(contents, size, 1, f);
                    fclose(f);
                    if (!success) { puts("failed reading config file"); goto errorquit_afteralloc; }
                    contents[size] = '\0';

                    enum json_tokener_error jerr;
                    configroot = json_tokener_parse_verbose(contents, &jerr);
                    free(contents);
                    if (jerr != json_tokener_success) { printf("JSON parsing error: %s\n", json_tokener_error_desc(jerr)); return 1; }

                    configfilename = optarg;
                    break;
                    errorquit_afteralloc:
                        free(contents);
                    errorquit_afterfopen:
                        fclose(f);
                    return 1;
                }
            }
        }
    }
    
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

    if (configfilename)
    {
        bool tmp_bool;
        float tmp_floats[3];
        struct json_object *tmp_jobj, *tmp2_jobj;

        GETFLTCONFOPTHELPER("density", AL_EAXREVERB_DENSITY);
        GETFLTCONFOPTHELPER("diffusion", AL_EAXREVERB_DIFFUSION);
        GETFLTCONFOPTHELPER("gain", AL_EAXREVERB_GAIN);
        GETFLTCONFOPTHELPER("gainHF", AL_EAXREVERB_GAINHF);
        GETFLTCONFOPTHELPER("gainLF", AL_EAXREVERB_GAINLF);
        GETFLTCONFOPTHELPER("decayTime", AL_EAXREVERB_DECAY_TIME);
        GETFLTCONFOPTHELPER("decayHFRatio", AL_EAXREVERB_DECAY_HFRATIO);
        GETFLTCONFOPTHELPER("decayLFRatio", AL_EAXREVERB_DECAY_LFRATIO);
        GETFLTCONFOPTHELPER("reflectionsDelay", AL_EAXREVERB_REFLECTIONS_DELAY);
        GETFLTCONFOPTHELPER("reflectionsGain", AL_EAXREVERB_REFLECTIONS_GAIN);
        // relf. pan.
        GETFLTCONFOPTHELPER("lateReverbDelay", AL_EAXREVERB_LATE_REVERB_DELAY);
        GETFLTCONFOPTHELPER("lateReverbGain", AL_EAXREVERB_LATE_REVERB_GAIN);
        // late reverb. pan.
        GETFLTCONFOPTHELPER("lateReverbDelay", AL_EAXREVERB_LATE_REVERB_DELAY);
        GETFLTCONFOPTHELPER("echoDepth", AL_EAXREVERB_ECHO_DEPTH);
        GETFLTCONFOPTHELPER("echoTime", AL_EAXREVERB_ECHO_TIME);
        GETFLTCONFOPTHELPER("modulationDepth", AL_EAXREVERB_MODULATION_DEPTH);
        GETFLTCONFOPTHELPER("modulationTime", AL_EAXREVERB_MODULATION_TIME);
        GETFLTCONFOPTHELPER("airAbsorptionGainHF", AL_EAXREVERB_AIR_ABSORPTION_GAINHF);
        GETFLTCONFOPTHELPER("HFReference", AL_EAXREVERB_HFREFERENCE);
        GETFLTCONFOPTHELPER("LFReference", AL_EAXREVERB_LFREFERENCE);
        GETFLTCONFOPTHELPER("roomRolloffFactor", AL_EAXREVERB_ROOM_ROLLOFF_FACTOR);
        GETBOOLCONFOPTHELPER("decayHFLimit", AL_EAXREVERB_DECAY_HFLIMIT);
    }

    // ===============================
    
    alGenAuxiliaryEffectSlots(1, &slot);
    alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, effect);
    alDeleteEffects(1, &effect);
    
    ALuint filter;
    alGenFilters(1, &filter);
    alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    alFilterf(filter, AL_LOWPASS_GAINHF, 1);
    alFilterf(filter, AL_LOWPASS_GAIN, reverbgain);

    alGenBuffers(BUFFERSCOUNT, buffers);

    alGenSources(1, &source);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(source, AL_ROLLOFF_FACTOR, 0);
    alSource3i(source, AL_AUXILIARY_SEND_FILTER, slot, 0, filter);

    alFilterf(filter, AL_LOWPASS_GAIN, origgain);
    alSourcei(source, AL_DIRECT_FILTER, filter);
    alDeleteFilters(1, &filter);
    
    printf("origgain: %f\nreverbgain: %f\n", origgain, reverbgain);
    if (configfilename) printf("configfilename: %s\n", configfilename);
    else puts("config file not specified");
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
