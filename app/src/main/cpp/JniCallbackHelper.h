//
// Created by 宗开黎 on 2020-01-14.
//

#ifndef NETEASEPLAYER_JNICALLBACKHELPER_H
#define NETEASEPLAYER_JNICALLBACKHELPER_H

#include <jni.h>

class JniCallbackHelper {
public:
    JniCallbackHelper(JavaVM *javaVm, JNIEnv *env, jobject pJobject);

    ~JniCallbackHelper();

    void onPrepared(int thread_mode);

private:
    JNIEnv *env = 0;
    jobject instance;
    jmethodID jmd_prepared;
    JavaVM *javaVM = 0;
};


#endif //NETEASEPLAYER_JNICALLBACKHELPER_H