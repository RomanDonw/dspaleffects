/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifdef MACRO_H
    #error macro.h can be included only one time
#endif
#define MACRO_H

struct floatopt { bool has; float value; } typedef floatopt;

#define PARSELONGFLOATOPT(optid, optindex, optname) \
    case optid:\
        if (sscanf(optarg, "%f", &floatopts[optindex].value) < 1) { puts("error parsing option --" optname " (required float)"); return 1; }\
        floatopts[optindex].has = true;\
        break;

#define SETFILTERFLOATPROPFROMOPT(optindex, alname) \
    if (floatopts[optindex].has) alFilterf(filter, alname, floatopts[optindex].value);
