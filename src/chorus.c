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

#define GETFLTCONFOPTHELPER(strname, alname) \
    {\
        if (json_object_object_get_ex(configroot, strname, &jobj))\
        {\
            if (jsonutil_getfloat(jobj, vec3f)) { puts("parsing \"" strname "\" config option failed (required float)"); return 1; }\
            alEffectf(effect, alname, *vec3f);\
        }\
        else if (strongconfoptcheck) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    }

struct floatopt
{
    bool has;
    float value;
} typedef floatopt;

struct intopt
{
    bool has;
    int value;
} typedef intopt;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{   
    const char *configfilename = NULL;
    struct json_object *configroot = NULL;

    floatopt floatopts[4] = {0};
    intopt intopts[2] = {0};

    {
        int p;
        static const struct option longopts[] =
        {
            { .name = "delay", .has_arg = required_argument, .val = 256, .flag = NULL },
            { .name = "depth", .has_arg = required_argument, .val = 257, .flag = NULL },
            { .name = "feedback", .has_arg = required_argument, .val = 258, .flag = NULL },
            { .name = "rate", .has_arg = required_argument, .val = 259, .flag = NULL },
            { .name = "phase", .has_arg = required_argument, .val = 260, .flag = NULL },
            { .name = "waveform", .has_arg = required_argument, .val = 261, .flag = NULL }
        };
        while ((p = getopt_long(argc, argv, "g:G:f:a:v:A:V:is", longopts, NULL)) != -1)
        {
            switch (p)
            {
                case 'a':
                    if (sscanf(optarg, "%f", &inampmod) < 1) { puts("error parsing option -a"); return 1; }
                    break;

                case 'A':
                    if (sscanf(optarg, "%f", &outampmod) < 1) { puts("error parsing option -A"); return 1; }
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &involmod) < 1) { puts("error parsing option -v"); return 1; }
                    break;

                case 'V':
                    if (sscanf(optarg, "%f", &outvolmod) < 1) { puts("error parsing option -V"); return 1; }
                    break;

                case 'g':
                    if (sscanf(optarg, "%f", &origgain) < 1) { puts("error parsing option -g"); return 1; }
                    origgain = clampf(origgain, 0, 1);
                    break;

                case 'G':
                    if (sscanf(optarg, "%f", &effectgain) < 1) { puts("error parsing option -G"); return 1; }
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

                case 256:
                    if (sscanf(optarg, "%f", &floatopts[0].value) < 1) { puts("error parsing option --delay"); return 1; }
                    floatopts[0].has = true;
                    break;

                case 257:
                    if (sscanf(optarg, "%f", &floatopts[1].value) < 1) { puts("error parsing option --depth"); return 1; }
                    floatopts[1].has = true;
                    break;

                case 258:
                    if (sscanf(optarg, "%f", &floatopts[2].value) < 1) { puts("error parsing option --feedback"); return 1; }
                    floatopts[2].has = true;
                    break;
                
                case 259:
                    if (sscanf(optarg, "%f", &floatopts[3].value) < 1) { puts("error parsing option --rate"); return 1; }
                    floatopts[3].has = true;
                    break;

                case 260:
                    if (sscanf(optarg, "%i", &intopts[0].value) < 1) { puts("error parsing option --phase"); return 1; }
                    intopts[0].has = true;
                    break;

                case 261:
                    if (!strcmp(optarg, "sinusoid")) intopts[1].value = AL_CHORUS_WAVEFORM_SINUSOID;
                    else if (!strcmp(optarg, "triangle")) intopts[1].value = AL_CHORUS_WAVEFORM_TRIANGLE;
                    else { printf("incorrect \"--waveform\" enumeration option value (allowed: \"sinusoid\" or \"triangle\", got: \"%s\")\n", optarg); return 1; }
                    intopts[1].has = true;
                    break;
            }
        }
    }

    if (alutil_init(48000, true, false)) return 1;
    if (alutil_loadEFX()) return 1;

    // ===============================

    ALuint effect;
    alGenEffects(1, &effect);
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_CHORUS);

    if (configroot)
    {
        bool flag;
        float vec3f[3];
        struct json_object *jobj;
        
        if (!floatopts[0].has) GETFLTCONFOPTHELPER("delay", AL_CHORUS_DELAY);
        if (!floatopts[1].has) GETFLTCONFOPTHELPER("depth", AL_CHORUS_DEPTH);
        if (!floatopts[2].has) GETFLTCONFOPTHELPER("feedback", AL_CHORUS_FEEDBACK);
        if (!floatopts[3].has) GETFLTCONFOPTHELPER("rate", AL_CHORUS_RATE);

        if (!intopts[0].has)
        {
            if (json_object_object_get_ex(configroot, "phase", &jobj))
            {
                if (json_object_get_type(jobj) != json_type_int) { puts("parsing \"phase\" config option failed (required int)"); return 1; }
                alEffecti(effect, AL_CHORUS_PHASE, json_object_get_int(jobj));
            }
            else if (strongconfoptcheck) { puts("key \"phase\" doesnt found in config file"); return 1; }
        }
        
        if (!intopts[1].has)
        {
            if (json_object_object_get_ex(configroot, "waveform", &jobj))
            {
                if (json_object_get_type(jobj) != json_type_string) { puts("parsing \"waveform\" config option failed (required string)"); return 1; }
                const char *waveform = json_object_get_string(jobj);
                if (!strcmp(waveform, "sinusoid")) alEffecti(effect, AL_CHORUS_WAVEFORM, AL_CHORUS_WAVEFORM_SINUSOID);
                else if (!strcmp(waveform, "triangle")) alEffecti(effect, AL_CHORUS_WAVEFORM, AL_CHORUS_WAVEFORM_TRIANGLE);
                else { printf("incorrect \"waveform\" enumeration option value (allowed: \"sinusoid\" or \"triangle\", got: \"%s\")\n", waveform); return 1; }
            }
            else if (strongconfoptcheck) { puts("key \"waveform\" doesnt found in config file"); return 1; }
        }

        json_object_put(configroot);
    }
    if (floatopts[0].has) alEffectf(effect, AL_CHORUS_DELAY, floatopts[0].value);
    if (floatopts[1].has) alEffectf(effect, AL_CHORUS_DEPTH, floatopts[1].value);
    if (floatopts[2].has) alEffectf(effect, AL_CHORUS_FEEDBACK, floatopts[2].value);
    if (floatopts[3].has) alEffectf(effect, AL_CHORUS_RATE, floatopts[3].value);
    if (intopts[0].has) alEffecti(effect, AL_CHORUS_PHASE, intopts[0].value);
    if (intopts[1].has) alEffecti(effect, AL_CHORUS_WAVEFORM, intopts[1].value);
    
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

    *sysname = "chorus";
    *dispname = "OpenAL Chorus";
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

    union { float f; int i; } v;
    alGetEffectf(effect, AL_CHORUS_DELAY, &v.f); printf("  delay: %f\n", v.f);
    alGetEffectf(effect, AL_CHORUS_DEPTH, &v.f); printf("  depth: %f\n", v.f);
    alGetEffectf(effect, AL_CHORUS_FEEDBACK, &v.f); printf("  feedback: %f\n", v.f);
    alGetEffecti(effect, AL_CHORUS_PHASE, &v.i); printf("  phase: %i\n", v.i);
    alGetEffectf(effect, AL_CHORUS_RATE, &v.f); printf("  rate: %f\n", v.f);

    alGetEffecti(effect, AL_CHORUS_WAVEFORM, &v.i);
    printf("  waveform: ");
    switch (v.i)
    {
        case AL_CHORUS_WAVEFORM_SINUSOID:
            puts("sinusoid");
            break;

        case AL_CHORUS_WAVEFORM_TRIANGLE:
            puts("triangle");
            break;

        default:
            puts("(undefined)");
    }
}