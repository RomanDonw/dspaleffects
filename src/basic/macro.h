#ifdef MACRO_H
    #error macro.h can be included only single time
#endif
#define MACRO_H

struct floatopt { bool has; float value; } typedef floatopt;
struct intopt { bool has; int value; } typedef intopt;

#define GETFLTCONFOPTHELPER(floatoptsidx, strname, alname) \
    if (!floatopts[floatoptsidx].has)\
    {\
        if (json_object_object_get_ex(configroot, strname, &jobj))\
        {\
            if (jsonutil_getfloat(jobj, &f)) { puts("parsing \"" strname "\" config option failed (required float)"); return 1; }\
            alEffectf(effect, alname, f);\
        }\
        else if (strongconfoptcheck) { puts("key \"" strname "\" doesnt found in config file"); return 1; }\
    }

#define PARSELONGFLOATOPT(optid, optindex, optname) \
    case optid:\
        if (sscanf(optarg, "%f", &floatopts[optindex].value) < 1) { puts("error parsing option --" optname " (required float)"); return 1; }\
        floatopts[optindex].has = true;\
        break;

#define SETEFFFLOATPROPFROMOPT(optindex, alname) \
    if (floatopts[optindex].has) alEffectf(effect, alname, floatopts[optindex].value);
#define SETEFFINTPROPFROMOPT(optindex, alname) \
    if (intopts[optindex].has) alEffecti(effect, alname, intopts[optindex].value);
