#ifndef JSONUTIL_H
#define JSONUTIL_H

#include <stdbool.h>
#include <json-c/json_object.h>

// all this functions return true on error!!!

bool json_getfloat(const struct json_object *obj, float *value);
bool json_getbool(const struct json_object *obj, bool *value);

#endif