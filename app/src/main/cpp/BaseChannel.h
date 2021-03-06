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
#include "JniCallbackHelper.h"

class BaseChannel {
public:
    BaseChannel(int streamIndex, AVCodecContext *codecContext, AVRational time_base) {
        this->stream_index = streamIndex;
        this->codecContext = codecContext;
        this->time_base = time_base;
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

    void setJniCallbackHelper(JniCallbackHelper *jni_callback_helper) {
        this->jni_callback_helper = jni_callback_helper;
    }

    int isPlaying;
    int stream_index;
    SafeQueue<AVPacket *> packets;
    SafeQueue<AVFrame *> frames;
    AVCodecContext *codecContext = 0;
    AVRational time_base;
    double audio_time;
    JniCallbackHelper *jni_callback_helper = 0;
};


#endif //FFMPEGPLAYER_BASECHANNEL_H
