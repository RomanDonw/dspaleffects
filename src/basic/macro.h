/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifdef MACRO_H
    #error macro.h can be included only one time
#endif
#define MACRO_H

#include "jsonutil/jsonutil.h"

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

#ifdef MACRO_USEFLTARRAY
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
#else
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
#endif

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
        if (sscanf(optarg, "%f", &floatopts[optindex].value) < 1) { puts("error parsing option --" optname " (required float)"); return 1; }\
        floatopts[optindex].has = true;\
        break;

#define PARSELONGVEC3OPT(optid, optindex, optname) \
    case optid:\
        if (sscanf(optarg, "%f,%f,%f", &vec3opts[optindex].value[0], &vec3opts[optindex].value[1], &vec3opts[optindex].value[2]) < 3)\
        { puts("error parsing option --" optname " (required 3D vector - \"float, float, float\")"); return 1; }\
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
            puts("error parsing option --" optname " (required boolean, allowed values: integer (where 0 - disable, other - enable), "\
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
