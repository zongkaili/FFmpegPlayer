#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include <zconf.h>
#include <android/log.h>
#include "NEPlayer.h"
#include "JniCallbackHelper.h"
#include "macro.h"
#include "LiveVideoChannel.h"
//直播推流
#include <rtmp.h>
#include <x264.h>

#define MAX_AUDIO_FRAME_SIZE 48000 * 4
//FFmpeg是C库，在c++中调用c库代码需要用extern "C" {}
extern "C" {
//封装格式
#include <libavformat/avformat.h>
//解码
#include <libavcodec/avcodec.h>
//缩放
#include <libswscale/swscale.h>

#include <libavutil/imgutils.h>
//重采样
#include <libswresample/swresample.h>
}
JavaVM *javaVm = 0;
NEPlayer *player = 0;
ANativeWindow* window = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//静态初始化

jint JNI_OnLoad(JavaVM *vm, void *args) {
    javaVm = vm;
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_kelly_ffmpegplayer_MyPlayer_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    RTMP_LibVersion();
    x264_picture_t *picture = new x264_picture_t;
    x264_picture_init(picture);

    return env->NewStringUTF(hello.c_str());
}

void renderFrame(uint8_t *src_data, int width, int height, int src_linesize) {
    pthread_mutex_lock(&mutex);
    //先释放之前的显示窗口
    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    //设置窗口属性
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        return;
    }
    //填充rgb数据给dst_data
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    int dst_linesize = window_buffer.stride * 4;//一行数据的长度，每个像素4个字节， window_buffer.stride：buffer一行的像素个数
    for (int i = 0; i < window_buffer.height; ++i) {
        //通过内存拷贝来进行渲染
        memcpy(dst_data + i * dst_linesize, src_data + i * src_linesize, dst_linesize);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
}

extern "C" JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_MyPlayer_prepareNative(JNIEnv *env, jobject thiz,
                                                   jstring data_source_) {
    const char *data_source = env->GetStringUTFChars(data_source_, 0);
    JniCallbackHelper *jni_callback_helper = new JniCallbackHelper(javaVm, env, thiz);
    player = new NEPlayer(data_source, jni_callback_helper);
    player->setRenderCallback(renderFrame);
    player->prepare();
    env->ReleaseStringUTFChars(data_source_, data_source);
}
extern "C" JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_MyPlayer_startNative(JNIEnv *env, jobject instance) {
    if (player) {
        player->start();
    }
}
extern "C" JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_MyPlayer_stopNative(JNIEnv *env, jobject instance) {
    if (player) {
        player->stop();
    }
}
extern "C" JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_MyPlayer_releaseNative(JNIEnv *env, jobject instance) {
    pthread_mutex_lock(&mutex);
    //先释放之前的显示窗口
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    //创建新的窗口用于视频显示
    pthread_mutex_unlock(&mutex);
    DELETE(player);
}

