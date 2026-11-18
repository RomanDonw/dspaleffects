/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "base.h"

#include <dspmodule.h>
#include <stdio.h>
#include <stdlib.h>

#include <json-c/json_tokener.h>

const unsigned short dspmodule_requiredAPIversion = 4;

static float origgain = 1, effectgain = 1, inampmod = 0, involmod = 1, outampmod = 0, outvolmod = 1;
static void *inleftport, *inrightport, *outleftport, *outrightport;
static ALuint source, buffer;
static bool allowidlerenders = false, strongconfoptcheck = true, limitoutput = false;

DSPModuleReturnCode dspmodule_startup(const DSPLoaderAPI * restrict lapi, DSPModuleStartupFunctionOptions * restrict opts)
{
    struct json_object *configroot = NULL;
    const char *configfilename = NULL;
    DSPModuleReturnCode ret;

    {
        int p;
        while ((p = getopt_long(opts->argc, opts->argv, "g:G:f:a:v:A:V:iso", effect_longopts, NULL)) != -1)
        {
            if (p > 255 || !p) { if ((ret = effect_optcallback(lapi->log, p)) != DSPModuleReturnCode_Continue) return ret; }
            else switch (p)
            {
                case 'a':
                    if (sscanf(optarg, "%f", &inampmod) < 1) goto parsefltoptfail;
                    break;

                case 'A':
                    if (sscanf(optarg, "%f", &outampmod) < 1) goto parsefltoptfail;
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &involmod) < 1) goto parsefltoptfail;
                    break;

                case 'V':
                    if (sscanf(optarg, "%f", &outvolmod) < 1) goto parsefltoptfail;
                    break;

                case 'g':
                    if (sscanf(optarg, "%f", &origgain) < 1) goto parsefltoptfail;
                    origgain = clampf(origgain, 0, 1);
                    break;

                case 'G':
                    if (sscanf(optarg, "%f", &effectgain) < 1) goto parsefltoptfail;
                    effectgain = clampf(effectgain, 0, 1);
                    break;

                case 'f':
                {
                    FILE *f = fopen(optarg, "r");
                    if (!f) { lapi->log(DSPLogLevel_Fatal, "unable to open config file"); return DSPModuleReturnCode_Stop; }

                    if (fseek(f, 0, SEEK_END)) { lapi->log(DSPLogLevel_Fatal, "failed to seek to end of config file"); goto errorquit_afterfopen; }
                    long size = ftell(f);
                    if (size < 0) { lapi->log(DSPLogLevel_Fatal, "failed to tell size of config file"); goto errorquit_afterfopen; }
                    if (fseek(f, 0, SEEK_SET)) { lapi->log(DSPLogLevel_Fatal, "failed to seek to start of config file"); goto errorquit_afterfopen; }

                    char *contents = malloc(size + 1);
                    if (!contents) { lapi->log(DSPLogLevel_Fatal, "memory allocation failed"); goto errorquit_afterfopen; }
                    bool success = fread(contents, size, 1, f);
                    fclose(f);
                    if (!success) { lapi->log(DSPLogLevel_Fatal, "failed reading config file"); goto errorquit_afteralloc; }
                    contents[size] = '\0';

                    enum json_tokener_error jerr;
                    configroot = json_tokener_parse_verbose(contents, &jerr);
                    free(contents);
                    if (jerr != json_tokener_success) { lapi->log(DSPLogLevel_Fatal, "JSON parsing error: %s", json_tokener_error_desc(jerr)); goto errorquit_afteralloc;  }

                    configfilename = optarg;
                    break;
                    errorquit_afteralloc:
                        free(contents);
                    errorquit_afterfopen:
                        fclose(f);
                    return DSPModuleReturnCode_Stop;
                }

                case 'i':
                    allowidlerenders = true;
                    break;

                case 's':
                    strongconfoptcheck = false;
                    break;

                case 'o':
                    limitoutput = true;
                    break;

                parsefltoptfail:
                    lapi->log(DSPLogLevel_Fatal, "failed parsing option -%c (required float)", (char)p);
                return DSPModuleReturnCode_Stop;
            }
        }
    }
    
    if (alutil_init(lapi->log, 48000, limitoutput, ALC_STEREO_SOFT)) return DSPModuleReturnCode_Stop;
    if (alutil_loadEFX(lapi->log)) return DSPModuleReturnCode_Stop;
    
    // ===============================

    ALuint effect;
    alGenEffects(1, &effect);
    if ((ret = effect_poststartup(lapi->log, configroot, strongconfoptcheck, effect, &opts->sysname, &opts->dispname)) != DSPModuleReturnCode_Continue)
        return ret;
    if (configroot) json_object_put(configroot);

    // ===============================

    if (!(inleftport = lapi->addport("input_left", NULL, DSPPortDirection_Input, 0)))
    { lapi->log(DSPLogLevel_Fatal, "error adding port for input left channel"); return DSPModuleReturnCode_Stop; }
    if (!(inrightport = lapi->addport("input_right", NULL, DSPPortDirection_Input, 0)))
    { lapi->log(DSPLogLevel_Fatal, "error adding port for input right channel"); return DSPModuleReturnCode_Stop; }
    
    if (!(outleftport = lapi->addport("output_left", NULL, DSPPortDirection_Output, 0)))
    { lapi->log(DSPLogLevel_Fatal, "error adding port for output left channel"); return DSPModuleReturnCode_Stop; }
    if (!(outrightport = lapi->addport("output_right", NULL, DSPPortDirection_Output, 0)))
    { lapi->log(DSPLogLevel_Fatal, "error adding port for output right channel"); return DSPModuleReturnCode_Stop; }

    // ===============================

    alGenSources(1, &source);
    
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcef(source, AL_ROLLOFF_FACTOR, 0);
    
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

    lapi->log(DSPLogLevel_Info, "inampmod: %f\ninvolmod: %f\noriggain: %f\neffectgain: %f\noutampmod: %f\noutvolmod: %f\nidlerendering: %s\nlimitoutput: %s",
        inampmod, involmod, origgain, effectgain, outampmod, outvolmod, allowidlerenders ? "allowed" : "not allowed", limitoutput ? "true" : "false");
    configfilename ? lapi->log(DSPLogLevel_Info, "configfilename: %s", configfilename) : lapi->log(DSPLogLevel_Info, "config file not specified");

    lapi->log(DSPLogLevel_Info, "effectprops:");
    effect_printprops(lapi->log, effect);
    alDeleteEffects(1, &effect);
    return 0;
}

