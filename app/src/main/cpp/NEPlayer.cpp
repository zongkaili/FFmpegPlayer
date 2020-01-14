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
}

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO, "kelly", FORMAT, ##__VA_ARGS__);

NEPlayer::NEPlayer(const char *data_source, JniCallbackHelper *jni_callback_helper) {
//     this->data_source = data_source;
    this->data_source = new char[strlen(data_source) + 1];//c++中字符串以"\0"结尾，所以需要+1
    strcpy(this->data_source, data_source);

    this->jni_callback_helper = jni_callback_helper;
}

NEPlayer::~NEPlayer() {
    if (data_source) {
        delete data_source;
        data_source = 0;
    }
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
    AVFormatContext *formatContext = avformat_alloc_context();

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
        return;
    }

    /**
     * 2.查找信息
     */
    ret = avformat_find_stream_info(formatContext, 0);
    if (ret < 0) {
        LOGI("%s", "prepare, avformat_find_stream_info error.");
        return;
    }

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
            return;
        }

        /**
         * 7.解码器上下文创建
         */
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            LOGI("%s", "prepare, avcodec_alloc_context3 error.")
            return;
        }

        /**
         * 8.设置上下文参数
         */
        ret = avcodec_parameters_to_context(codecContext, codecParameters);
        if (ret < 0) {
            LOGI("%s", "prepare, avcodec_parameters_to_context error.")
            return;
        }

        /**
         * 9.打开解码器
         */
        ret = avcodec_open2(codecContext, codec, 0);
        if (ret < 0) {
            LOGI("%s", "prepare, avcodec_open2 error.")
            return;
        }

        /**
         * 10.从编码器的参数中获取流类型
         */
        if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            //音频流
            audio_channel = new AudioChannel();
        } else if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            //视频流
            video_channel = new VideoChannel();
        }
    }

    /**
     * 11.没有流的情况处理
     */
    if (!audio_channel && !video_channel) {
        LOGI("%s", "没有找到音频流和视频流.")
        return;
    }

    //准备工作做好了，回调java可以做后面的操作了
    if (jni_callback_helper) {
        jni_callback_helper->onPrepared(THREAD_CHILD);
    }
}


