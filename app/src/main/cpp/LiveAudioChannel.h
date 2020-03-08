//
// Created by 宗开黎 on 2020/3/7.
//

#ifndef FFMPEGPLAYER_LIVEAUDIOCHANNEL_H
#define FFMPEGPLAYER_LIVEAUDIOCHANNEL_H


#include "librtmp/rtmp.h"
#include <sys/types.h>
#include <string.h>
#include <faac.h>

class LiveAudioChannel {
    typedef void(*AudioCallback)(RTMPPacket *packet);

public:
    LiveAudioChannel();

    ~LiveAudioChannel();

    void initAudioEncoder(int sample_rate, int channels);

    void encodeData(int8_t *data);

    int getInputSamples();

    void setAudioCallback(AudioCallback callback);

    RTMPPacket *getAudioSeqHeader();

private:
    int mChannels;

    u_long inputSamples;
    u_long maxOutputBytes;
    faacEncHandle audioEncoder = 0;
    u_char *buffer = 0;
    AudioCallback audioCallback;

};

#endif //FFMPEGPLAYER_LIVEAUDIOCHANNEL_H
