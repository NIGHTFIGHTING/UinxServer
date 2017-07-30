#include "commen.h"

//缓冲区
Storage *storage;
//生产者-线程
void *producer(void *args)
{
    int id = *(int *)args;
    delete (int *)args;

    while (1)
        storage->produce(id);   //生产
    return NULL;
}
//消费者-线程
void *consumer(void *args)
{
    int id = *(int *)args;
    delete (int *)args;

    while (1)
        storage->consume(id);   //消费
    return NULL;
}
//主控线程
int main()
{
    int nProducer = 1;
    int nConsumer = 2;
    cout << "please input the number of producer: ";
    cin >> nProducer;
    cout << "please input the number of consumer: ";
    cin >> nConsumer;
    cout << "please input the size of buffer: ";

    int size;
    cin >> size;
    storage = new Storage(size);

    pthread_t *thread = new pthread_t[nProducer+nConsumer];
    //创建消费者进程
    for (int i = 0; i < nConsumer; ++i)
        pthread_create(&thread[i], NULL, consumer, new int(i));
    //创建生产者进程
    for (int i = 0; i < nProducer; ++i)
        pthread_create(&thread[nConsumer+i], NULL, producer, new int(i));

    //等待线程结束
    for (int i = 0; i < nProducer+nConsumer; ++i)
        pthread_join(thread[i], NULL);

    delete storage;
    delete []thread;
}
