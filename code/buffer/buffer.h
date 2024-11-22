#ifndef _BUFFER_H_
#define _BUFFER_H_
#include<vector>
#include<atomic>
#include<sys/uio.h>
#include<unistd.h>
#include<iostream>
#include<assert.h>
#include<string.h>
class Buffer{
public:
    Buffer(int BufferSize=1024);
    ~Buffer();
    ssize_t ReadFd(int fd,int* readErr);//读取数据
    ssize_t WriteFd(int fd);//发送数据
    void Retrieve(size_t len);//回收指定长度
    void RetrieveAll();//回收所有缓冲区
    void RetrievePartly(const char *end);//回收部分缓冲区

    size_t GetReadPos();//获得读指针位置
    char* GetBeginReadPtr();
   // const char* GetBeginReadPtr();
    size_t GetReadableBytes();

    size_t GetWritePos();//获得写指针位置
    char* GetBeginWritePtr();
    size_t GetWritableBytes();//获得可写的字节大小
    void HasWriten(size_t len);
    char* GetBeginPtr();//获得缓冲区的起始指针

    //写入缓冲区
    void Append(std::string str);
    void Append(const char* str,size_t len);

    void ShowData();
    std::string RetrieveAllToStr();
    
private:
    std::vector<char> buffer_;//缓冲区
    std::atomic<size_t> readPos_;//读指针
    std::atomic<size_t> writePos_;//写指针
    
    
    void EnsureWritable(ssize_t);//确保区域可写
    void MakeSpace(size_t);//扩容或者整理内碎片
    
    
};




#endif