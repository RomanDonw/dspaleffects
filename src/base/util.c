/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "util.h"

#include <stddef.h>

const char *al_strerror(ALenum error)
{
    switch (error)
    {
        case AL_NO_ERROR:
            return "no error (success)";

        case AL_INVALID_NAME:
            return "invalid name";

        case AL_INVALID_ENUM:
            return "invalid name";

        default:
            return NULL;
    }
}

const char *alc_strerror(ALCenum error)
{
    return NULL;
}