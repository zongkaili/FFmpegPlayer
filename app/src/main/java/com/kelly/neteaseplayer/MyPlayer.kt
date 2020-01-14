package com.kelly.neteaseplayer

import android.text.TextUtils
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView

/**
 * author: zongkaili
 * data: 2019-11-15
 * desc:
 */
class MyPlayer: SurfaceHolder.Callback {

    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }

    private var onPrepareListener: OnPrepareListener? = null

    fun setOnPrepareListener(listener: OnPrepareListener) {
        this.onPrepareListener = listener
    }

    interface OnPrepareListener {
        fun onPrepared()
    }

    external fun stringFromJNI(): String
    external fun audioDecode(input:String, output: String)
    private external fun videoDecodeAndRender(path:String, surface: Surface)

    private external fun prepareNative(dataSource: String)
    private external fun startNative()
    private external fun stopNative()
    private external fun releaseNative()

    /**
     * jni回调方法
     */
    public fun onPrepared() {
        onPrepareListener?.onPrepared()
    }

    private var mSurfaceHolder: SurfaceHolder? = null

    fun setSurfaceView(surfaceView: SurfaceView) {
        mSurfaceHolder?.removeCallback(this)
        mSurfaceHolder = surfaceView.holder
        mSurfaceHolder?.addCallback(this)
    }

    override fun surfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int) {
        holder?.surface
    }

    override fun surfaceDestroyed(holder: SurfaceHolder?) {
    }

    override fun surfaceCreated(holder: SurfaceHolder?) {
    }

    fun play (path:String) {
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