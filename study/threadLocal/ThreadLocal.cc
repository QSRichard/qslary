#include <iostream>
#include <pthread.h>
#include <unistd.h>


// 测试结果 main thread1 以及thread2 三个线程中i的地址以及值都不一样

__thread int i = 0;

void *f1(void *arg)
{
    i++;
    printf("f1 i address %p val %d\n", &i, i);
    return nullptr;
}

void *f2(void *arg)
{
    i += 2;
    printf("f2 i address %p val %d\n", &i, i);
    return nullptr;
}

int main()
{
    pthread_t pid1, pid2;
    i += 3;
    pthread_create(&pid1, NULL, f1, NULL);
    pthread_create(&pid2, NULL, f2, NULL);
    pthread_join(pid1, NULL);
    pthread_join(pid2, NULL);
    printf("main i address %p val %d\n", &i, i);

    return 0;
}