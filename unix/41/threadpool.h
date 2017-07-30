#ifndef __ICNLUDE_THREADPOOL_H__
#define __INCLUDE_THREADPOOL_H__


#include "condition.h"

//任务结构体，将任务放入队列由线程池中的线程来执行
typedef struct task
{
    void *(*run)(void *arg);  // 任务回调函数
    void *arg;		      // 回调函数参数
    struct task *next;
}task_t;

// 线程池结构体
typedef struct threadpool
{
    //任务准备就绪或者线程池销毁通知
    condition_t  ready;
    //任务队列头指针
    task_t *first;
    //任务队列尾指针
    task_t *last;
    //线程池中当前线程数
    int counter;
    //线程池中当前正在等待任务的线程数
    int idle;
    //线程池中最大允许线程数
    int max_threads;
    //销毁线程池的时候置1
    int quit;
}threadpool_t;

//初始化线程池
void threadpool_init(threadpool_t *pool,int threads);

// 往线程池中添加任务
void threadpool_add_task(threadpool_t *pool,void *(*run)(void *arg),void *arg);

//销毁线程池
void threadpool_destory(threadpool_t *pool);

#endif // __INCLUDE_THREADPOOL_H__
