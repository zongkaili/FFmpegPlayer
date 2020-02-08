//
// Created by 宗开黎 on 2020-01-13.
//

#include <string.h>
#include <android/log.h>
#include <pthread.h>

#include "NEPlayer.h"
#include "AudioChannel.h"
#include "VideoChannel.h"
#include "macro.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

NEPlayer::NEPlayer(const char *data_source, JniCallbackHelper *jni_callback_helper) {
//     this->data_source = data_source;
    this->data_source = new char[strlen(data_source) + 1];//c++中字符串以"\0"结尾，所以需要+1
    strcpy(this->data_source, data_source);
    this->jni_callback_helper = jni_callback_helper;
    pthread_mutex_init(&seek_mutex, 0);
}

NEPlayer::~NEPlayer() {
//    if (data_source) {
//        delete data_source;
//        data_source = 0;
//    }
    DELETE(data_source);
    DELETE(jni_callback_helper);
    pthread_mutex_destroy(&seek_mutex);
}

void *task_prepare(void *args) {
    NEPlayer *player = static_cast<NEPlayer *>(args);
    player->_prepare();
    return 0;//函数指针一定要return
}

void NEPlayer::prepare() {
    /**
     * 开子线程执行耗时任务
     */
    pthread_create(&pid_prepare, 0, task_prepare, this);
}

