//
// Created by 宗开黎 on 2020-01-14.
//

#include <android/log.h>

#ifndef FFMPEGPLAYER_MACRO_H
#define FFMPEGPLAYER_MACRO_H

#define THREAD_MAIN 1
#define THREAD_CHILD 2

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO, "kelly", FORMAT, ##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR, "kelly", FORMAT, ##__VA_ARGS__);
#define DELETE(object) if (object) { delete object; object = 0;}

//错误代码
#define ERROR_CODE_OK 0
#define ERROR_CODE_FFMPEG_PREPARE -1000
#define ERROR_CODE_FFMPEG_PLAY -2000

//打不开媒体数据源
#define FFMPEG_CAN_NOT_OPEN_URL (ERROR_CODE_FFMPEG_PREPARE - 1)
//找不到媒体流信息
#define FFMPEG_CAN_NOT_FIND_STREAMS (ERROR_CODE_FFMPEG_PREPARE - 2)
//找不到解码器
#define FFMPEG_FIND_DECODER_FAIL (ERROR_CODE_FFMPEG_PREPARE - 3)
//无法根据解码器创建上下文
#define FFMPEG_ALLOC_CODEC_CONTEXT_FAIL (ERROR_CODE_FFMPEG_PREPARE - 4)
//根据留信息 配置上下文参数失败
#define FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL (ERROR_CODE_FFMPEG_PREPARE - 5)
//打开解码器失败
#define FFMPEG_OPEN_DECODER_FAIL (ERROR_CODE_FFMPEG_PREPARE - 6)
//没有音视频
#define FFMPEG_NO_MEDIA (ERROR_CODE_FFMPEG_PREPARE - 7)

//读取媒体数据包失败
#define FFMPEG_READ_PACKETS_FAIL (ERROR_CODE_FFMPEG_PLAY - 1)

#endif //FFMPEGPLAYER_MACRO_H
