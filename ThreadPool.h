#pragma once
#include "TaskQueue.h"
#include "TaskQueue.cc"
#include <pthread.h>

template <typename T>
class ThreadPool {
public:
    ThreadPool(int min, int max);
    ~ThreadPool();

    void addTask(Task<T> task);

    int getBusyNum();

    int getAliveNum();


private:
    static void* worker(void* arg);
    static void* manager(void* arg);
    void threadExit();

    pthread_mutex_t m_lock;
    pthread_cond_t m_notEmpty;
    TaskQueue<T>* m_taskQ;
    pthread_t m_managerID;
    pthread_t* m_threadIDs;
    int m_minNum;
    int m_maxNum;
    int m_busyNum;
    int m_aliveNum;
    int m_exitNum;

    bool m_shutdown = false;

};