void NEPlayer::_prepare() {
    //1.打开媒体地址
    formatContext = avformat_alloc_context();

    AVDictionary *dictionary = 0;
    av_dict_set(&dictionary, "timeout", "5000000", 0);//单位微秒
    /**
     * 1.上下文
     * 2.url:文件路径或直播地址
     * 3.AVInputFormat: 输入的封装格式
     * 4.参数
     */
    int ret = avformat_open_input(&formatContext, data_source, 0, &dictionary);
    av_dict_free(&dictionary);
    if (ret) {
        LOGI("%s", "prepare, avformat_open_input error.");
        if (jni_callback_helper) {
            jni_callback_helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }

    /**
     * 2.查找信息
     */
    ret = avformat_find_stream_info(formatContext, 0);
    if (ret < 0) {
        LOGI("%s", "prepare, avformat_find_stream_info error.");
        if (jni_callback_helper) {
            jni_callback_helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    mDuration = formatContext->duration / AV_TIME_BASE;

    /**
     * 3.根据流信息个数循环查找
     */
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        /**
         * 4.获取媒体流（音/视频）
         */
        AVStream *stream = formatContext->streams[i];
        /**
         * 5.从流中获取解码这段流的参数
         */
        AVCodecParameters *codecParameters = stream->codecpar;
        /**
         * 6.通过流的编解码参数中的编解码id,来获取当前流的解码器
         */
        AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
        if (!codec) {
            LOGI("%s", "prepare, avcodec_find_decoder error.")
            if (jni_callback_helper) {
                jni_callback_helper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }

        /**
         * 7.解码器上下文创建
         */
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            LOGI("%s", "prepare, avcodec_alloc_context3 error.")
            if (jni_callback_helper) {
                jni_callback_helper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }

        /**
         * 8.设置上下文参数
         */
        ret = avcodec_parameters_to_context(codecContext, codecParameters);
        if (ret < 0) {
            LOGI("%s", "prepare, avcodec_parameters_to_context error.")
            if (jni_callback_helper) {
                jni_callback_helper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }

        /**
         * 9.打开解码器
         */
        ret = avcodec_open2(codecContext, codec, 0);
        if (ret < 0) {
            LOGI("%s", "prepare, avcodec_open2 error.")
            if (jni_callback_helper) {
                jni_callback_helper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        AVRational time_base = stream->time_base;
        /**
         * 10.从编码器的参数中获取流类型
         */
        if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            //音频流
            audio_channel = new AudioChannel(i, codecContext, time_base);
            if (mDuration != 0) {//直播不需要回调进度
                audio_channel->setJniCallbackHelper(jni_callback_helper);
            }
        } else if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            //视频流
            AVRational frame_rate = stream->avg_frame_rate;
            //转fps
            int fps = av_q2d(frame_rate);
            video_channel = new VideoChannel(i, codecContext, time_base, fps);
            video_channel->setRenderCallback(renderCallback);
            if (mDuration != 0) {
                video_channel->setJniCallbackHelper(jni_callback_helper);
            }
        }
    }

    /**
     * 11.没有流的情况处理
     */
    if (!audio_channel && !video_channel) {
        LOGI("%s", "没有找到音频流和视频流.")
        if (jni_callback_helper) {
            jni_callback_helper->onError(THREAD_CHILD, FFMPEG_NO_MEDIA);
        }
        return;
    }

    //准备工作做好了，回调java可以做后面的操作了
    if (jni_callback_helper) {
        jni_callback_helper->onPrepared(THREAD_CHILD);
    }
}

void *task_start(void *args) {
    NEPlayer *player = static_cast<NEPlayer *>(args);
    player->_start();
    return 0;//函数指针一定要return
}

void NEPlayer::start() {
    isPlaying = 1;
    if (video_channel) {
        video_channel->setAudioChannel(audio_channel);
        video_channel->start();
    }
    if (audio_channel) {
        audio_channel->start();
    }
    pthread_create(&pid_start, 0, task_start, this);
}

/**
 * 真正开始播放
 * 读取音视频包，加入相应的音视频队列
 * TODO AVPacket 生产
 */
void NEPlayer::_start() {
    while (isPlaying) {
        /**
        * 泄漏点1：控制 AVPacket 队列大小，等待队列中的数据被消费
        */
        if (video_channel && video_channel->packets.size() > 100) {
            av_usleep(10 * 1000);//microceconds 微秒
            //休眠
            continue;
        }
        if (audio_channel && audio_channel->packets.size() > 100) {
            av_usleep(10 * 1000);//microceconds 微秒
            //休眠
            continue;
        }

        AVPacket *packet = av_packet_alloc();
        pthread_mutex_lock(&seek_mutex);
        int ret = av_read_frame(formatContext, packet);
        pthread_mutex_unlock(&seek_mutex);
        if (!ret) {//读取成功
           if (video_channel && video_channel->stream_index == packet->stream_index) {//视频数据包
               video_channel->packets.push(packet);
           } else if (audio_channel && audio_channel->stream_index == packet->stream_index) {//音频数据包
               audio_channel->packets.push(packet);
           }
        } else if (ret == AVERROR_EOF) {//End of file

        } else {
            break;
        }
    }
    isPlaying = 0;
    video_channel->stop();
    audio_channel->stop();
}

void NEPlayer::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}

int NEPlayer::getDuration() {
    return mDuration;
}

/**
 * 指定时间点重新开始播放
 * @param progress 要seek的时间戳
 */
void NEPlayer::seek(int progress) {
    if (progress < 0 || progress > mDuration) {
        return;
    }
    if (!audio_channel && !video_channel) {
        return;
    }
    if (!formatContext) {
        return;
    }
    pthread_mutex_lock(&seek_mutex);

    int ret = av_seek_frame(formatContext, -1, progress * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        if (jni_callback_helper) {
            jni_callback_helper->onError(THREAD_CHILD, ret);
        }
        return;
    }
    //4个队列可能存在 未消费数据，需要reset
    if (audio_channel) {
        audio_channel->packets.setWork(0);
        audio_channel->frames.setWork(0);
        audio_channel->packets.clear();
        audio_channel->frames.clear();
        audio_channel->packets.setWork(1);
        audio_channel->frames.setWork(1);
    }
    if (video_channel) {
        video_channel->packets.setWork(0);
        video_channel->frames.setWork(0);
        video_channel->packets.clear();
        video_channel->frames.clear();
        video_channel->packets.setWork(1);
        video_channel->frames.setWork(1);
    }
    pthread_mutex_unlock(&seek_mutex);
}

void *task_stop(void *args) {
    NEPlayer *player = static_cast<NEPlayer *>(args);
    player->isPlaying = 0;
    //pthread_join 引发anr
    pthread_join(player->pid_prepare, 0);
    pthread_join(player->pid_start, 0);
    if (player->formatContext) {
        avformat_close_input(&player->formatContext);
        avformat_free_context(player->formatContext);
        player->formatContext = 0;
    }
    DELETE(player->audio_channel);
    DELETE(player->video_channel);

    return 0;// 一定要return！
}

void NEPlayer::stop() {
    jni_callback_helper = 0;
    if (video_channel) {
        video_channel->jni_callback_helper = 0;
    }
    if (audio_channel) {
        audio_channel->jni_callback_helper = 0;
    }
    pthread_create(&pid_stop, 0, task_stop, this);
}


