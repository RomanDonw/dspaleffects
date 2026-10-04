#include "base.h"

#include <dspmodule.h>
#include <stdio.h>
#include <stdlib.h>

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

const unsigned short dspmodule_requiredAPIversion = 1;

static float origgain = 1, effectgain = 1, inampmod = 0, involmod = 1, outampmod = 0, outvolmod = 1;
static void *inleftport, *inrightport, *outleftport, *outrightport;
static ALuint source, buffer;
static bool allowidlerenders = false, strongconfoptcheck = true;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    struct json_object *configroot = NULL;
    const char *configfilename = NULL;
    unsigned short ret;

    {
        int p;
        while ((p = getopt_long(argc, argv, "g:G:f:a:v:A:V:is", effect_longopts, NULL)) != -1)
        {
            if (p > 255) if (ret = effect_optcallback(p)) return ret;
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
                PARSELONGFLOATOPT(256, 0, "delay")
                PARSELONGFLOATOPT(257, 1, "depth")
                PARSELONGFLOATOPT(258, 2, "feedback")
                PARSELONGFLOATOPT(259, 3, "rate")

                case 260:
                    if (sscanf(optarg, "%i", &intopts[0].value) < 1) { puts("error parsing option --phase (required integer)"); return 1; }
                    intopts[0].has = true;
                    break;

                case 261:
                    if (!strcmp(optarg, "sinusoid")) intopts[1].value = AL_CHORUS_WAVEFORM_SINUSOID;
                    else if (!strcmp(optarg, "triangle")) intopts[1].value = AL_CHORUS_WAVEFORM_TRIANGLE;
                    else { printf("incorrect --waveform enumeration option value (allowed: \"sinusoid\" or \"triangle\", got: \"%s\")\n", optarg); return 1; }
                    intopts[1].has = true;
                    break;
                */
            }
        }
    }
    
    if (alutil_init(48000, true, false)) return 1;
    if (alutil_loadEFX()) return 1;
    
    // ===============================

    ALuint effect;
    if (ret = effect_poststartup(configroot, strongconfoptcheck, &effect, sysname, dispname)) return ret;

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

    effect_printprops(effect);
    alDeleteEffects(1, &effect);
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