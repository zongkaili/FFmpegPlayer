//
// Created by 宗开黎 on 2020-01-13.
//

#include "AudioChannel.h"
#include "macro.h"

AudioChannel::AudioChannel(int stream_index, AVCodecContext *codecContext, AVRational time_base)
        : BaseChannel(stream_index, codecContext, time_base) {
    out_sample_rate = 44100;
    //输出缓冲区初始化
    //输出样本的大小
    out_sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);//16 bits = 2 字节
    //输出声道数
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);//2
    //输出缓冲区大小
    out_buffers_size = out_sample_rate * out_sample_size * out_channels;
    out_buffers = static_cast<uint8_t *>(malloc(out_buffers_size));
    memset(out_buffers, 0, out_buffers_size);
    //重采样上下文，申请空间并设置参数
    swr_context = swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, out_sample_rate,
                                     codecContext->channel_layout, codecContext->sample_fmt,
                                     codecContext->sample_rate, NULL, NULL);
    swr_init(swr_context);
}

void *task_audio_decode(void *args) {
    AudioChannel *videoChannel = static_cast<AudioChannel *>(args);
    videoChannel->audio_decode();
    return 0;
}

void AudioChannel::audio_decode() {
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

void *task_audio_play(void *args) {
    AudioChannel *videoChannel = static_cast<AudioChannel *>(args);
    videoChannel->audio_play();
    return 0;
}

//4.3 创建回调函数
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    int pcm_size = audioChannel->getPCM();
    if (pcm_size > 0) {
        (*bq)->Enqueue(bq, audioChannel->out_buffers, static_cast<SLuint32>(pcm_size));
    }
}

/**
 * 获取PCM数据
 * @return 数据大小
 */
int AudioChannel::getPCM() {
    int pcm_size = 0;
    AVFrame *frame = 0;
    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        //pcm数据处理，重采样
        //1.上下文， 2.输出缓冲区（大小要根据声音数据类型来定
        int out_nb_samples = static_cast<int>(av_rescale_rnd(
                swr_get_delay(swr_context, frame->sample_rate) + frame->nb_samples,
                frame->sample_rate, out_sample_rate, AV_ROUND_UP));
        //number of samples output per channel, 每个声道的样本数
        int samples_per_channel = swr_convert(swr_context, &out_buffers, out_nb_samples,
                                              (const uint8_t **) (frame->data), frame->nb_samples);
        //重采样后pcm数据大小 = 每个声道的样本数 * 声道数 * 一个样本的大小
        pcm_size = samples_per_channel * out_sample_size * out_channels;

        //每帧音频的时间
        audio_time = frame->best_effort_timestamp * av_q2d(time_base);
        if (jni_callback_helper) {
            jni_callback_helper->onProgress(THREAD_CHILD, audio_time);
        }
        break;
    }
    releaseAVFrame(&frame);
    return pcm_size;
}

/**
 * 播放pcm音频原始数据
 */
void AudioChannel::audio_play() {
    /**
     *  1、创建引擎并获取引擎接口 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
     */
    SLresult result;
    // 1.1 创建引擎对象：SLObjectItf engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.2 初始化引擎
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.3 获取引擎接口 SLEngineItf engineInterface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    /**
     * 2、设置混音器  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
     */
    // 2.1 创建混音器：SLObjectItf outputMixObject
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0,
                                                 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 2.2 初始化混音器
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //不启用混响可以不用获取混音器接口
    // 获得混音器接口
    //result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
    //                                         &outputMixEnvironmentalReverb);
    //if (SL_RESULT_SUCCESS == result) {
    //设置混响 ： 默认。
    //SL_I3DL2_ENVIRONMENT_PRESET_ROOM: 室内
    //SL_I3DL2_ENVIRONMENT_PRESET_AUDITORIUM : 礼堂 等
    //const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    //(*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
    //       outputMixEnvironmentalReverb, &settings);
    //}

    /**
     * 3、创建播放器 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
     */
    //3.1 配置输入声音信息
    //创建buffer缓冲类型的队列 2个队列
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                       2};
    //pcm数据格式
    //SL_DATAFORMAT_PCM：数据格式为pcm格式
    //2：双声道
    //SL_SAMPLINGRATE_44_1：采样率为44100
    //SL_PCMSAMPLEFORMAT_FIXED_16：采样格式为16bit
    //SL_PCMSAMPLEFORMAT_FIXED_16：数据大小为16bit
    //SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT：左右声道（双声道）
    //SL_BYTEORDER_LITTLEENDIAN：小端模式
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                                   SL_BYTEORDER_LITTLEENDIAN};

    //数据源 将上述配置信息放到这个数据源中
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    //3.2 配置音轨（输出）
    //设置混音器
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};
    //需要的接口 操作队列的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    //3.3 创建播放器
    result = (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &audioSrc,
                                                   &audioSnk, 1, ids, req);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //3.4 初始化播放器：SLObjectItf bqPlayerObject
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //3.5 获取播放器接口：SLPlayItf bqPlayerPlay
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    /**
     * 4、设置播放回调函数 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
     */
    //4.1 获取播放器队列接口：SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);

    //4.2 设置回调 void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);

    /**
     * 5、设置播放器状态为播放状态 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
     */
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);

    /**
     * 6、手动激活回调函数 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
     */
    bqPlayerCallback(bqPlayerBufferQueue, this);

//    /**
//     * 7、释放 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//     */
//    //7.1 设置停止状态
//    if (bqPlayerPlay) {
//        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
//        bqPlayerPlay = 0;
//    }
//    //7.2 销毁播放器
//    if (bqPlayerObject) {
//        (*bqPlayerObject)->Destroy(bqPlayerObject);
//        bqPlayerObject = 0;
//        bqPlayerBufferQueue = 0;
//    }
//    //7.3 销毁混音器
//    if (outputMixObject) {
//        (*outputMixObject)->Destroy(outputMixObject);
//        outputMixObject = 0;
//    }
//    //7.4 销毁引擎
//    if (engineObject) {
//        (*engineObject)->Destroy(engineObject);
//        engineObject = 0;
//        engineInterface = 0;
//    }


}

AudioChannel::~AudioChannel() {
    if (swr_context) {
        swr_free(&swr_context);
        swr_context = 0;
    }
    DELETE(out_buffers);
}

/**
 * 1.解码
 * 2.播放
 */
void AudioChannel::start() {
    isPlaying = 1;
    packets.setWork(1);
    frames.setWork(1);
    pthread_create(&pid_audio_decode, 0, task_audio_decode, this);
    pthread_create(&pid_audio_play, 0, task_audio_play, this);
}

void AudioChannel::stop() {
    isPlaying = 0;
    packets.setWork(0);
    frames.setWork(0);
    pthread_join(pid_audio_decode, 0);
    pthread_join(pid_audio_play, 0);

    //7.1 设置停止状态
    if (bqPlayerPlay) {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
        bqPlayerPlay = 0;
    }
    //7.2 销毁播放器
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;
        bqPlayerBufferQueue = 0;
    }
    //7.3 销毁混音器
    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }
    //7.4 销毁引擎
    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = 0;
        engineInterface = 0;
    }

}


