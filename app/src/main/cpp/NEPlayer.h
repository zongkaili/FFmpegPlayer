//
// Created by 宗开黎 on 2020-01-13.
//

#ifndef FFMPEGPLAYER_NEPLAYER_H
#define FFMPEGPLAYER_NEPLAYER_H

extern "C" {
#include <libavformat/avformat.h>
}

#include "AudioChannel.h"
#include "VideoChannel.h"
#include "JniCallbackHelper.h"

class NEPlayer {
    friend void *task_stop(void *args);

public:
    NEPlayer(const char *data_source, JniCallbackHelper *pHelper);

    virtual ~NEPlayer();

    void prepare();

    void _prepare();

    void start();

    void _start();

    void setRenderCallback(RenderCallback renderCallback);

    jint getDuration();

    void seek(jint i);

    void stop();

private:
    char *data_source;
    AudioChannel *audio_channel = 0;
    VideoChannel *video_channel = 0;
    JniCallbackHelper *jni_callback_helper = 0;
    pthread_t pid_prepare;
    pthread_t pid_start;
    pthread_t pid_stop;
    AVFormatContext *formatContext = 0;
    bool isPlaying;
    int mDuration;
    RenderCallback renderCallback;
    pthread_mutex_t seek_mutex;
};


#endif //FFMPEGPLAYER_NEPLAYER_H
