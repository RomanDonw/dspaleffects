/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

// unused and will be removed in the future commits, but saved for the history.

#ifndef TMPBUFF_H
#define TMPBUFF_H

#include <stddef.h>
#include <stdbool.h>

struct tmpbuff
{
    void *buffer;
    size_t size;
} typedef tmpbuff_t;

bool tmpbuff_get(void **buff, size_t size);
void tmpbuff_free(void *buff);

#endif