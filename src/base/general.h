/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef GENERAL_H
#define GENERAL_H

#include <dspmodule.h>
#include <al.h>

extern ALuint al_source;
extern float al_inampmod, al_involmod, al_outampmod, al_outvolmod;

// returns 0 on success.

char al_init(const DSPLoaderAPI *lapi);
char al_process(const DSPLoaderAPI *lapi, unsigned long duration, unsigned long rate);
void al_quit(void);

#endif