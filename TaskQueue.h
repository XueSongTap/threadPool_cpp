#pragma once
#include <queue>
#include <pthread.h>
//定义任务结构体
using callback = void(*)(void*);

template<typename T>
struct Task{
    Task(){
        function = nullptr;
        arg = nullptr;
    }
    Task(callback f, void* arg) {
        function = f;
        this -> arg  = (T*)arg;
    }
    callback function;
    T * arg;
};

// 任务队列
template<typename T>
class TaskQueue{
public:
    TaskQueue();
    ~TaskQueue();
    // 添加任务
    void addTask(Task<T> task);
    void addTask(callback func, void* arg);

    // 取出任务
    Task<T> takeTask();


    // 获取当前队列中任务个数
    inline int taskNumber(){
        return m_queue.size();
    }
private:
    // 互斥锁
    pthread_mutex_t m_mutex;
    // 任务队列
    std::queue<Task<T>> m_queue;
};