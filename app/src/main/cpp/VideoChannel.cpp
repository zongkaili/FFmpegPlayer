//
// Created by 宗开黎 on 2020-01-13.
//

extern "C" {
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

#include "VideoChannel.h"
#include "macro.h"

/**
 *  当视频比音频播放慢时
 * 主动丢帧： 丢掉数据包信息
 * @param q
 */
void dropAVPacket(queue<AVPacket *> &q) {
    while (!q.empty()) {
        AVPacket *packet = q.front();
        //AV_PKT_FLAG_KEY 关键帧
        if (packet->flags != AV_PKT_FLAG_KEY) {
            //非关键帧才丢
            BaseChannel::releaseAVPacket(&packet);
            q.pop();
        } else {
            break;
        }
    }
}

/**
 * 当视频比音频播放慢时
 * 主动丢帧：丢掉帧数据
 * @param q
 */
void dropAVFrame(queue<AVFrame *> &q) {
    if (!q.empty()) {
        AVFrame *frame = q.front();
        BaseChannel::releaseAVFrame(&frame);
        q.pop();
    }
}

VideoChannel::VideoChannel(int stream_index, AVCodecContext *codecContext, AVRational time_base,
                           int fps) : BaseChannel(stream_index, codecContext, time_base) {
    this->fps = fps;
    packets.setSyncCallback(dropAVPacket);
    frames.setSyncCallback(dropAVFrame);
}

void *task_video_decode(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->video_decode();
    return 0;
}

/**
 * TODO AVPacket消费
 * 消费速度比生成速度慢
 */
void VideoChannel::video_decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        /**
         * 泄漏点2：控制 AVFrame队列
         */
        if (isPlaying && frames.size() > 100) {
            av_usleep(10 * 1000);//microceconds 微秒
            //休眠
            continue;
        }
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

        //        extra_delay = repeat_pict / (2*fps)
        double extra_delay = frame->repeat_pict / (2 * fps);//每一帧的额外延时时间
        double avg_delay = 1.0 / fps;//根据fps 得到的平均延时时间
        double real_delay = extra_delay + avg_delay;

        //视频时间
        double video_time = frame->best_effort_timestamp * av_q2d(time_base);

        if (!audio_channel) {//没音频
            av_usleep(real_delay * 1000000);
            if (jni_callback_helper){
                jni_callback_helper->onProgress(THREAD_CHILD, video_time);
            }
        } else {
            //以音频的时间为基准
            double audio_time = audio_channel->audio_time;
            double time_diff = video_time - audio_time;
            if (time_diff > 0) {
                //视频比音频快，等音频
                if (time_diff > 1){//时间差超过1秒
                    av_usleep((real_delay * 2) * 1000000);//慢慢等
                } else{
                    av_usleep((real_delay + time_diff) * 1000000);
                }
            } else if (time_diff < 0) {
                //画面延迟了
                //比音频慢，追音频 (丢帧)
                if (fabs(time_diff) >= 0.05) {
//                packets.sync();
                    frames.sync();
                    continue;
                }
            } else {
                //完美同步
                LOGI("%S", "视频和音频同步 >>>>> ");
            }
        }

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

void VideoChannel::setAudioChannel(AudioChannel *audio_channel) {
    this->audio_channel = audio_channel;
}


void VideoChannel::stop() {
    isPlaying = 0;
    packets.setWork(0);
    frames.setWork(0);
    pthread_join(pid_video_decode, 0);
    pthread_join(pid_video_play, 0);
}