extern "C" JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_MyPlayer_setSurfaceNative(JNIEnv *env, jobject clazz, jobject surface) {
    pthread_mutex_lock(&mutex);
    //先释放之前的显示窗口
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    //创建新的窗口用于视频显示
    window = ANativeWindow_fromSurface(env, surface);
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_kelly_ffmpegplayer_MyPlayer_getDurationNative(JNIEnv *env, jobject thiz) {
    if (player) {
        return player->getDuration();
    }
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_MyPlayer_seekNative(JNIEnv *env, jobject thiz, jint play_progress) {
    if (player) {
        player->seek(play_progress);
    }
}

//*********************之前的实现方式，已被重新封装**************************

extern "C" JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_MyPlayer_videoDecodeAndRender(
        JNIEnv *env, jobject instance,
        jstring path_, jobject surface) {
    LOGI("%s", "videoDecodeAndRender, start.");
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    const char *path = env->GetStringUTFChars(path_, 0);

    //FFmpeg 视频绘制
    //初始化网络模块
    avformat_network_init();
    //总上下文
    AVFormatContext *formatContext = avformat_alloc_context();

    AVDictionary *opts = NULL;
    av_dict_set(&opts, "timeout", "3000000", 0);
    int ret = avformat_open_input(&formatContext, path, NULL, &opts);
    if (ret) {
        LOGI("%s", "videoDecodeAndRender, avformat_open_input error.");
        return;
    }
    //视频流
    int video_stream_idx = -1;
    avformat_find_stream_info(formatContext, NULL);
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }
    //视频流索引
    AVCodecParameters *codecpar = formatContext->streams[video_stream_idx]->codecpar;
    //解码器
    AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
    //解码器的上下文
    AVCodecContext *codecContext = avcodec_alloc_context3(dec);
    //将解码器参数copy到解码器上下文
    avcodec_parameters_to_context(codecContext, codecpar);

    avcodec_open2(codecContext, dec, NULL);
    //解码 yuv数据
    AVPacket *packet = av_packet_alloc();
    //从视频流中读取数据包
    /**
     * 重视速度： fast_bilinear, point
     * 重视质量：cubic, spline, lanczos
     * 缩小：
     * 重视速度： fast_bilinear, point
     * 重视质量：gauss, bilinear
     * 重视锐度： cubic, spline, lanczos
     */
    SwsContext *swsContext = sws_getContext(codecContext->width, codecContext->height,
                                            codecContext->pix_fmt,
                                            codecContext->width, codecContext->height,
                                            AV_PIX_FMT_RGBA, SWS_BILINEAR, 0, 0, 0);
    ANativeWindow_setBuffersGeometry(nativeWindow, codecContext->width, codecContext->height,
                                     WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer outBuffer;
    while (av_read_frame(formatContext, packet) >= 0) {
        avcodec_send_packet(codecContext, packet);
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            break;
        }
        //接收的容器
        uint8_t *dst_data[4];
        //每一行的首地址
        int dst_linesize[4];
        //其内部会给dst_data和dst_linesize赋值
        av_image_alloc(dst_data, dst_linesize, codecContext->width, codecContext->height,
                       AV_PIX_FMT_RGBA, 1);
        //绘制
        sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height,
                  dst_data, dst_linesize);

        ANativeWindow_lock(nativeWindow, &outBuffer, NULL);

        //渲染
        uint8_t *firstWindow = static_cast<uint8_t *>(outBuffer.bits);
        uint8_t *src_data = dst_data[0];
        //拿到一行有多少个字节 RGBA
        int destStride = outBuffer.stride * 4;
        int src_linesize = dst_linesize[0];
        for (int i = 0; i < outBuffer.height; ++i) {
            //通过内存拷贝来进行渲染
            memcpy(firstWindow + i * destStride, src_data + i * src_linesize, destStride);
        }

        ANativeWindow_unlockAndPost(nativeWindow);
    }
    LOGI("%s", "videoDecodeAndRender, end.");

    env->ReleaseStringUTFChars(path_, path);
}

extern "C" JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_MyPlayer_audioDecode(
        JNIEnv *env, jobject instance,
        jstring input_, jstring output_) {
    const char *input = env->GetStringUTFChars(input_, 0);
    const char *output = env->GetStringUTFChars(output_, 0);
    avformat_network_init();
    //总上下文
    AVFormatContext *formatContext = avformat_alloc_context();
    //打开音频文件
    if (avformat_open_input(&formatContext, input, NULL, NULL) != 0) {
        LOGI("%s", "无法打开音频文件");
        return;
    }
    //获取输入文件信息
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        LOGI("%s", "无法获取输入文件信息");
        return;
    }
    //视频时长（单位：微妙us，转换为秒需要除以1000000）
    int audio_stream_idx = -1;
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }
    //获取到解码参数
    AVCodecParameters *codecpar = formatContext->streams[audio_stream_idx]->codecpar;
    //找到解码器
    AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
    //创建解码上下文
    AVCodecContext *codecContext = avcodec_alloc_context3(dec);
    //将解码参数填充到解码上下文中
    avcodec_parameters_to_context(codecContext, codecpar);
    avcodec_open2(codecContext, dec, NULL);
    //获取转换器上下文
    SwrContext *swrContext = swr_alloc();

    //输入的这些参数
    AVSampleFormat in_sample = codecContext->sample_fmt;
    //输入采样率
    int in_sample_rate = codecContext->sample_rate;
    //输入声道布局
    uint64_t in_ch_layout = codecContext->channel_layout;

    //输出参数------固定
    //输出采样格式
    AVSampleFormat out_sample = AV_SAMPLE_FMT_S16;
    //输出采样
    int out_sample_rate = 44100;
    //输出声道布局
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    //设置装换器的输入/输出参数
    swr_alloc_set_opts(swrContext, out_ch_layout, out_sample, out_sample_rate,
                       in_ch_layout, in_sample, in_sample_rate, 0, NULL);
    //设置参数之后，初始化转换器其他的默认参数
    swr_init(swrContext);

    //申请缓冲区，av_malloc(通道数*采样率)
    uint8_t *out_buffer = (uint8_t *) (av_malloc(2 * 44100));
    FILE *fp_pcm = fopen(output, "wb");

    //读取数据包
    AVPacket *packet = av_packet_alloc();
    int count = 0;
    while (av_read_frame(formatContext, packet) >= 0) {
        //将原始数据包数据作为输入提供给解码器
        avcodec_send_packet(codecContext, packet);
        //解压缩数据
        AVFrame *frame = av_frame_alloc();
        //内部将数据包(packet)中的数据解码出来，放到frame中
        int ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            LOGI("解码完成");
            break;
        }
        if (packet->stream_index != audio_stream_idx) {
            continue;
        }
        LOGI("正在解码 %d", count++);

        //-----------frame-->统一的格式--------
        //将一帧数据转换到输出缓冲区
        swr_convert(swrContext, &out_buffer, 2 * 44100,
                    (const uint8_t **) (frame->data), frame->nb_samples);

        int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
        int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb, frame->nb_samples,
                                                         out_sample, 1);
        fwrite(out_buffer, 1, out_buffer_size, fp_pcm);

    }
    fclose(fp_pcm);
    av_free(out_buffer);
    swr_free(&swrContext);
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);

    env->ReleaseStringUTFChars(input_, input);
    env->ReleaseStringUTFChars(output_, output);
}

