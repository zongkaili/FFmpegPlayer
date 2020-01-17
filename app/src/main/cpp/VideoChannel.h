//
// Created by 宗开黎 on 2020-01-13.
//

#ifndef FFMPEGPLAYER_VIDEOCHANNEL_H
#define FFMPEGPLAYER_VIDEOCHANNEL_H


#include "BaseChannel.h"

typedef void (*RenderCallback)(uint8_t *, int, int, int);

class VideoChannel : public BaseChannel {

public:
    VideoChannel(int stream_index, AVCodecContext *codecContext);

    void start();

    void video_decode();

    void video_play();

    void setRenderCallback(RenderCallback renderCallback);

private:
    int isPlaying;
    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    RenderCallback renderCallback;
};


#endif //FFMPEGPLAYER_VIDEOCHANNEL_H
