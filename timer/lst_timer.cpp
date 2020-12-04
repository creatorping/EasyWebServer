#include "lst_timer.h"
#include "../http/http_conn.h"

sort_timer_lst::sort_timer_lst()
{
    head = NULL;
    tail = NULL;
}
sort_timer_lst::~sort_timer_lst()
{
    util_timer *tmp = head;
    while(tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp=head;
    }
}
//添加一个util_timer到链表中
void sort_timer_lst::add_timer(util_timer *timer)
{
    if(!timer)
    {
        return ;
    }
    if(!head)
    {
        head=tail=timer;
        return ;
    }
    if(timer->expire < head->expire)
    {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer,head);
}
//更新修改后的timer在链表中的位置
void sort_timer_lst::adjust_timer(util_timer *timer)
{
    if(!timer)
    {
        return ;
    }
    util_timer *tmp = timer->next;
    if(!tmp||(timer->expire < tmp -> expire))
    {
        return;
    }
    if(timer == head)
    {
        head == head->next;
        head -> prev = NULL;
        timer -> next = NULL;
        add_timer(timer,head);
    }
    else
    {
        timer->prev->next=timer->next;
        timer->next->prev=timer->prev;
        add_timer(timer,timer->next);
    }
}
void sort_timer_lst::del_timer(util_timer *timer)
{
    if(!timer)
    {
        return ;
    }
    if((timer==head) && (timer==tail))
    {
        delete timer;
        head=NULL;
        tail=NULL;
        return ;
    }
    if(timer==head)
    {
        head=head->next;
        head->prev = NULL;
        delete timer;
        return ;
    }
    if(timer == tail)
    {
        tail=tail->prev;
        tail->next=NULL;
        delete timer;
        return ;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}
//清楚链表中过期的socket
void sort_timer_lst::tick()
{
    if(!head)
    {
        return ;
    }
    time_t cur = time(NULL);
    util_timer *tmp = head;
    while(tmp)
    {
        if(cur<tmp->expire)
        {
            break;
        }
        tmp->cb_func(tmp->user_data);//清除过期socket，关闭socket连接
        head = tmp->next;
        if(head)
        {
            head->prev=NULL;
        }
        delete tmp;
        tmp=head;
    }
}
//在链表中找适合位置，将timer添加到链表中
void sort_timer_lst::add_timer(util_timer *timer,util_timer *lst_head)
{
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
    while(tmp)
    {
        if(timer->expire < tmp->expire)
        {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev=prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if(!tmp)
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}
void utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int utils::setnonblockint(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void utils::addfd(int epollfd,int fd,bool one_shot,int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;
    if(1 == TRIGMode)//开启ET模式
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if(one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblockint(fd);
}

//信号处理函数
void utils::sig_handler(int sig)
{
    //为保证函数的可入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1],(char *)&msg,1,0);
    errno = save_errno;
}

//设置信号函数
void utils::addsig(int sig,void(handler)(int),bool restart)
{
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = handler;
    if(restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)!=-1);
}
//定时处理任务，重新定时以不断触发SIGALRM信号
void utils::timer_handler()
{
    m_timer_lst.tick();
    alarm(m_TIMESLOT);
}

void utils::show_error(int connfd,const char *info)
{
    send(connfd,info,strlen(info),0);
    close(connfd);
}

int *utils::u_pipefd = 0;
int utils::u_epollfd = 0;

class utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(utils::u_epollfd,EPOLL_CTL_DEL,user_data->sockfd,0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}