//*********************直播Live**************************
LiveVideoChannel *live_video_channel = 0;
SafeQueue<RTMPPacket *> packets;
bool isStart;
pthread_t pid_start;
uint32_t  start_time;

void ReleaseRTMPPacket(RTMPPacket **packet) {
    if (*packet) {
        RTMPPacket_Free(*packet);
        delete *packet;
        *packet = 0;
    }
}

void callback(RTMPPacket *packet){
    if(packet){
        if (packet->m_nTimeStamp == -1){
            //需要时间戳，就设置一个时间戳
            packet->m_nTimeStamp = RTMP_GetTime() - start_time;
        }
        packets.push(packet);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_live_LivePusher_initNative(JNIEnv *env, jobject thiz) {
    live_video_channel = new LiveVideoChannel;
    live_video_channel->setVideoCallback(callback);
    packets.setReleaseCallback(ReleaseRTMPPacket);
}

void *task_start_live(void *args) {
    char *url = static_cast<char *>(args);
    RTMP *rtmp = 0;
    int ret;
    do {
        //1, rtmp 初始化
        rtmp = RTMP_Alloc();
        if (!rtmp) {
            LOGE("Live >>> rtmp 初始化失败");
            break;
        }

        RTMP_Init(rtmp);
        // 2, 设置流媒体地址
        ret = RTMP_SetupURL(rtmp, url);
        if (!ret) {
            LOGE("Live >>> 设置流媒体地址失败");
            break;
        }
        //3, 开启输出模式
        RTMP_EnableWrite(rtmp);
        //4, 建立连接
        ret = RTMP_Connect(rtmp, 0);
        if (!ret) {
            LOGE("Live >>> 建立连接失败");
            break;
        }
        //5, 建立流连接
        ret = RTMP_ConnectStream(rtmp, 0);
        if (!ret) {
            LOGE("Live >>> 建立流连接失败");
            break;
        }
        //跟服务器连通了
        //6, 记录开始推流的时间
        start_time =  RTMP_GetTime();
        packets.setWork(1);
        RTMPPacket *packet = 0;
        while (isStart) {
            int ret = packets.pop(packet);
            if(!isStart){
                break;
            }
            if (!ret){
                continue;
            }
            //7, 发送数据包
            packet->m_nInfoField2 = rtmp->m_stream_id;
            ret  = RTMP_SendPacket(rtmp, packet, 1);
            if (!ret){
                //发包失败，rtmp断开连接
                break;
            }
        }
        ReleaseRTMPPacket(&packet);
    } while (0);
    isStart = 0;
    packets.setWork(0);
    packets.clear();
    if (rtmp){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    delete (url);
    return 0;//记得return
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_live_LivePusher_startLiveNative(JNIEnv *env, jobject thiz, jstring path_) {
    if (isStart) {
        return;
    }
    isStart = 1;
    const char *path = env->GetStringUTFChars(path_, 0);
    //要进行服务器连接，创建子线程
    char *url = new char[strlen(path) + 1];// "\0"
    strcpy(url, path);
    pthread_create(&pid_start, 0, task_start_live, url);
    env->ReleaseStringUTFChars(path_, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_live_LivePusher_stopLiveNative(JNIEnv *env, jobject thiz) {
    isStart = 0;
    packets.setWork(0);
    pthread_join(pid_start, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_live_LivePusher_initVideoEncoderNative(JNIEnv *env, jobject thiz, jint width,
                                                        jint height, jint bitrate, jint fps) {
    if(live_video_channel){
        live_video_channel->initVideoEncoder(width, height, bitrate, fps);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_live_LivePusher_pushVideoNative(JNIEnv *env, jobject thiz, jbyteArray data_) {
    if(!live_video_channel || !isStart){
        return;
    }
    jbyte *data = env->GetByteArrayElements(data_, 0);
    live_video_channel->encodeData(data);
    env->ReleaseByteArrayElements(data_, data, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kelly_ffmpegplayer_live_LivePusher_releaseNative(JNIEnv *env, jobject thiz) {
    DELETE(live_video_channel);
}