/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef UTIL_H
#define UTIL_H

#include <al.h>
#include <alc.h>

const char *al_strerror(ALenum error);
const char *alc_strerror(ALCenum error);

#endif