package com.kelly.neteaseplayer

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.view.View
import android.widget.Toast
import kotlinx.android.synthetic.main.activity_main.*
import java.io.File

class MainActivity : AppCompatActivity() {
    private var mPlayer: MyPlayer? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        mPlayer = MyPlayer()
        mPlayer?.setSurfaceView(surfaceView)
        mPlayer?.setDataSource(File(Environment.getExternalStorageDirectory() , "/live.mp4").absolutePath)
        mPlayer?.setOnPrepareListener(object : MyPlayer.OnPrepareListener {
            override fun onPrepared() {
                Toast.makeText(this@MainActivity, "媒体已准备好了", Toast.LENGTH_SHORT).show()
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
        val output = File(Environment.getExternalStorageDirectory(), "input_output.pcm").absolutePath
        mPlayer?.audioDecode(input, output)
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
}
