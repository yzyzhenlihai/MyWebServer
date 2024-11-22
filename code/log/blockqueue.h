#ifndef _BLOCKQUEUE_H_
#define _BLOCKQUEUE_H_
#include<mutex>
#include<condition_variable>
#include<deque>
//定义阻塞队列，生产者消费者模式
template<class T>
class BlockQueue{
public:
    BlockQueue(size_t maxSize=1024);
    ~BlockQueue();
    bool pushBack(T item);//入队
    bool popFront(T &item);//出队
    size_t getSize();
    void Flush();
    void Close();
private:
    std::mutex mutex_;
    std::condition_variable notEmpty;
    std::condition_variable notFull;
    std::deque<T> blockQue_;//阻塞队列
    size_t maxSize_;//阻塞队列最大容量
    bool isClose_;//是否关闭
};

template<class T>
BlockQueue<T>::BlockQueue(size_t maxSize):maxSize_(maxSize){
    isClose_=false;
}

template<class T>
BlockQueue<T>::~BlockQueue(){Close();}

template<class T>
size_t BlockQueue<T>::getSize(){
    std::unique_lock<std::mutex> locker(mutex_);
    return blockQue_.size();
}
template<class T>
//入队尾
bool BlockQueue<T>::pushBack(T item){
    std::unique_lock<std::mutex> locker(mutex_);
    while(blockQue_.size()>=maxSize_){
        notFull.wait(locker);
        if(isClose_)return false;
    }
    blockQue_.push_back(item);
    notEmpty.notify_one();
    return true;
}

template<class T>
//出队头
bool BlockQueue<T>::popFront(T &item){
    std::unique_lock<std::mutex> locker(mutex_);
    while(blockQue_.size()==0){
        notEmpty.wait(locker);
        if(isClose_)return false;
    }
    item=blockQue_.front();
    blockQue_.pop_front();
    notFull.notify_one();
    return true;
}

template<class T>
void BlockQueue<T>::Flush(){
    notEmpty.notify_one();//唤醒阻塞的消费者
}
template<class T>
void BlockQueue<T>::Close(){
    std::unique_lock<std::mutex> locker(mutex_);
    isClose_=true;
    notEmpty.notify_all();
    notFull.notify_all();
}
#endif