DSPModuleReturnCode dspmodule_process(const DSPLoaderAPI * restrict lapi, DSPModuleProcessFunctionOptions * restrict opts)
{
    float *outleft = lapi->getportbuffer(outleftport, opts->duration);
    float *outright = lapi->getportbuffer(outrightport, opts->duration);
    if (!(allowidlerenders || outleft || outright)) return DSPModuleReturnCode_Continue;
    
    const float *inleft = lapi->getportbuffer(inleftport, opts->duration);
    const float *inright = lapi->getportbuffer(inrightport, opts->duration);

    // ===============================
    
    alSourceRewind(source);
    alSourcei(source, AL_BUFFER, 0);

    size_t buffsize = (opts->duration + 1) * sizeof(float) * 2;
    float buff[buffsize];
    for (size_t i = 0; i < ((size_t)opts->duration) << 1; i++)
    { buff[i] = adjf(i & 1 ? (inright ? inright[i >> 1] : 0) : (inleft ? inleft[i >> 1] : 0), inampmod) * involmod; }
    alBufferData(buffer, AL_FORMAT_STEREO_FLOAT32, buff, buffsize, opts->rate);

    alSourcei(source, AL_BUFFER, buffer);
    alSourcePlay(source);

    // ===============================

    if (alutil_render(lapi->log, buff, opts->duration, opts->rate)) return DSPModuleReturnCode_Stop;
    if (outleft || outright) for (unsigned long i = 0; i < opts->duration; i++)
    {
        if (outleft) outleft[i] = adjf(buff[i * 2], outampmod) * outvolmod;
        if (outright) outright[i] = adjf(buff[i * 2 + 1], outampmod) * outvolmod;
    }

    return 0;
}