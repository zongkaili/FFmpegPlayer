//
// Created by 宗开黎 on 2020-01-13.
//

#ifndef FFMPEGPLAYER_AUDIOCHANNEL_H
#define FFMPEGPLAYER_AUDIOCHANNEL_H


#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "BaseChannel.h"
extern "C" {
#include <libswresample/swresample.h>
}

class AudioChannel : public BaseChannel {

public:
    AudioChannel(int stream_index, AVCodecContext *codecContext);

    void start();

    void audio_decode();

    void audio_play();

    int getPCM();

    uint8_t *out_buffers = 0;
    int out_sample_rate;
    int out_buffers_size;
    int out_sample_size;
    int out_channels;

private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;

    //引擎
    SLObjectItf engineObject = 0;
   //引擎接口
    SLEngineItf engineInterface = 0;
    //混音器
    SLObjectItf outputMixObject = 0;
    //播放器
    SLObjectItf bqPlayerObject = 0;
    //播放器接口
    SLPlayItf bqPlayerPlay = 0;
    //播放器队列接口
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = 0;

    SwrContext *swr_context = 0;
};


#endif //FFMPEGPLAYER_AUDIOCHANNEL_H
