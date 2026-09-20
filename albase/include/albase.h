/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef ALBASE_H
#define ALBASE_H

#include <stdbool.h>
#include <al.h>

// returns 0 on success.

char albase_init(unsigned long firstrate);
//char albase_checkrate(unsigned long rate);
// [albase_render]: left & right can be NULL (all combinations are allowed).
char albase_render(float left[], float right[], unsigned long duration, unsigned long rate);
void albase_quit(void);

struct ALBaseSource
{
    // all fields are readonly!
    ALuint source, buffers[2];
    //bool stereo;
} typedef ALBaseSource;

void albase_source_create(ALBaseSource *source);
void albase_source_destroy(const ALBaseSource *source);
// [albase_source_updatemono]: mono can be NULL.
char albase_source_updatemono(const ALBaseSource *source, const float mono[], unsigned long duration, unsigned long rate);
// [albase_source_updatestereo]: left & right can be NULL (all combinations are allowed).
char albase_source_updatestereo(const ALBaseSource *source, const float left[], const float right[], unsigned long duration, unsigned long rate);

#endif