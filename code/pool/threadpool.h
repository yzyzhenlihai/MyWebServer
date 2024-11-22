#ifndef _THREADPOLL_H_
#define _THREADPOOL_H_
#include<iostream>
#include<thread>
#include<vector>
#include<mutex>
#include<condition_variable>
#include<queue>
#include<memory>
#include<functional>
class ThreadPool{
public:
    ThreadPool(int threadNum):pool_(std::make_shared<Pool>()){
            //创建线程
        for(int i=0;i<threadNum;i++){
            //匿名函数创建线程
            std::thread([pool=pool_]{
                //创建锁
                try{
                    std::unique_lock<std::mutex> lock(pool->mutex_);
                    while(!pool->isClose_){
                        if(!pool->requestQueue_.empty()){
                            //取出任务
                            //std::cout<<"thread is executing task"<<std::endl;
                            auto task = std::move(pool->requestQueue_.front());//转为右值
                            pool->requestQueue_.pop();
                            lock.unlock();
                            task();
                            //执行完任务重新抢锁
                            lock.lock();
                        }else if(pool->isClose_){
                            break;
                        }else{
                            //此时任务队列为空
                            //std::cout<<"thread is waiting for condition"<<std::endl;
                            pool->notEmpty_.wait(lock);
                        }
                    }
                }catch(const std::system_error& e){
                    std::cerr<<"Thread---System error:"<<e.what()<<std::endl;
                }
                
            }).detach();
            
            /*(catch(const std::system_error& e){
                std::cerr<<"System error:"<<e.what()<<std::endl;
            }*/
            
        }
        std::cout<<"ThreadPool is created"<<std::endl;
    }
    ~ThreadPool(){
        if(static_cast<bool>(pool_)){
            {
                    std::unique_lock<std::mutex> lock(pool_->mutex_);
                    pool_->isClose_=true;
            }
            pool_->notEmpty_.notify_all();//唤醒所有线程
        }
    }
    ThreadPool() = default;
    ThreadPool(ThreadPool&&) = default;
    template<class F>
    void AddTask(F&& task){
        try{
            if(pool_==nullptr){
                std::cout<<"pool is nullptr"<<std::endl;
                return;
            }
            std::unique_lock<std::mutex> lock(pool_->mutex_);
            //添加任务
            pool_->requestQueue_.push(std::forward<F>(task));//完美转发,右值引用可以避免拷贝
        }catch(const std::system_error &e){
            std::cout<<"AddTask---syserror:"<<e.what()<<std::endl;
        }
        pool_->notEmpty_.notify_one();
    }
private:
    struct Pool
    {   
        std::mutex mutex_;
        std::condition_variable notEmpty_;
        std::condition_variable notFull_;
        std::queue<std::function<void()>> requestQueue_;//任务请求队列 
        bool isClose_;//是否被关闭
        
    };
    
    std::shared_ptr<Pool> pool_;//统一管理锁和任务请求队列
    
};

#endif