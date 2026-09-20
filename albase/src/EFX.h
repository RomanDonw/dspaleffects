/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef EFX_H
#define EFX_H

#include <alext.h>

#ifdef EFX_IMPL
    #define EFX_EXTERN_KW
#else
    #define EFX_EXTERN_KW extern
#endif

EFX_EXTERN_KW LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
EFX_EXTERN_KW LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;

EFX_EXTERN_KW LPALGENEFFECTS alGenEffects;
EFX_EXTERN_KW LPALDELETEEFFECTS alDeleteEffects;
EFX_EXTERN_KW LPALEFFECTI alEffecti;
EFX_EXTERN_KW LPALEFFECTF alEffectf;
EFX_EXTERN_KW LPALEFFECTFV alEffectfv;

EFX_EXTERN_KW LPALGENFILTERS alGenFilters;
EFX_EXTERN_KW LPALDELETEFILTERS alDeleteFilters;
EFX_EXTERN_KW LPALFILTERI alFilteri;
EFX_EXTERN_KW LPALFILTERF alFilterf;

#undef EFX_EXTERN_KW

#endif