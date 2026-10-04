/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef BASE_H
#define BASE_H

#include <getopt.h>

#include "alutil/general.h"
#include "alutil/EFX.h"

#include <json-c/json_object.h>

extern const struct option effect_longopts[];

extern unsigned short effect_optcallback(int optid);
// [effect_poststartup]: configroot can be NULL.
extern unsigned short effect_poststartup(const struct json_object *configroot, bool strongconfoptcheck, ALuint effect, const char **sysname, const char **dispname);
extern void effect_printprops(ALuint effect);

#endif