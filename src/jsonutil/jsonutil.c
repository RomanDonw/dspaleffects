/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "jsonutil.h"

#include <errno.h>
#include <string.h>

bool jsonutil_getfloat(const struct json_object *obj, float *value)
{
    enum json_type objt = json_object_get_type(obj);
    if (objt != json_type_double && objt != json_type_int) { errno = EINVAL; return true; }
    *value = json_object_get_double(obj);
    return false;
}

bool jsonutil_getbool(const struct json_object *obj, bool *value)
{
    if (json_object_get_type(obj) != json_type_boolean) { errno = EINVAL; return true; }
    *value = json_object_get_boolean(obj);
    return false;
}

bool jsonutil_getvec3f(const struct json_object *obj, float value[])
{
    if (json_object_get_type(obj) != json_type_array) { errno = EINVAL; return true; }

    float ret[3];
    json_object *element;

    if (!(element = json_object_array_get_idx(obj, 0))) goto idxinval;
    if (jsonutil_getfloat(obj, ret)) return true;

    if (!(element = json_object_array_get_idx(obj, 1))) goto idxinval;
    if (jsonutil_getfloat(obj, ret + 1)) return true;

    if (!(element = json_object_array_get_idx(obj, 2))) goto idxinval;
    if (jsonutil_getfloat(obj, ret + 2)) return true;

    memcpy(value, ret, sizeof(ret));
    return false;

    idxinval:
        errno = ERANGE;
    return true;
}