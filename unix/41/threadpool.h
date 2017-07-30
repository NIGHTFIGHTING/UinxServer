#ifndef __ICNLUDE_THREADPOOL_H__
#define __INCLUDE_THREADPOOL_H__


#include "condition.h"

//����ṹ�壬���������������̳߳��е��߳���ִ��
typedef struct task
{
    void *(*run)(void *arg);  // ����ص�����
    void *arg;		      // �ص���������
    struct task *next;
}task_t;

// �̳߳ؽṹ��
typedef struct threadpool
{
    //����׼�����������̳߳�����֪ͨ
    condition_t  ready;
    //�������ͷָ��
    task_t *first;
    //�������βָ��
    task_t *last;
    //�̳߳��е�ǰ�߳���
    int counter;
    //�̳߳��е�ǰ���ڵȴ�������߳���
    int idle;
    //�̳߳�����������߳���
    int max_threads;
    //�����̳߳ص�ʱ����1
    int quit;
}threadpool_t;

//��ʼ���̳߳�
void threadpool_init(threadpool_t *pool,int threads);

// ���̳߳����������
void threadpool_add_task(threadpool_t *pool,void *(*run)(void *arg),void *arg);

//�����̳߳�
void threadpool_destory(threadpool_t *pool);

#endif // __INCLUDE_THREADPOOL_H__
