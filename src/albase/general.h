/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef GENERAL_H
#define GENERAL_H

#include <dspmodule.h>
#include <al.h>

extern ALuint albase_source;
extern float albase_inampmod, albase_involmod, albase_outampmod, albase_outvolmod;

// returns 0 on success.

char albase_init(const DSPLoaderAPI *lapi);
//char albase_process(const DSPLoaderAPI *lapi, unsigned long duration, unsigned long rate);
void albase_quit(void);

#endif