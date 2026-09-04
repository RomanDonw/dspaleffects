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