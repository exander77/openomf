#include "audio/stream.h"
#include "utils/log.h"
#include <AL/al.h>
#include <AL/alc.h>

int audio_stream_create(audio_stream *stream) {
    // Dump old errors
    while(alGetError() != AL_NO_ERROR);

    // Reserve resources
    alGenSources(1, &stream->alsource);
    alGenBuffers(AUDIO_BUFFER_COUNT, stream->albuffers);
    if(alGetError() != AL_NO_ERROR) {
        alDeleteSources(1, &stream->alsource);
        ERROR("Could not create OpenAL buffers!");
        return 1;
    }
    
    // Pick format
    switch(stream->bytes) {
        case 1: switch(stream->channels) {
            case 1: stream->alformat = AL_FORMAT_MONO8; break;
            case 2: stream->alformat = AL_FORMAT_STEREO8; break;
        }; break;
        case 2: switch(stream->channels) {
            case 1: stream->alformat = AL_FORMAT_MONO16; break;
            case 2: stream->alformat = AL_FORMAT_STEREO16; break;
        }; break;
    };
    if(!stream->alformat) {
        alDeleteSources(1, &stream->alsource);
        alDeleteBuffers(AUDIO_BUFFER_COUNT, stream->albuffers);
        ERROR("Could not find suitable audio format!");
        return 1;
    }
    
    // Set playing as 0. If playing = 1 and we lose data from buffers, 
    // this will reset playback & refill buffers.
    stream->playing = 0;
    
    // All done
    return 0;
}

int audio_stream_start(audio_stream *stream) {
    char buf[AUDIO_BUFFER_SIZE];
    int ret;
    unsigned int i;
    for(i = 0; i < AUDIO_BUFFER_COUNT; i++) {
        ret = stream->update(stream, buf, AUDIO_BUFFER_SIZE);
        alBufferData(stream->albuffers[i], stream->alformat, buf, ret, stream->frequency);
        if(ret <= 0) break; // If sample is short and doesn't have more data, stop here.
    }
    alSourceQueueBuffers(stream->alsource, i, stream->albuffers);
    alSourcePlay(stream->alsource);
    stream->playing = 1;
    return 0;
}

void audio_stream_stop(audio_stream *stream) {
    stream->playing = 0;
    alSourceStop(stream->alsource);
}

int alplaying(audio_stream *stream) {
    ALenum state;
    alGetSourcei(stream->alsource, AL_SOURCE_STATE, &state);
    return (state == AL_PLAYING);
}

int audio_stream_playing(audio_stream *stream) {
    return stream->playing;
}

int audio_stream_render(audio_stream *stream) {
    // Don't do anything unless playback has been started
    if(!stream->playing) {
        return 1;
    }

    // See if we have any empty buffers to fill
    int val;
    alGetSourcei(stream->alsource, AL_BUFFERS_PROCESSED, &val);
    if(val <= 0) {
        return 0;
    }

    // Handle buffer filling and loading
    char buf[AUDIO_BUFFER_SIZE];
    ALuint bufno;
    int ret = 0;
    int first = val;
    while(val--) {
        // Fill buffer & re-queue
        ret = stream->update(stream, buf, AUDIO_BUFFER_SIZE);
        if(ret <= 0 && val == first-1) {
            stream->playing = 0;
            return 1;
        }
        alSourceUnqueueBuffers(stream->alsource, 1, &bufno);
        alBufferData(bufno, stream->alformat, buf, ret, stream->frequency);
        alSourceQueueBuffers(stream->alsource, 1, &bufno);
    }
    
    // If stream has stopped because of empty buffers, restart playback.
    if(stream->playing && !alplaying(stream)) {
        alSourcePlay(stream->alsource);
    }
    
    // All done!
    return 0;
}

void audio_stream_free(audio_stream *stream) {
    // Free resources
    int val;
    ALuint buf;
    alGetSourcei(stream->alsource, AL_BUFFERS_PROCESSED, &val);
    while(val--) {
        alSourceUnqueueBuffers(stream->alsource, val, &buf);
    }
    alDeleteSources(1, &stream->alsource);
    alDeleteBuffers(AUDIO_BUFFER_COUNT, stream->albuffers);
}