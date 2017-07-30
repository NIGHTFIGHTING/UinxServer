#ifndef COMMEN_H_INCLUDED
#define COMMEN_H_INCLUDED

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <memory>
#include <iostream>
using namespace std;

class Storage
{
public:
    Storage(unsigned int _bufferSize);
    ~Storage();

    void consume(int id);   //消费
    void produce(int id);   //生产

private:
    // 打印缓冲区状态
    void display(bool isConsumer = false);

private:
    unsigned int buffSize;
    int *m_storage; //缓冲区

    unsigned short int in;  //生产位置
    unsigned short int out; //消费位置
    unsigned int product_number;    //产品编号

    sem_t sem_full; //满信号量
    sem_t sem_empty;//空信号量
    pthread_mutex_t mutex;  //互斥量: 保护缓冲区互斥访问
};

Storage::Storage(unsigned int _bufferSize)
    :buffSize(_bufferSize), in(0), out(0), product_number(0)
{
    m_storage = new int[buffSize];
    for (unsigned int i = 0; i < buffSize; ++ i)
        m_storage[i] = -1;

    sem_init(&sem_full, 0, 0);
    //将empty信号量初始化为缓冲区大小
    sem_init(&sem_empty, 0, buffSize);
    pthread_mutex_init(&mutex, NULL);
}

Storage::~Storage()
{
    delete []m_storage;

    pthread_mutex_destroy(&mutex);
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
}

void Storage::produce(int id)
{
    printf("producer %d is waiting storage not full\n", id);
    //获取empty信号量
    sem_wait(&sem_empty);
    //获取互斥量
    pthread_mutex_lock(&mutex);

    //生产
    cout << "++ producer " << id << " begin produce "
         << ++product_number << " ..." << endl;
    m_storage[in] = product_number;
    //打印此时缓冲区状态
    display(false);
    in = (in+1)%buffSize;
    cout << "   producer " << id << " end produce ...\n" << endl;

    //释放互斥量
    pthread_mutex_unlock(&mutex);
    //释放full信号量
    sem_post(&sem_full);
    sleep(1);
}
void Storage::consume(int id)
{
    printf("consumer %d is waiting storage not empty\n", id);
    //获取full信号量
    sem_wait(&sem_full);
    //获取互斥量
    pthread_mutex_lock(&mutex);

    //消费
    int consume_id = m_storage[out];
    cout << "-- consumer " << id << " begin consume "
         << consume_id << " ..." << endl;
    m_storage[out] = -1;
    //打印此时缓冲区状态
    display(true);
    out = (out+1)%buffSize;
    cout << "   consumer " << id << " end consume ...\n" << endl;

    //解锁互斥量
    pthread_mutex_unlock(&mutex);
    //释放empty信号量
    sem_post(&sem_empty);
    sleep(1);
}
void Storage::display(bool isConsme)
{
    cout << "states: { ";
    for (unsigned int i = 0; i < buffSize; ++i)
    {
        if (isConsme && out == i)
            cout << '#';
        else if (!isConsme && in == i)
            cout << '*';

        if (m_storage[i] == -1)
            cout << "null ";
        else
            printf("%-4d ", m_storage[i]);
    }
    cout << "}" << endl;
}

inline void err_exit(const std::string &msg)
{
    perror(msg.c_str());
    exit(EXIT_FAILURE);
}
inline void err_quit(const std::string &msg)
{
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}
inline void err_thread(const std::string &msg, int retno)
{
    std::cerr << msg << ": " << strerror(retno) << std::endl;
    exit(EXIT_FAILURE);
}
inline void err_check(const std::string &msg, int retno)
{
    if (retno != 0)
        err_thread(msg, retno);
}

#endif // COMMEN_H_INCLUDED
