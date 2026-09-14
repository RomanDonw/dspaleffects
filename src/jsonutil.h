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

bool json_getfloat(const struct json_object *obj, float *value);
bool json_getbool(const struct json_object *obj, bool *value);

#endif