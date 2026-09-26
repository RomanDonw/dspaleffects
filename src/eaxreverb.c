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
#include <string.h>

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include "alutil/general.h"
#include "alutil/EFX.h"
#include "jsonutil/jsonutil.h"

const unsigned short dspmodule_requiredAPIversion = 1;

static float origgain = 1, effectgain = 1, inampmod = 0, involmod = 1, outampmod = 0, outvolmod = 1;
static void *inleftport, *inrightport, *outleftport, *outrightport;
static ALuint source, buffer;
static bool allowidlerenders = false, strongconfoptcheck = true;

static void printeffectprops(ALuint effect);

struct floatopt { bool has; float value; } typedef floatopt;
struct intopt { bool has; int value; } typedef intopt;
struct vec3opt { bool has; float value[3]; } typedef vec3opt;

#define GETFLTVEC3CONFOPTHELPER(vec3optsidx, strname, alname) \
    if (!vec3opts[vec3optsidx].has)\
    {\
        if (json_object_object_get_ex(configroot, strname, &jobj))\
        {\
            if (jsonutil_getvec3f(jobj, vec3f)) { puts("failed parsing option \"" strname "\" (required vector/array of 3 floats)"); return 1; }\
            alEffectfv(effect, alname, vec3f);\
        }\
        else if (strongconfoptcheck) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    }

#define GETFLTCONFOPTHELPER(floatoptsidx, strname, alname) \
    if (!floatopts[floatoptsidx].has)\
    {\
        if (json_object_object_get_ex(configroot, strname, &jobj))\
        {\
            if (jsonutil_getfloat(jobj, vec3f)) { puts("parsing \"" strname "\" config option failed (required float)"); return 1; }\
            alEffectf(effect, alname, *vec3f);\
        }\
        else if (strongconfoptcheck) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    }

#define GETBOOLCONFOPTHELPER(intoptsidx, strname, alname) \
    if (!intopts[intoptsidx].has)\
    {\
        if (json_object_object_get_ex(configroot, strname, &jobj))\
        {\
            if (jsonutil_getbool(jobj, &flag)) { puts("parsing \"" strname "\" config option failed (required boolean)"); return 1; }\
            alEffecti(effect, alname, flag);\
        }\
        else if (strongconfoptcheck) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    }

#define PARSELONGFLOATOPT(optid, optindex, optname) \
    case optid:\
        if (sscanf(optarg, "%f", &floatopts[optindex].value) < 1) { puts("error parsing option " optname " (required float)"); return 1; }\
        floatopts[optindex].has = true;\
        break;

#define PARSELONGVEC3OPT(optid, optindex, optname) \
    case optid:\
        if (sscanf(optarg, "%f,%f,%f", &vec3opts[optindex].value[0], &vec3opts[optindex].value[1], &vec3opts[optindex].value[2]) < 3)\
        { puts("error parsing option " optname " (required 3D vector - \"float, float, float\")"); return 1; }\
        vec3opts[optindex].has = true;\
        break;

#define PARSELONGBOOLOPT(optid, optindex, optname) \
    case optid:\
        if (sscanf(optarg, "%i", &intopts[optindex].value) >= 1);\
        else if (!strcmp(optarg, "true") || !strcmp(optarg, "on") || !strcmp(optarg, "enable") || !strcmp(optarg, "yes"))\
            intopts[optindex].value = true;\
        else if (!strcmp(optarg, "false") || !strcmp(optarg, "off") || !strcmp(optarg, "disable") || !strcmp(optarg, "no"))\
            intopts[optindex].value = false;\
        else\
        {\
            puts("error parsing option " optname " (required boolean, allowed values: integer (where 0 - disable, other - enable), "\
                "\"true\"/\"on\"/\"enable\"/\"yes\" or \"false\"/\"off\"/\"disable\"/no\")");\
            return 1;\
        }\
        intopts[optindex].has = true;\
        break;

#define SETEFFFLOATPROPFROMOPT(optindex, alname) \
    if (floatopts[optindex].has) alEffectf(effect, alname, floatopts[optindex].value);
#define SETEFFVEC3PROPFROMOPT(optindex, alname) \
    if (vec3opts[optindex].has) alEffectfv(effect, alname, vec3opts[optindex].value);
#define SETEFFINTPROPFROMOPT(optindex, alname) \
    if (intopts[optindex].has) alEffecti(effect, alname, intopts[optindex].value);
    
unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{   
    const char *configfilename = NULL;
    struct json_object *configroot = NULL;

    floatopt floatopts[20] = {0};
    intopt intopts[1] = {0};
    vec3opt vec3opts[2] = {0};

    {
        int p;
        static const struct option longopts[] =
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
        };
        while ((p = getopt_long(argc, argv, "g:G:f:a:v:A:V:is", longopts, NULL)) != -1)
        {
            switch (p)
            {
                case 'a':
                    if (sscanf(optarg, "%f", &inampmod) < 1) { puts("error parsing option -a (required float)"); return 1; }
                    break;

                case 'A':
                    if (sscanf(optarg, "%f", &outampmod) < 1) { puts("error parsing option -A (required float)"); return 1; }
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &involmod) < 1) { puts("error parsing option -v (required float)"); return 1; }
                    break;

                case 'V':
                    if (sscanf(optarg, "%f", &outvolmod) < 1) { puts("error parsing option -V (required float)"); return 1; }
                    break;

                case 'g':
                    if (sscanf(optarg, "%f", &origgain) < 1) { puts("error parsing option -g (required float)"); return 1; }
                    origgain = clampf(origgain, 0, 1);
                    break;

                case 'G':
                    if (sscanf(optarg, "%f", &effectgain) < 1) { puts("error parsing option -G (required float)"); return 1; }
                    effectgain = clampf(effectgain, 0, 1);
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
                    if (jerr != json_tokener_success) { printf("JSON parsing error: %s\n", json_tokener_error_desc(jerr)); goto errorquit_afteralloc;  }

                    configfilename = optarg;
                    break;
                    errorquit_afteralloc:
                        free(contents);
                    errorquit_afterfopen:
                        fclose(f);
                    return 1;
                }

                case 'i':
                    allowidlerenders = true;
                    break;

                case 's':
                    strongconfoptcheck = false;
                    break;

                /*
                    ==========================
                    ====== LONG OPTIONS ======
                    ===========================
                */

                PARSELONGFLOATOPT(256, 0, "--density")
                PARSELONGFLOATOPT(257, 1, "--diffusion")
                PARSELONGFLOATOPT(258, 2, "--gain")
                PARSELONGFLOATOPT(259, 3, "--gainHF")
                PARSELONGFLOATOPT(260, 4, "--gainLF")
                PARSELONGFLOATOPT(261, 5, "--decayTime")
                PARSELONGFLOATOPT(262, 6, "--decayHFRatio")
                PARSELONGFLOATOPT(263, 7, "--decayLFRatio")
                PARSELONGFLOATOPT(264, 8, "--reflectionsGain")
                PARSELONGFLOATOPT(265, 9, "--reflectionsDelay")
                PARSELONGFLOATOPT(266, 10, "--lateReverbGain")
                PARSELONGFLOATOPT(267, 11, "--lateReverbDelay")
                PARSELONGFLOATOPT(268, 12, "--echoTime")
                PARSELONGFLOATOPT(269, 13, "--echoDepth")
                PARSELONGFLOATOPT(270, 14, "--modulationTime")
                PARSELONGFLOATOPT(271, 15, "--modulationDepth")
                PARSELONGFLOATOPT(272, 16, "--airAbsorptionGainHF")
                PARSELONGFLOATOPT(273, 17, "--HFReference")
                PARSELONGFLOATOPT(274, 18, "--LFReference")
                PARSELONGFLOATOPT(275, 19, "--roomRolloffFactor")

                PARSELONGBOOLOPT(276, 0, "--decayHFLimit")
                
                PARSELONGVEC3OPT(277, 0, "--reflectionsPan")
                PARSELONGVEC3OPT(278, 1, "--lateReverbPan")
            }
        }
    }

    if (alutil_init(48000, true, false)) return 1;
    if (alutil_loadEFX()) return 1;

    // ===============================

    ALuint effect;
    alGenEffects(1, &effect);
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

    if (configroot)
    {
        bool flag;
        float vec3f[3];
        struct json_object *jobj;

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
        GETFLTCONFOPTHELPER(12, "echoDepth", AL_EAXREVERB_ECHO_DEPTH);
        GETFLTCONFOPTHELPER(13, "echoTime", AL_EAXREVERB_ECHO_TIME);
        GETFLTCONFOPTHELPER(14, "modulationDepth", AL_EAXREVERB_MODULATION_DEPTH);
        GETFLTCONFOPTHELPER(15, "modulationTime", AL_EAXREVERB_MODULATION_TIME);
        GETFLTCONFOPTHELPER(16, "airAbsorptionGainHF", AL_EAXREVERB_AIR_ABSORPTION_GAINHF);
        GETFLTCONFOPTHELPER(17, "HFReference", AL_EAXREVERB_HFREFERENCE);
        GETFLTCONFOPTHELPER(18, "LFReference", AL_EAXREVERB_LFREFERENCE);
        GETFLTCONFOPTHELPER(19, "roomRolloffFactor", AL_EAXREVERB_ROOM_ROLLOFF_FACTOR);
        GETBOOLCONFOPTHELPER(0, "decayHFLimit", AL_EAXREVERB_DECAY_HFLIMIT);

        json_object_put(configroot);
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

    alGenSources(1, &source);
    
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(source, AL_ROLLOFF_FACTOR, 0);
    
    ALuint filter;
    alGenFilters(1, &filter);
    alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    alFilterf(filter, AL_LOWPASS_GAINHF, 1);
    alFilterf(filter, AL_LOWPASS_GAIN, origgain);
    alSourcei(source, AL_DIRECT_FILTER, filter);
    alDeleteFilters(1, &filter);
    
    // ===============================
    
    ALuint slot;
    alGenAuxiliaryEffectSlots(1, &slot);
    alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, effect);
    alAuxiliaryEffectSlotf(slot, AL_EFFECTSLOT_GAIN, effectgain);
    alSource3i(source, AL_AUXILIARY_SEND_FILTER, slot, 0, AL_FILTER_NULL);
    
    // ===============================

    alGenBuffers(1, &buffer);

    // ===============================

    printf("inampmod: %f\ninvolmod: %f\noriggain: %f\neffectgain: %f\noutampmod: %f\noutvolmod: %f\nidle renders: %s\n",
        inampmod, involmod, origgain, effectgain, outampmod, outvolmod, allowidlerenders ? "allowed" : "not allowed");
    configfilename ? printf("configfilename: %s\n", configfilename) : puts("config file not specified");

    printeffectprops(effect);
    alDeleteEffects(1, &effect);

    *sysname = "eaxreverb";
    *dispname = "OpenAL EAX Reverb.";
    return 0;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long duration, unsigned long rate, unsigned long long nsectime)
{
    float *outleft = lapi->getportbuffer(outleftport, duration);
    float *outright = lapi->getportbuffer(outrightport, duration);
    if (!(allowidlerenders || outleft || outright)) return 0;
    
    const float *inleft = lapi->getportbuffer(inleftport, duration);
    const float *inright = lapi->getportbuffer(inrightport, duration);

    // ===============================
    
    alSourceRewind(source);
    alSourcei(source, AL_BUFFER, 0);

    size_t buffsize = (duration + 1) * sizeof(float) * 2;
    float buff[buffsize];
    for (size_t i = 0; i < ((size_t)duration) << 1; i++)
    { buff[i] = i & 1 ? (inright ? inright[i >> 1] : 0) : (inleft ? inleft[i >> 1] : 0); }
    alBufferData(buffer, AL_FORMAT_STEREO_FLOAT32, buff, buffsize, rate);

    alSourcei(source, AL_BUFFER, buffer);
    alSourcePlay(source);

    // ===============================

    if (alutil_render(buff, duration, rate)) return 1;
    if (outleft || outright) for (unsigned long i = 0; i < duration; i++)
    {
        if (outleft) outleft[i] = buff[i * 2];
        if (outright) outright[i] = buff[i * 2 + 1];
    }

    return 0;
}

