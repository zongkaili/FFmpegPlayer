package com.kelly.ffmpegplayer

import android.annotation.SuppressLint
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.view.View
import android.widget.SeekBar
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import kotlinx.android.synthetic.main.activity_play.*
import java.io.File

class VideoPlayActivity : AppCompatActivity(), SeekBar.OnSeekBarChangeListener {
    private var mPlayer: MyPlayer? = null
    private var mSeekBar: SeekBar? = null //进度条-与播放总时长挂钩
    private var mTvTime: TextView? = null
    private var isTouch = false //用户是否拖拽了进度条
    private var isSeek = false//是否seek
    private var mDuration: Int = 0 //视频总时长

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_play)
        mSeekBar = findViewById(R.id.seekBar)
        mSeekBar?.setOnSeekBarChangeListener(this)
        mTvTime = findViewById(R.id.tvTime)
        mPlayer = MyPlayer()
        mPlayer?.setSurfaceView(surfaceView)
        mPlayer?.setDataSource(
            File(
                Environment.getExternalStorageDirectory(),
                "live.mp4"
            ).absolutePath
        )
        mPlayer?.setOnPrepareListener(object : MyPlayer.OnPrepareListener {
            @SuppressLint("SetTextI18n")
            override fun onPrepared() {
                //得到视频总时长
                mDuration = mPlayer?.getDuration()!!
                runOnUiThread {
                    //直播 通过ffmpeg 得到的 duration 是 0
                    if (mDuration != 0) { //本地视频文件
                        mTvTime?.text = "00:00/" + getMinutes(mDuration) + ":" + getSeconds(mDuration)
                        mTvTime?.visibility = View.VISIBLE
                        seekBar.visibility = View.VISIBLE
                    }
                    Toast.makeText(this@VideoPlayActivity, "开始播放", Toast.LENGTH_SHORT).show()
                }
                mPlayer?.start()
            }
        })
        mPlayer?.setOnErrorListener(object : MyPlayer.OnErrorListener {
            override fun onError(errorCode: Int) {
                runOnUiThread {
                    Toast.makeText(
                        this@VideoPlayActivity,
                        "出错啦，错误码 >>> $errorCode ",
                        Toast.LENGTH_SHORT
                    ).show()
                }
            }
        })
        mPlayer?.setOnProgressListener(object : MyPlayer.OnProgressListener {
            @SuppressLint("SetTextI18n")
            override fun onProgress(progress: Int) {
                // progress 底层ffmpeg 获取到的 当前播放时间（秒）
                if (!isTouch) {
                    runOnUiThread(Runnable {
                        if (mDuration != 0) {
                            if (isSeek) {
                                isSeek = false
                                return@Runnable
                            }
                            mTvTime?.text =
                                "${getMinutes(progress)}:${getSeconds(progress)}/${getMinutes(
                                    mDuration
                                )}:${getSeconds(mDuration)}"
                            seekBar.progress = progress * 100 / mDuration
                        }
                    })
                }
            }
        })

        // Example of a call to a native method
        sample_text.text = mPlayer?.stringFromJNI()
    }

    fun open(view: View) {
        val file = File(Environment.getExternalStorageDirectory(), "live.mp4")
        if (!file.exists()) {
            Log.d("FFmpeg_jni", " file do not exists.")
            return
        }
        mPlayer?.play(file.absolutePath)
    }

    fun audioTranscode(view: View) {
        val input = File(Environment.getExternalStorageDirectory(), "input.mp3").absolutePath
        val output =
            File(Environment.getExternalStorageDirectory(), "input_output.pcm").absolutePath
        mPlayer?.audioDecode(input, output)
    }

    private fun getMinutes(duration: Int): String? {
        val minutes = duration / 60
        return if (minutes <= 9) {
            "0$minutes"
        } else "" + minutes
    }

    private fun getSeconds(duration: Int): String? {
        val seconds = duration % 60
        return if (seconds <= 9) {
            "0$seconds"
        } else "" + seconds
    }

    override fun onResume() {
        super.onResume()
        mPlayer?.prepare()
    }

    override fun onStop() {
        super.onStop()
        mPlayer?.stop()
    }

    override fun onDestroy() {
        super.onDestroy()
        mPlayer?.release()
    }

    @SuppressLint("SetTextI18n")
    override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
        if (fromUser) {
            mTvTime?.text = getMinutes(progress * mDuration / 100) + ":" +
                    getSeconds(progress * mDuration / 100) + "/" +
                    getMinutes(mDuration) + ":" +
                    getSeconds(mDuration)
        }
    }

    override fun onStartTrackingTouch(seekBar: SeekBar?) {
        isTouch = true
    }

    override fun onStopTrackingTouch(seekBar: SeekBar?) {
        isTouch = false
        isSeek = false
        val seekBarProgress = seekBar!!.progress //seekbar 的进度
        val playProgress = seekBarProgress * mDuration / 100 //转成 播放时间
        mPlayer?.seek(playProgress)
    }
}
