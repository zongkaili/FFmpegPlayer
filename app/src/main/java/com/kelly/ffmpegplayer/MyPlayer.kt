package com.kelly.ffmpegplayer

import android.text.TextUtils
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView

/**
 * author: zongkaili
 * data: 2019-11-15
 * desc:
 */
class MyPlayer : SurfaceHolder.Callback {

    companion object {
        //准备过程错误码
        private const val ERROR_CODE_FFMPEG_PREPARE = 1000
        //播放过程错误码
        private const val ERROR_CODE_FFMPEG_PLAY = 2000

        //打不开媒体数据源
        const val FFMPEG_CAN_NOT_OPEN_URL = ERROR_CODE_FFMPEG_PREPARE - 1
        //找不到媒体流信息
        const val FFMPEG_CAN_NOT_FIND_STREAMS = ERROR_CODE_FFMPEG_PREPARE - 2
        //找不到解码器
        const val FFMPEG_FIND_DECODER_FAIL = ERROR_CODE_FFMPEG_PREPARE - 3
        //无法根据解码器创建上下文
        const val FFMPEG_ALLOC_CODEC_CONTEXT_FAIL = ERROR_CODE_FFMPEG_PREPARE - 4
        //根据留信息 配置上下文参数失败
        const val FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL = ERROR_CODE_FFMPEG_PREPARE - 5
        //打开解码器失败
        const val FFMPEG_OPEN_DECODER_FAIL = ERROR_CODE_FFMPEG_PREPARE - 6
        //没有音视频
        const val FFMPEG_NO_MEDIA = ERROR_CODE_FFMPEG_PREPARE - 7
        //读取媒体数据包失败
        const val FFMPEG_READ_PACKETS_FAIL = ERROR_CODE_FFMPEG_PLAY - 1

        init {
            System.loadLibrary("native-lib")
        }
    }

    private var onPrepareListener: OnPrepareListener? = null
    private var onErrorListener: OnErrorListener? = null

    fun setOnPrepareListener(listener: OnPrepareListener) {
        this.onPrepareListener = listener
    }

    fun setOnErrorListener(listener: OnErrorListener) {
        this.onErrorListener = listener;
    }

    interface OnPrepareListener {
        fun onPrepared()
    }

    interface OnErrorListener {
        fun onError(errorCode: Int)
    }

    external fun stringFromJNI(): String
    external fun audioDecode(input: String, output: String)
    private external fun videoDecodeAndRender(path: String, surface: Surface)

    private external fun prepareNative(dataSource: String)
    private external fun startNative()
    private external fun stopNative()
    private external fun releaseNative()

    private external fun setSurfaceNative(surface: Surface)

    /**
     * jni回调方法
     */
    public fun onPrepared() {
        onPrepareListener?.onPrepared()
    }

    public fun onError(errorCode: Int) {
        onErrorListener?.onError(errorCode)
    }

    private var mSurfaceHolder: SurfaceHolder? = null

    fun setSurfaceView(surfaceView: SurfaceView) {
        mSurfaceHolder?.removeCallback(this)
        mSurfaceHolder = surfaceView.holder
        mSurfaceHolder?.addCallback(this)
    }

    override fun surfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int) {
        setSurfaceNative(holder?.surface!!)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder?) {
    }

    override fun surfaceCreated(holder: SurfaceHolder?) {
    }

    fun play(path: String) {
        if (TextUtils.isEmpty(path) || mSurfaceHolder?.surface == null) {
            return
        }
        videoDecodeAndRender(path, mSurfaceHolder?.surface!!)
    }

    private var dataSource: String = ""

    fun setDataSource(dataSource: String) {
        this.dataSource = dataSource;
    }

    fun prepare() {
        prepareNative(dataSource)
    }

    fun start() {
        startNative()
    }

    fun stop() {
        stopNative()
    }

    fun release() {
        releaseNative()
    }

}