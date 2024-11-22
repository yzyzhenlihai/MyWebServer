#include"epoller.h"

Epoller::Epoller(int maxevent):events_(maxevent){
    epfd_=epoll_create(1);
}

bool Epoller::AddFd(int fd,uint32_t events){
    if(fd<0) return false;
    epoll_event event;
    event.events=events;
    event.data.fd=fd;
    if(epoll_ctl(epfd_,EPOLL_CTL_ADD,fd,&event)<0) return false;

    return true;
}

bool Epoller::ModFd(int fd,uint32_t events){
    if(fd<0) return false;
    epoll_event event;
    event.events=events;
    event.data.fd=fd;
    if(epoll_ctl(epfd_,EPOLL_CTL_MOD,fd,&event)<0) return false;

    return true;
}

bool Epoller::DelFd(int fd){

    if(fd<0) return false;
    epoll_event event={0};
    if(epoll_ctl(epfd_,EPOLL_CTL_DEL,fd,&event)<0) return false;
    return true;
}

int Epoller::getEpfd(){
    return epfd_;
}
int Epoller::Wait(int timeout){
    //开始监听
    int requestNum=epoll_wait(epfd_,&events_[0],static_cast<int>(events_.size()),timeout);
    return requestNum;
}
int Epoller::GetEventFd(int loc){
    return events_[loc].data.fd;       
}
uint32_t Epoller::GetEvent(int loc){
    return events_[loc].events;
}