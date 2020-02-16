package com.kelly.ffmpegplayer.live;

import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

import com.kelly.ffmpegplayer.R;

public class LiveActivity extends AppCompatActivity {
    //    private CameraHelper cameraHelper;
    private LivePusher mPusher;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_live);
        SurfaceView surfaceView = findViewById(R.id.surfaceView);
//        cameraHelper = new CameraHelper(this);
//        cameraHelper.setPreviewDisplay(surfaceView.getHolder());

        mPusher = new LivePusher(this);
        mPusher.setPreviewDisplay(surfaceView.getHolder());
    }

    /**
     * 转换摄像头
     *
     * @param view
     */
    public void switchCamera(View view) {
//        cameraHelper.switchCamera();
        mPusher.switchCamera();
    }

    /**
     * 开始直播
     * 查看推流状态：http://59.111.90.142:8080/stat
     * 直播服务器地址：rtmp://59.111.90.142/myapp/
     * @param view
     */
    public void startLive(View view) {
        mPusher.startLive("rtmp://59.111.90.142/myapp/");
    }

    /**
     * 停止直播
     *
     * @param view
     */
    public void stopLive(View view) {
        mPusher.stopLive();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mPusher.release();
    }
}
