//
// Created by 宗开黎 on 2020-01-13.
//


#ifndef FFMPEGPLAYER_BASECHANNEL_H
#define FFMPEGPLAYER_BASECHANNEL_H
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
}

#include "safe_queue.h"

class BaseChannel {
public:
    BaseChannel(int streamIndex, AVCodecContext *codecContext) {
        this->stream_index = streamIndex;
        this->codecContext = codecContext;
        packets.setReleaseCallback(releaseAVPacket);
        frames.setReleaseCallback(releaseAVFrame);
    }

    virtual ~BaseChannel() {
        packets.clear();
        frames.clear();
    }

    /**
     * 释放AVPacket
     * @param packet
     */
    static void releaseAVPacket(AVPacket **packet) {
        if (packet) {
            av_packet_free(packet);
            *packet = 0;
        }
    }

    /**
     * 释放AVFrame
     * @param packet
     */
    static void releaseAVFrame(AVFrame **frame) {
        if (frame) {
            av_frame_free(frame);
            *frame = 0;
        }
    }
    int isPlaying;
    int stream_index;
    SafeQueue<AVPacket *> packets;
    SafeQueue<AVFrame *> frames;
    AVCodecContext *codecContext = 0;
};


#endif //FFMPEGPLAYER_BASECHANNEL_H
