//
// Created by 宗开黎 on 2020/3/7.
//

#include "LiveAudioChannel.h"
#include "macro.h"
#include "libfaac/include/faac.h"
#include "../../../../../../../Library/Android/sdk/ndk-bundle/sysroot/usr/include/sys/types.h"


LiveAudioChannel::LiveAudioChannel() {}

LiveAudioChannel::~LiveAudioChannel() {
//   todo 释放
    DELETE(buffer);
    if (audioEncoder) {
        faacEncClose(audioEncoder);
        audioEncoder = 0;
    }
}

void LiveAudioChannel::initAudioEncoder(int sample_rate, int channels) {
    mChannels = channels;

    /**
     * inputSamples: 编码器每次进行编码时接收的最大样本数
     * maxOutputBytes: 编码器最大输出数据个数（字节数）
     */
    audioEncoder = faacEncOpen(sample_rate, channels, &inputSamples, &maxOutputBytes);

    //获取当前编码器的配置
    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(audioEncoder);

    config->mpegVersion = MPEG4;//指定使用MPEG-4标准
    config->aacObjectType = LOW;//LC 低复杂度标准
    config->inputFormat = FAAC_INPUT_16BIT;//16位
    config->outputFormat = 0;//需要原始数据，而不是adts
    config->useTns = 1;//降噪
    config->useLfe = 0;//环绕

    int ret = faacEncSetConfiguration(audioEncoder, config);
    if (!ret) {
        LOGE("Live >>> 音频编码器配置失败");
        return;
    }
    //初始化输出缓冲区
    buffer = new u_char[maxOutputBytes];
}

void LiveAudioChannel::encodeData(int8_t *data) {
    //faac编码，返回编码后数据的字节长度
    int byteLen = faacEncEncode(audioEncoder, reinterpret_cast<int32_t *>(data), inputSamples, buffer, maxOutputBytes);

    if (byteLen > 0) {
        LOGE("Live >>> 音频编码成功");
        //组RTMP包
        RTMPPacket *packet = new RTMPPacket;
        int body_size = 1 + byteLen;
        RTMPPacket_Alloc(packet, body_size);

        packet->m_body[0] = 0xAF;//双声道
        if (mChannels == 1) {
            packet->m_body[0] = 0xAE;//单声道
        }

        packet->m_body[1] = 0x01;

        memcpy(&packet->m_body[2], buffer, byteLen);

        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
        packet->m_nBodySize = body_size;
        packet->m_nTimeStamp = -1;
        packet->m_hasAbsTimestamp = 1;
        packet->m_nChannel = 11;
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;

        audioCallback(packet);
    }
}

int LiveAudioChannel::getInputSamples() {
    return inputSamples;
}

void LiveAudioChannel::setAudioCallback(LiveAudioChannel::AudioCallback callback) {
    this->audioCallback = callback;
}

RTMPPacket *LiveAudioChannel::getAudioSeqHeader() {
    u_char *ppBuffer;
    u_long byteLen;
    faacEncGetDecoderSpecificInfo(audioEncoder, &ppBuffer, &byteLen);

    //组RTMP包
    RTMPPacket *packet = new RTMPPacket;
    int body_size = 2 + byteLen;
    RTMPPacket_Alloc(packet, body_size);

    packet->m_body[0] = 0xAF;//双声道
    if(mChannels == 1){
        packet->m_body[0] = 0xAE;//单声道
    }

    packet->m_body[1] = 0x00;//序列头配置信息为 0x00

    memcpy(&packet->m_body[2], ppBuffer, byteLen);

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = body_size;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 1;
    packet->m_nChannel = 11;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;

    return packet;
}