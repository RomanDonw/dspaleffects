/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "base.h"

#include <dspmodule.h>
#include <stdio.h>
#include <stdlib.h>

const unsigned short dspmodule_requiredAPIversion = 1;

static float origgain = 1, filtergain = 1, inampmod = 0, involmod = 1, outampmod = 0, outvolmod = 1;
static void *inleftport, *inrightport, *outleftport, *outrightport;
static ALuint source, buffer;
static bool allowidlerenders = false, limitoutput = false;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    unsigned short ret;

    {
        int p;
        while ((p = getopt_long(argc, argv, "g:G:a:v:A:V:io", filter_longopts, NULL)) != -1)
        {
            if (p > 255 || !p) { if (ret = filter_optcallback(p)) return ret; }
            else switch (p)
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
                    if (sscanf(optarg, "%f", &filtergain) < 1) { puts("error parsing option -G (required float)"); return 1; }
                    filtergain = clampf(filtergain, 0, 1);
                    break;

                case 'i':
                    allowidlerenders = true;
                    break;

                case 'o':
                    limitoutput = true;
                    break;
            }
        }
    }
    
    if (alutil_init(48000, true, limitoutput, ALC_STEREO_SOFT)) return 1;
    if (alutil_loadEFX()) return 1;
    
    // ===============================

    ALuint filter;
    alGenFilters(1, &filter);
    if (ret = filter_poststartup(filter, filtergain, sysname, dispname)) return ret;

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
    alSourcef(source, AL_ROLLOFF_FACTOR, 0);
    alSourcei(source, AL_DIRECT_FILTER, filter);
    
    // ===============================

    alGenBuffers(1, &buffer);

    // ===============================

    printf("inampmod: %f\ninvolmod: %f\noriggain: %f\nfiltergain: %f\noutampmod: %f\noutvolmod: %f\nidlerendering: %s\nlimitoutput: %s\n",
        inampmod, involmod, origgain, filtergain, outampmod, outvolmod, allowidlerenders ? "allowed" : "not allowed", limitoutput ? "true" : "false");

    puts("filterprops:");
    filter_printprops(filter);
    alDeleteFilters(1, &filter);
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
    { buff[i] = adjf(i & 1 ? (inright ? inright[i >> 1] : 0) : (inleft ? inleft[i >> 1] : 0), inampmod) * involmod; }
    alBufferData(buffer, AL_FORMAT_STEREO_FLOAT32, buff, buffsize, rate);

    alSourcei(source, AL_BUFFER, buffer);
    alSourcePlay(source);

    // ===============================

    if (alutil_render(buff, duration, rate)) return 1;
    if (outleft || outright) for (unsigned long i = 0; i < duration; i++)
    {
        if (outleft) outleft[i] = adjf(buff[i * 2], outampmod) * outvolmod;
        if (outright) outright[i] = adjf(buff[i * 2 + 1], outampmod) * outvolmod;
    }

    return 0;
}