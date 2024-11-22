#ifndef _TIMER_H_
#define _TIMER_H_

#include<vector>
#include<functional>
#include<chrono>
#include<unordered_map>
#include<assert.h>
#include "../log/log.h"
typedef std::function<void()> TimeOutCallBack;
typedef std::chrono::milliseconds MS;//毫秒
typedef std::chrono::high_resolution_clock Clock;
typedef Clock::time_point TimeStamp;

struct Node{
    size_t id_;//节点编号
    TimeOutCallBack callBack_;
    TimeStamp expire_;//过期时间
    //重载<
    bool operator<(const Node t){
        return expire_<t.expire_;
    }
};

class Timer{
public:
    Timer();
    ~Timer();
    void Add(int id,int timeoutMS,TimeOutCallBack callback);//加入计时器堆
    int getNextTick();//获得最快过期的请求时间
    void justTimer(int id,int timeoutMS);
private:

    std::vector<Node> heap_;    //定时器堆
    std::unordered_map<int,size_t> fromIdToLocation_;//id与location的映射
    

private:
    
    bool justDown(int id);//向下调整
    void justUp(int id);//向上调整
    void swapNode(size_t loc1,size_t loc2);
    void Del(size_t loc);//删除指定位置
    void Tick();//关闭超时连接
    void Pop(size_t loc);//出队
    void showNodeId();
    
};


#endif