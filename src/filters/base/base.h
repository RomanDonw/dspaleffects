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

extern const struct option filter_longopts[];

extern unsigned short filter_optcallback(int optid);
// [filter_poststartup]: configroot can be NULL.
extern unsigned short filter_poststartup(ALuint filter, float filtergain, const char **sysname, const char **dispname);
extern void filter_printprops(ALuint filter);

#endif