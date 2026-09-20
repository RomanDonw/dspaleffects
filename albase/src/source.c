/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "albase.h"

#include <stdlib.h>
#include <alext.h>

#include "context.h"

void albase_source_create(ALBaseSource *source)
{
    alGenSources(1, &source->source);
    alGenBuffers(2, source->buffers);
}

void albase_source_destroy(const ALBaseSource *source)
{
    alDeleteBuffers(2, source->buffers);
    alDeleteSources(1, &source->source);
}

char albase_source_updatemono(const ALBaseSource *source, const float mono[], unsigned long duration, unsigned long rate)
{
    ALint procbuffs, queuedbuffs;
    alGetSourcei(source->source, AL_BUFFERS_PROCESSED, &procbuffs);
    alGetSourcei(source->source, AL_BUFFERS_QUEUED, &queuedbuffs);

    ALuint buff;
    while (procbuffs-- > 0)
    {
        alSourceUnqueueBuffers(source->source, 1, &buff);
        alBufferData(buff, AL_FORMAT_MONO_FLOAT32, mono, duration * sizeof(float), rate);
        alSourceQueueBuffers(source->source, 1, &buff);
    }
    while (queuedbuffs < 2)
    {
        buff = source->buffers[queuedbuffs++];
        alBufferData(buff, AL_FORMAT_MONO_FLOAT32, mono, duration * sizeof(float), rate);
        alSourceQueueBuffers(source->source, 1, &buff);
    }

    ALint state;
    alGetSourcei(source->source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) alSourcePlay(source->source);

    return 0;
}

char albase_source_updatestereo(const ALBaseSource *source, const float left[], const float right[], unsigned long duration, unsigned long rate)
{
    if (tmpbuffsize != sizeof(float) * duration * 2)
    {
        void *new_tmpbuffdata = realloc(tmpbuffdata, sizeof(float) * duration * 2);
        if (!new_tmpbuffdata) return 1;
        tmpbuffdata = new_tmpbuffdata;
        tmpbuffsize = sizeof(float) * duration * 2;
    }

    for (size_t i = 0; i < (size_t)duration << 1; i++) ((float *)tmpbuffdata)[i] = i & 1 ? (right ? right[i >> 1] : 0) : (left ? left[i >> 1] : 0);

    ALint procbuffs, queuedbuffs;
    alGetSourcei(source->source, AL_BUFFERS_PROCESSED, &procbuffs);
    alGetSourcei(source->source, AL_BUFFERS_QUEUED, &queuedbuffs);

    ALuint emptybuff;
    while (procbuffs-- > 0)
    {
        alSourceUnqueueBuffers(source->source, 1, &emptybuff);
        alBufferData(emptybuff, AL_FORMAT_STEREO_FLOAT32, tmpbuffdata, tmpbuffsize, rate);
        alSourceQueueBuffers(source->source, 1, &emptybuff);
    }
    while (queuedbuffs < 2)
    {
        emptybuff = source->buffers[queuedbuffs++];
        alBufferData(emptybuff, AL_FORMAT_STEREO_FLOAT32, tmpbuffdata, tmpbuffsize, rate);
        alSourceQueueBuffers(source->source, 1, &emptybuff);
    }

    ALint state;
    alGetSourcei(source->source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) alSourcePlay(source->source);

    return 0;
}