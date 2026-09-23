/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef ALUTIL_GENERAL_H
#define ALUTIL_GENERAL_H

#include <stdbool.h>
#include <al.h>

// all these functions returns 0 on success.

char alutil_init(unsigned long firstrate, bool errorlog);
char alutil_render(float interleaved[], unsigned long duration, unsigned long rate);
char alutil_quit(void);

#endif