static void printeffectprops(ALuint effect)
{
    puts("effectprops:");

    union { float f3[3]; int i; } v;
    alGetEffectf(effect, AL_EAXREVERB_DENSITY, v.f3); printf("  density: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_DIFFUSION, v.f3); printf("  diffusion: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_GAIN, v.f3); printf("  gain: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_GAINHF, v.f3); printf("  gainHF: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_GAINLF, v.f3); printf("  gainLF: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_DECAY_TIME, v.f3); printf("  decayTime: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, v.f3); printf("  decayHFRatio: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, v.f3); printf("  decayLFRatio: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, v.f3); printf("  reflectionsGain: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, v.f3); printf("  reflectionsDelay: %f\n", *v.f3);
    alGetEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, v.f3); printf("  reflectionsPan: [%f, %f, %f]\n", v.f3[0], v.f3[1], v.f3[2]);
    alGetEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, v.f3); printf("  lateReverbGain: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, v.f3); printf("  lateReverbDelay: %f\n", *v.f3);
    alGetEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, v.f3); printf("  lateReverbPan: [%f, %f, %f]\n", v.f3[0], v.f3[1], v.f3[2]);
    alGetEffectf(effect, AL_EAXREVERB_ECHO_TIME, v.f3); printf("  echoTime: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, v.f3); printf("  echoDepth: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_MODULATION_TIME, v.f3); printf("  modulationTime: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, v.f3); printf("  modulationDepth: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, v.f3); printf("  airAbsorptionGainHF: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_HFREFERENCE, v.f3); printf("  HFReference: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_LFREFERENCE, v.f3); printf("  LFReference: %f\n", *v.f3);
    alGetEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, v.f3); printf("  roomRolloffFactor: %f\n", *v.f3);
    alGetEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, &v.i); printf("  decayHFLimit: %s\n", v.i ? "true" : "false");
}