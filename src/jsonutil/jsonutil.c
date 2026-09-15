/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "jsonutil.h"

#include <errno.h>

bool json_getfloat(const struct json_object *obj, float *value)
{
    errno = 0;
    double v = json_object_get_double(obj);
    if (errno) return true;
    *value = v;
    return false;
}

bool json_getbool(const struct json_object *obj, bool *value)
{
    errno = 0;
    bool v = json_object_get_boolean(obj);
    if (errno) return true;
    *value = v;
    return false;
}