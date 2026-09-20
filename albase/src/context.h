/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "albase.h"

#include <stddef.h>

extern ALCint __albase_alctx_attrs[];
#define alctx_attrs (__albase_alctx_attrs)

extern void *__albase_tmpbuffdata;
extern size_t __albase_tmpbuffsize;
#define tmpbuffdata (__albase_tmpbuffdata)
#define tmpbuffsize (__albase_tmpbuffsize)