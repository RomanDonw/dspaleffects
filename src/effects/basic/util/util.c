/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "util.h"

#include <ctype.h>

char strfcmpl(const char *str1, const char *str2)
{
    while ((*str1) && (*str2))
    {
        if (tolower(*str1) != tolower(*str2)) return 0;
        str1++;
        str2++;
    }
    return 1;
}