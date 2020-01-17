//
// Created by 宗开黎 on 2020-01-13.
//

extern "C" {
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include "VideoChannel.h"

VideoChannel::VideoChannel(int stream_index, AVCodecContext *codecContext) : BaseChannel(
        stream_index, codecContext) {
}

void *task_video_decode(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->video_decode();
    return 0;
}

void VideoChannel::video_decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        //从队列中取视频数据压缩包 AVPacket
        int ret = packets.pop(packet);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        //把数据包发给解码器进行解码
        ret = avcodec_send_packet(codecContext, packet);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            break;
        }
        //发送一个数据包成功后，接受解码返回的frame
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            break;
        }
        //此时，一个数据包已被成功解码为AVFrame，加入队列，可以开始播放了
        frames.push(frame);
    }
    releaseAVPacket(&packet);
}

void *task_video_play(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->video_play();
    return 0;
}

void VideoChannel::video_play() {
    AVFrame *frame = 0;

    SwsContext *sws_ctx = sws_getContext(codecContext->width, codecContext->height,
                                         codecContext->pix_fmt,
                                         codecContext->width, codecContext->height,
                                         AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
    //接收的容器
    uint8_t *dst_data[4];
    //每一行的首地址
    int dst_linesize[4];
    //其内部会给dst_data和dst_linesize赋值
    av_image_alloc(dst_data, dst_linesize, codecContext->width, codecContext->height,
                   AV_PIX_FMT_RGBA, 1);
    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        //格式转换 yuv--->rgba  绘制
        sws_scale(sws_ctx, frame->data, frame->linesize, 0, codecContext->height, dst_data,
                  dst_linesize);

        renderCallback(dst_data[0], codecContext->width, codecContext->height, dst_linesize[0]);

        releaseAVFrame(&frame);
    }
    isPlaying = 0;
    releaseAVFrame(&frame);
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);
}

/**
 * 1.解码
 * 2.播放
 */
void VideoChannel::start() {
    isPlaying = 1;
    packets.setWork(1);
    frames.setWork(1);
    pthread_create(&pid_video_decode, 0, task_video_decode, this);
    pthread_create(&pid_video_play, 0, task_video_play, this);
}

void VideoChannel::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}



