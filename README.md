# FFmpegPlayer
FFmpeg 编解码，直播推流学习demo

### 此demo包含功能：
 - FFmpeg播放器：集成FFmpeg库，jni调用库函数编解码音视频，播放视频。
   
   注：用demo播放视频需要将mp4视频文件拷贝到手机外部存储根目录，命名为“live.mp4”,既可播放，也可自行选择文件目录，然后在demo中把路径更改该路径即可。
   
 - 直播：rtmp推流 & x264编解码，jni实现
   
   注：此功能依赖于服务器，所以需要搭建服务器环境，搭建流程可参考：https://www.jianshu.com/p/cf7f0552ffe9，
   此功能可用于学习rtmp直播推流和x264编解码的jni实现逻辑。
   推流成功后，可用FFmpeg命令播放服务器视频流：ffplay -i rtmp://xxx.xxx.xxx.xxx/myapp/
