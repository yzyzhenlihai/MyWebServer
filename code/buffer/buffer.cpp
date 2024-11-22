#include"buffer.h"

Buffer::Buffer(int BufferSize):buffer_(BufferSize),readPos_(0),writePos_(0){}
Buffer::~Buffer(){}
void Buffer::Retrieve(size_t len){
    assert(len<=GetReadableBytes());
    readPos_+=len;
}
void Buffer::RetrieveAll(){
    writePos_=0;
    readPos_=0;
    bzero(&buffer_[0], buffer_.size());
    //buffer_.clear();
}
void Buffer::RetrievePartly(const char *end){
    assert(GetBeginReadPtr()<=end);
    Retrieve(end-GetBeginReadPtr());

}
//往缓冲区添加数据
void Buffer::Append(std::string str){
    Append(str.data(),str.size());
}
void Buffer::Append(const char *str,size_t len){

    EnsureWritable(len);
    std::copy(str,str + len,GetBeginWritePtr());
    writePos_.fetch_add(len);
}
ssize_t Buffer::ReadFd(int fd,int *readErr){
    //一次项读取所有数据,利用readv系统调用
    char buf[65535];//临时缓冲区
    iovec iov[2];
    const size_t writable=GetWritableBytes();//获得可写的区域大小
    iov[0].iov_base=GetBeginPtr()+writePos_;
    iov[0].iov_len=writable;
    iov[1].iov_base=buf;    
    iov[1].iov_len=sizeof(buf);
    //开始读取
    ssize_t len=readv(fd,iov,2);
    if(len<0){
        //错误处理
        *readErr=errno;
    }else if(static_cast<size_t>(len)<=writable){
        //std::cout<<"before fetch_add:"<<writePos_<<std::endl;
        writePos_.fetch_add(len);
        //std::cout<<"after fetch_add:"<<writePos_<<std::endl;
    }else{
        //超出缓冲区
        writePos_=buffer_.size();
        Append(buf,len-writable);
        // EnsureWritable(static_cast<size_t>(len)-writable);
        // //拷贝数据
        // std::copy(buf,buf+len-writable,GetBeginWritePtr());
        // //修改指针
        // std::cout<<"before fetch_add:"<<writePos_<<std::endl;
        // writePos_.fetch_add(len-static_cast<size_t>(len)-writable);
        // std::cout<<"after fetch_add:"<<writePos_<<std::endl;
        //打印读取的数据
        //ShowData();
        
    }
    return len;
}
ssize_t Buffer::WriteFd(int fd){
    //写数据
    size_t readsize=GetReadableBytes();
    ssize_t size=write(fd,GetBeginReadPtr(),readsize);
    if(size==-1){
        //错误处理
    }else if(size>=0){
        //std::cout<<"buffer send data successful"<<std::endl;
        readPos_.fetch_add(readsize);
    }
    return readsize;
}
void Buffer::EnsureWritable(ssize_t len){
    if(static_cast<size_t>(len)>GetWritableBytes()){
        MakeSpace(len);
    }
    assert(GetWritableBytes() >= static_cast<size_t>(len));
}
//扩容
void Buffer::MakeSpace(size_t len){
    //计算剩余所有空间
    size_t resSpace=readPos_+GetWritableBytes();
    if(resSpace>=static_cast<size_t>(len)){
        //剩余空间可用,整理内碎片
        char* beginptr=GetBeginPtr();
        size_t readable=GetReadableBytes();
        std::copy(beginptr+readPos_,beginptr+writePos_,beginptr);
        //std::cout<<"before MakeSpace: writePos:"<<writePos_<<"readPos_:"<<readPos_<<std::endl;
        readPos_=0;
        writePos_=readable;
        //std::cout<<"after MakeSpace: writePos:"<<writePos_<<"readPos_:"<<readPos_<<std::endl;
        assert(readable == GetReadableBytes());
    }else{
        //扩容
        buffer_.resize(writePos_+len+1);
    }
}
size_t Buffer::GetReadableBytes(){
    return writePos_-readPos_;
}
char* Buffer::GetBeginReadPtr(){
    return GetBeginPtr()+readPos_;
}
size_t Buffer::GetReadPos(){
    return readPos_;
}
size_t Buffer::GetWritePos(){
    return writePos_;
}
size_t Buffer::GetWritableBytes(){
    return buffer_.size()-writePos_;
}
char* Buffer::GetBeginWritePtr(){
    return GetBeginPtr()+writePos_;
}
char* Buffer::GetBeginPtr(){
    return &(*buffer_.begin());
}

//展示当前读取的数据
void Buffer::ShowData(){
    std::cout<<readPos_<<" "<<writePos_<<std::endl;
    for(size_t i = readPos_;i<writePos_;i++){
        std::cout<<buffer_[i];
    }
}

void Buffer::HasWriten(size_t len){
    writePos_.fetch_add(len);
}

std::string Buffer::RetrieveAllToStr(){
    std::string str=std::string(GetBeginReadPtr(),GetReadableBytes());
    RetrieveAll();
    return str;
}