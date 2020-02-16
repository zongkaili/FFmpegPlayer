package com.kelly.ffmpegplayer

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.kelly.ffmpegplayer.live.LiveActivity
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        btnPlayer.setOnClickListener { startActivity(Intent(this, VideoPlayActivity::class.java)) }
        btnLive.setOnClickListener { startActivity(Intent(this, LiveActivity::class.java)) }

    }
}
