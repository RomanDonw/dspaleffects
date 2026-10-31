/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef ALUTIL_GENERAL_H
#define ALUTIL_GENERAL_H

#include <dspmodule.h>
#include <stdbool.h>
#include <al.h>

// all these functions returns 0 on success.

// [alutil_init & alutil_render]: log can be NULL.
char alutil_init(DSPLoaderAPILogFunctionPrototype log, unsigned long firstrate, bool limitoutput);
char alutil_render(DSPLoaderAPILogFunctionPrototype log, float interleaved[], unsigned long duration, unsigned long rate);
char alutil_quit(void);

#endif