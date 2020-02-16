package com.kelly.ffmpegplayer.live;

import android.app.Activity;
import android.view.SurfaceHolder;

public class LivePusher {
    private final VideoChannel mVideoChannel;
    private final AudioChannel mAudioChannel;

    static {
        System.loadLibrary("native-lib");
    }

    public LivePusher(Activity activity) {
        initNative();
        mVideoChannel = new VideoChannel(activity, this);
        mAudioChannel = new AudioChannel();
    }

    public void switchCamera() {
        mVideoChannel.switchCamera();
    }

    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        mVideoChannel.setPreviewDisplay(surfaceHolder);
    }

    /**
     * 开始直播
     * 先准备好编码器
     *
     * @param path rtmp推流地址  rtmp://59.111.90.142/myapp/
     */
    public void startLive(String path) {
        startLiveNative(path);
        mVideoChannel.startLive();
        mAudioChannel.startLive();
    }

    /**
     * 停止直播
     */
    public void stopLive() {
        mVideoChannel.stopLive();
        mAudioChannel.stopLive();
        stopLiveNative();
    }

    /**
     * 释放
     */
    public void release() {
        mVideoChannel.release();
        mAudioChannel.release();
        releaseNative();
    }

    /**
     * 初始化，准备队列
     */
    private native void initNative();


    /**
     * 建立服务器连接 开子线程，循环从队列中读取 RTMP 包
     *
     * @param path
     */

    private native void startLiveNative(String path);

    /**
     * 停止直播，断开服务器连接，跳出子线程循环
     */
    private native void stopLiveNative();

    /**
     * 初始化视频编码器（设置视频相关参数：width、height、bitrate、fps）
     */
    public native void initVideoEncoderNative(int width, int height, int bitrate, int fps);

    /**
     * 推送视频数据（进行H.264编码）
     *
     * @param data
     */
    public native void pushVideoNative(byte[] data);

    /**
     * 释放
     */
    public native void releaseNative();

}
