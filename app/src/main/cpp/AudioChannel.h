//
// Created by 宗开黎 on 2020-01-13.
//

#ifndef FFMPEGPLAYER_AUDIOCHANNEL_H
#define FFMPEGPLAYER_AUDIOCHANNEL_H


#include "BaseChannel.h"

class AudioChannel : public BaseChannel {

public:
    AudioChannel(int stream_index, AVCodecContext *codecContext);
};


#endif //FFMPEGPLAYER_AUDIOCHANNEL_H
