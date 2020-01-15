//
// Created by 宗开黎 on 2020-01-13.
//

#ifndef FFMPEGPLAYER_NEPLAYER_H
#define FFMPEGPLAYER_NEPLAYER_H


#include "AudioChannel.h"
#include "VideoChannel.h"
#include "JniCallbackHelper.h"

class NEPlayer {
public:
    NEPlayer(const char *data_source, JniCallbackHelper *pHelper);

    virtual ~NEPlayer();

    void prepare();

    void _prepare();

private:
    char *data_source;
    AudioChannel *audio_channel = 0;
    VideoChannel *video_channel = 0;
    JniCallbackHelper *jni_callback_helper = 0;
    pthread_t pid_prepare;
};


#endif //FFMPEGPLAYER_NEPLAYER_H
