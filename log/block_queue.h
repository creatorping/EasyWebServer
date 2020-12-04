#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"
using namespace std;

template <class T>
class block_queue
{
public:
    block_queue(int max_size = 1000)
    {
        if(max_size <= 0)
        {
            exit(-1);
        }
        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }
    void clear()
    {
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }
    ~block_queue()
    {
        m_mutex.lock();
        if(m_array != NULL)
        {
            delete[] m_array;
        }
        m_mutex.unlock();
    }
    //判断队列是否满了
    bool full()
    {
        bool flag = false;
        m_mutex.lock();
        if(m_size >= m_max_size)
        {
            flag = true;
        }
        m_mutex.unlock();
        return flag;
    }
    //判断队列是否为空
    bool empty()
    {
        bool flag = false;
        m_mutex.lock();
        if(m_size == 0)
        {
            flag = true;
        }
        m_mutex.unlock();
        return false;
    }
    bool front(T &value)
    {
        m_mutex.lock();
        if(0 == m_size)
        {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }
    //返回队尾元素
    bool back(T &value)
    {
        m_mutex.lock();
        if(0 == m_size)
        {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        return true;
    }
    //返回队列中元素个数
    int size()
    {
        int tmp=0;
        m_mutex.lock();
        tmp=m_size;
        m_mutex.unlock();
        return tmp;
    }
    int max_size()
    {
        int tmp=0;
        m_mutex.lock();
        tmp=m_max_size;
        m_mutex.unlock();
        return tmp;
    }
    //往队列中添加元素，使用生产者和消费者模型
    bool push(const T &item)
    {
        m_mutex.lock();
        if(m_size>=m_max_size)
        {
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        m_back = (m_back+1)%m_max_size;
        m_array[m_back] = item;
        m_size++;
        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }
    //取出队列头元素，使用生产者和消费者模型
    bool pop(T &item)
    {
        m_mutex.lock();
        while(m_size<=0)
        {
            if(!m_cond.wait(m_mutex.get()))
            {
                m_mutex.unlock();
                return false;
            }
        }
        m_front = (m_front+1)%m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }
    //添加超时处理
    bool pop(T &item,int ms_time)//毫秒为单位
    {
        struct timespec t = {0,0};
        clock_gettime(CLOCK_REALTIME,&t);
        m_mutex.lock();
        if(m_size <= 0)
        {
            t.tv_sec +=  ms_time/1000;
            t.tv_nsec += (ms_time%1000)*1000000;
            if(!m_cond.timewait(m_mutex.get(),t))
            {
                m_mutex.unlock();
                return false;
            }
        }
        m_front = (m_front+1)%m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }
private:
    locker m_mutex;
    cond m_cond;

    T *m_array;
    int m_size;//已用容量
    int m_max_size;//最大容量
    int m_front;//队首
    int m_back;//队尾
};
#endif
