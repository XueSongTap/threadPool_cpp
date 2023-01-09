#include "ThreadPool.h"
#include "TaskQueue.h"
#include <iostream>
#include <string.h>
#include <string>
#include <unistd.h>

template <typename T>
ThreadPool<T>::ThreadPool(int minNum, int maxNum){
    //实例化任务队列
    m_taskQ = new TaskQueue<T>;
    do {
        m_minNum = minNum;
        m_maxNum = maxNum;
        m_busyNum = 0;
        m_aliveNum - minNum;

        m_threadIDs = new pthread_t[maxNum];
        if (m_threadIDs == nullptr){
            std::cout << "new threadIDs fail.." << std::endl;
            break;

        }

        memset(m_threadIDs, 0, sizeof(pthread_t) * maxNum);
        
        if (pthread_mutex_init(&m_lock, nullptr) != 0 || pthread_cond_init(& m_notEmpty, nullptr) != 0){
            std::cout << "mutx or condition init fail..." <<  std::endl;
            break;
        } 

        for ( int i = 0; i < minNum; i ++) {
            pthread_create(&m_threadIDs[i], nullptr, worker, this);
        }
        pthread_create(&m_managerID, NULL, manager, this); 

    }while (0);
}
template <typename T>
ThreadPool<T>::~ThreadPool() {
    m_shutdown = true;
    pthread_join(m_managerID, nullptr);
    for (int i = 0; i < m_aliveNum; ++ i) {
        pthread_cond_signal(&m_notEmpty);
    }
    //释放内存
    if (m_taskQ) {
        delete m_taskQ;
    }

    if (m_threadIDs) {
        delete[] m_threadIDs;
    }
    pthread_mutex_destroy(&m_lock);
    // ?
    pthread_cond_destroy(&m_notEmpty);
    
}
template <typename T>
void ThreadPool<T>::addTask(Task<T> task) {
    if (m_shutdown){
        pthread_mutex_unlock(&m_lock);
        return;
    }


    m_taskQ -> addTask(task);

    pthread_cond_signal(&m_notEmpty);
}


template <typename T>
int ThreadPool<T>::getAliveNum(){
    int threadNum = 0;
    pthread_mutex_lock(&m_lock);
    threadNum= m_aliveNum;
    pthread_mutex_unlock(&m_lock);
    return threadNum;
}
template <typename T>
int ThreadPool<T>::getBusyNum() {

    int busyNum = 0;
    pthread_mutex_lock(&m_lock);
    busyNum = m_busyNum;
    pthread_mutex_unlock(&m_lock);
    return busyNum;
}
template <typename T>
void* ThreadPool<T>::worker(void* arg) {
    ThreadPool *pool = static_cast<ThreadPool*>(arg);
    // 一直不停工作
    while (true) {
        pthread_mutex_lock(&pool->m_lock);
        while (pool -> m_taskQ -> taskNumber() == 0 && !pool -> m_shutdown) {
            pthread_cond_wait(&pool -> m_notEmpty, &pool -> m_lock);
            if (pool -> m_exitNum > 0) {
                pool -> m_exitNum --;
                if (pool -> m_aliveNum > pool -> m_minNum) {
                    pool -> m_aliveNum --;
                    pthread_mutex_unlock(&pool -> m_lock);
                    pool -> threadExit();
                }
            }
        }
        if (pool -> m_shutdown) {
            pthread_mutex_unlock(&pool -> m_lock);
            pool -> threadExit();
        }
        pool -> m_busyNum ++;

        Task<T> task = pool -> m_taskQ -> takeTask();
        pthread_mutex_unlock(&pool -> m_lock);

        std::cout << "thread " << std::to_string(pthread_self()) << "start workiong ..." << std::endl;

        task.function(task.arg);
        delete task.arg;
        task.arg = nullptr;

        std::cout << "thread "<< std::to_string(pthread_self()) <<  "end working ..." << std::endl;

        pthread_mutex_lock(&pool -> m_lock);
        pool -> m_busyNum --;
        pthread_mutex_unlock(&pool -> m_lock);
    }

    return nullptr;

}
template <typename T>
void* ThreadPool<T>::manager(void* arg) {
    ThreadPool * pool = static_cast<ThreadPool*>(arg);

    while (!pool -> m_shutdown) {
        sleep(3);
        
        // 取出忙的线程的数量
        pthread_mutex_lock(&pool -> m_lock);
        int queueSize = pool -> m_taskQ -> taskNumber();
        int liveNum = pool -> m_aliveNum;
        int busyNum = pool -> m_busyNum;

        pthread_mutex_unlock(&pool -> m_lock);

        const int NUMBER = 2;
        //添加线程
        if (queueSize > liveNum && liveNum < pool -> m_maxNum) {
            pthread_mutex_lock(&pool -> m_lock);
            int num = 0;
            for (int i = 0; i < pool -> m_maxNum && num < NUMBER && pool ->m_aliveNum < pool -> m_maxNum; ++ i) {
                if(pool -> m_threadIDs[i] == 0) {
                    pthread_create(&pool -> m_threadIDs[i], nullptr, worker, pool);
                    num ++;
                    pool -> m_aliveNum ++;
                }
            }
            pthread_mutex_unlock(&pool -> m_lock);
        }


        // 销毁线程
        if (busyNum * 2 < liveNum && liveNum > pool -> m_minNum) {
            pthread_mutex_lock(&pool -> m_lock);
            pool -> m_exitNum = NUMBER;
            pthread_mutex_unlock(&pool -> m_lock);
            
            for (int i = 0; i < NUMBER; i ++) {
                pthread_cond_signal(&pool -> m_notEmpty);
            }
        }

    }

    return nullptr;
}




template <typename T>
void ThreadPool<T>::threadExit() {
    pthread_t tid = pthread_self();
    for (int i = 0; i < m_maxNum; ++ i) {
        if (m_threadIDs[i] == tid) {
            m_threadIDs[i] = 0;
            std::cout << "threadExit() called, " << std::to_string(tid) << "exiting..." << std::endl;
            break;
        }
    }
    pthread_exit(nullptr);
}


