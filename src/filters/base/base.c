/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "base.h"

#include <dspmodule.h>
#include <stdio.h>
#include <stdlib.h>

const unsigned short dspmodule_requiredAPIversion = 4;

static float origgain = 1, filtergain = 1, inampmod = 0, involmod = 1, outampmod = 0, outvolmod = 1;
static void *inleftport, *inrightport, *outleftport, *outrightport;
static ALuint source, buffer;
static bool allowidlerenders = false, limitoutput = false;

DSPModuleReturnCode dspmodule_startup(const DSPLoaderAPI * restrict lapi, DSPModuleStartupFunctionOptions * restrict opts)
{
    DSPModuleReturnCode ret;

    {
        int p;
        while ((p = getopt_long(opts->argc, opts->argv, "g:G:a:v:A:V:io", filter_longopts, NULL)) != -1)
        {
            if (p > 255 || !p) { if ((ret = filter_optcallback(lapi->log, p)) != DSPModuleReturnCode_Continue) return ret; }
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
                    if (sscanf(optarg, "%f", &filtergain) < 1) goto parsefltoptfail;
                    filtergain = clampf(filtergain, 0, 1);
                    break;

                case 'i':
                    allowidlerenders = true;
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
    
    if (alutil_init(lapi->log, 48000, limitoutput)) return DSPModuleReturnCode_Stop;
    if (alutil_loadEFX(lapi->log)) return DSPModuleReturnCode_Stop;
    
    // ===============================

    ALuint filter;
    alGenFilters(1, &filter);
    if ((ret = filter_poststartup(lapi->log, filter, filtergain, &opts->sysname, &opts->dispname)) != DSPModuleReturnCode_Continue) return ret;

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
    alSourcei(source, AL_DIRECT_FILTER, filter);
    
    // ===============================

    alGenBuffers(1, &buffer);

    // ===============================

    lapi->log(DSPLogLevel_Info, "inampmod: %f\ninvolmod: %f\noriggain: %f\nfiltergain: %f\noutampmod: %f\noutvolmod: %f\nidlerendering: %s\nlimitoutput: %s",
        inampmod, involmod, origgain, filtergain, outampmod, outvolmod, allowidlerenders ? "allowed" : "not allowed", limitoutput ? "true" : "false");

    lapi->log(DSPLogLevel_Info, "filterprops:");
    filter_printprops(lapi->log, filter);
    alDeleteFilters(1, &filter);
    return DSPModuleReturnCode_Continue;
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

    return DSPModuleReturnCode_Continue;
}