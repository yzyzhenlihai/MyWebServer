#include "timer.h"

Timer::Timer(){}
Timer::~Timer(){}


int Timer::getNextTick(){
    Tick();//关闭超时连接
    int res=0;
    if(heap_.size()){
        Node node = heap_.front();
        res=std::chrono::duration_cast<MS>(node.expire_ - Clock::now()).count();
        if(res<0) res=0;
    }
    return res;
}
//关闭超时连接
void Timer::Tick(){
    //LOG_DEBUG("Tick is executing---there are %d clients totally",heap_.size());
    while(heap_.size()){
        Node node = heap_.front();
        //std::cout<<node.id_<<" ";
        if(std::chrono::duration_cast<MS>(node.expire_ - Clock::now()).count()<=0){
            //已经超时
            LOG_DEBUG("Client[%d] is closing...",node.id_);
            node.callBack_();//执行回调函数
            Pop(0);
        }else break;
        
    }
    //std::cout<<std::endl;
}
//删除指定节点
void Timer::Del(size_t loc){
    assert(loc<heap_.size());
    swapNode(loc,heap_.size()-1);
    fromIdToLocation_.erase(heap_.back().id_);
    heap_.pop_back();
    if(!justDown(loc)){
        justUp(loc);
    }
    //std::cout<<"after del:";
    //showNodeId();
}
//弹出
void Timer::Pop(size_t loc){
    assert(loc<heap_.size());
    Del(loc);
}
//调整定时器事件
void Timer::justTimer(int id,int timeoutMS){
    if(fromIdToLocation_.count(id)){
        Node node = heap_[fromIdToLocation_[id]];
        node.expire_ = Clock::now() + MS(timeoutMS);
        if(!justDown(id)){
            justUp(id);
        }
    }
}
//添加定时器节点
void Timer::Add(int id,int timeoutMS,TimeOutCallBack callback){
    if(fromIdToLocation_.count(id)==0){
        //新定时器
        Node newNode={0};
        newNode.id_=id;
        newNode.expire_=Clock::now()+MS(timeoutMS);
        newNode.callBack_=callback;
        fromIdToLocation_[id]=heap_.size();
        //std::cout<<"new timer : "<<id<<std::endl;
        heap_.push_back(newNode);//插入堆
        //showNodeId();
        justUp(id);//向上调整
    }else{
        //更新定时器
        time_t loc=fromIdToLocation_[id];
        heap_[loc].expire_=Clock::now()+MS(timeoutMS);
        heap_[loc].callBack_=callback;
        //向下调整/向上调整
        if(!justDown(id)){
            justUp(id);
        }
    }
}
//向上调整
void Timer::justUp(int id){
    size_t i = fromIdToLocation_[id];
    size_t j = (i-1)/2;
    while(i){
        if(heap_[j] < heap_[i]) break;
        else{
            swapNode(i,j);
            i=j;
            j=(i-1)/2;
        }
    }
}
//交换节点
void Timer::swapNode(size_t loc1,size_t loc2){
    assert(loc1>=0);
    assert(loc2>=0);
    std::swap(heap_[loc1],heap_[loc2]);
    fromIdToLocation_[heap_[loc1].id_]=loc1;
    fromIdToLocation_[heap_[loc2].id_]=loc2;
}

//小根堆向下调整
bool Timer::justDown(int id){
    assert(id>=0);
    size_t i=fromIdToLocation_[id];
    size_t index=i;
    size_t j=2*i+1;
    size_t n = heap_.size();
    while(j<n){
        if(j+1<n && heap_[j+1]<heap_[j])j++;
        if(heap_[i]<heap_[j]) break;
        else{
            swapNode(i,j);
            i=j;
            j=2*i+1;
        }
    }
    return index < i; //返回False表示没有向下调整
}

void Timer::showNodeId(){
    for(auto node : heap_){
        std::cout<<node.id_<<" ";
    }
    std::cout<<std::endl;
}