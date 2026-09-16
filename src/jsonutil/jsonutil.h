/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef JSONUTIL_H
#define JSONUTIL_H

#include <stdbool.h>
#include <json-c/json_object.h>

// all this functions return true on error!!!

int jsonutil_getfloat(const struct json_object *obj, float *value);
int jsonutil_getbool(const struct json_object *obj, bool *value);
int jsonutil_getvec3f(const struct json_object *obj, float value[]);

#endif