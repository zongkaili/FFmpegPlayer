//
// Created by 宗开黎 on 2020/2/15 0015.
//

#ifndef FFMPEGPLAYER_LIVEVIDEOCHANNEL_H
#define FFMPEGPLAYER_LIVEVIDEOCHANNEL_H

#include <cstdint>
#include <x264.h>
#include <rtmp.h>

class LiveVideoChannel {
    typedef void(*VideoCallback)(RTMPPacket *packet);
public:
    LiveVideoChannel();

    virtual ~LiveVideoChannel();

    void initVideoEncoder(int width, int height, int bitrate, int fps);

    void encodeData(int8_t *data);

    void setVideoCallback(VideoCallback callback);

private:
    pthread_mutex_t mutex;
    int mWidth;
    int mHeight;
    int mBitrate;
    int mFps;
    x264_t *videoEncoder = 0;
    x264_picture_t *pic_in= 0;
    int y_len;
    int uv_len;
    VideoCallback videoCallback;
    void senSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len);

    void sendFrame(int type, uint8_t *payload, int iPayload);
};


#endif //FFMPEGPLAYER_LIVEVIDEOCHANNEL_H
