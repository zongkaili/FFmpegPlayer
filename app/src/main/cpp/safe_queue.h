//
// Created by Administrator on 2020/1/16 0016.
//

#ifndef FFMPEGPLAYER_SAFE_QUEUE_H
#define FFMPEGPLAYER_SAFE_QUEUE_H

#include <queue>
#include <pthread.h>
using namespace std;

template <typename T>
/**
 * 线程安全的队列
 * @tparam T
 */
class SafeQueue {
    typedef void(*ReleaseCallback)(T *);
public:
    SafeQueue() {
       pthread_mutex_init(&mutex, 0);//动态初始化
       pthread_cond_init(&cond, 0);
    }
    ~SafeQueue() {
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    /**
     * 入队
     * @param value
     */
    void push(T value) {
        //线程安全  加锁
        pthread_mutex_lock(&mutex);
        if (work) {
            q.push(value);
            pthread_cond_signal(&cond);//发送通知，队列有数据了
        } else {
            //不在工作状态，需要释放, 交给调用者释放
            if (releaseCallback) {
                releaseCallback(&value);
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    /**
     * 出队
     * @param value
     * @return
     */
    int pop(T &value) {
        int ret = 0;
        pthread_mutex_lock(&mutex);
        //工作状态且队列没有数据，一直等待
        while (work && q.empty()) {
            pthread_cond_wait(&cond, &mutex);
        }
        if (!q.empty()) {
            value = q.front();
            q.pop();
            ret = 1;
        }
        pthread_mutex_unlock(&mutex);
        return ret;
    }

    void setWork(int work) {
        pthread_mutex_lock(&mutex);
        this->work = work;
        //work状态改变，也通知一下
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    int empty() {
        return q.empty();
    }

    int size() {
        return q.size();
    }

    void clear() {
        pthread_mutex_lock(&mutex);
        unsigned int size = q.size();
        for (int i = 0; i < size; ++i) {
            T value = q.front();
            //释放
            if (releaseCallback) {
                releaseCallback(&value);
            }
            q.pop();
        }
        pthread_mutex_unlock(&mutex);
    }

    void setReleaseCallback(ReleaseCallback releaseCallback) {
        this->releaseCallback = releaseCallback;
    }

private:
    queue <T> q;
    pthread_mutex_t mutex;//互斥锁
    pthread_cond_t cond;//条件变量
    int work;//标记当前队列是否是工作状态
    ReleaseCallback releaseCallback;
};

#endif //FFMPEGPLAYER_SAFE_QUEUE_H
