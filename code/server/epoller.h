#ifndef _EPOLLER_H_
#define _EPOLLER_H_
#include<sys/epoll.h>
#include<vector>
#include<assert.h>
class Epoller{
public:
    Epoller(int maxevents);
    int getEpfd();
    bool AddFd(int fd,uint32_t events);//添加监听
    bool ModFd(int fd,uint32_t events);//修改监听
    bool DelFd(int fd);//删除监听
    int GetEventFd(int loc);
    uint32_t GetEvent(int loc);
    int Wait(int timeout);//开始监听
private:
    
    int epfd_;
    std::vector<epoll_event> events_;//存监听到的事件
    
};

#endif