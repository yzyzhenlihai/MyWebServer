#include "httpresponse.h"

const std::unordered_map<int,std::string> HttpResponse::CODE_PATH={
    {404 ,"/404.html"},
    {403 ,"/403.html"},
    {400 ,"/400.html"}
};
const std::unordered_map<int,std::string> HttpResponse::CODE_STATE{
    {404,"Not Found"},
    {403,"Forbidden"},
    {400,"Bad Request"},
    {200,"OK"}
};

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};
HttpResponse::HttpResponse(){}

HttpResponse::~HttpResponse(){}

char* HttpResponse::GetFile(){
    if(mmFile_){
        return mmFile_;
    }
    return nullptr;
}
size_t HttpResponse::GetFileLen(){
    return statBuf_.st_size;
}
//初始化响应报文
void HttpResponse::Init(const std::string srcDir,std::string path,bool isKeepAlive,int code){
    srcDir_=srcDir;
    path_=path;
    code_=code;
    isKeepAlive_=isKeepAlive;
    statBuf_={0};
    mmFile_=nullptr;
}
//构造响应报文
void HttpResponse::MakeResponse(Buffer &buffer_){
    if(stat((srcDir_+path_).data(),&statBuf_) < 0){
        code_=404;//Not Found
    }else if(!(statBuf_.st_mode & S_IROTH)){
        code_=403;//Forbbiden
    }
    //重定向错误网页
    ErrorHtml_();
    //添加状态行
    AddStateLine_(buffer_);
    //添加响应头
    AddHeader_(buffer_);
    //添加响应正文
    AddContent(buffer_);
}
//重定向错误页面
void HttpResponse::ErrorHtml_(){
    if(CODE_PATH.count(code_)){
        path_=CODE_PATH.find(code_)->second;
    }
}
//添加状态行
void HttpResponse::AddStateLine_(Buffer &buffer_){
    //HTTP版本、 //状态码、状态信息
    std::string codeState="";
    if(CODE_STATE.count(code_)){
        codeState=CODE_STATE.find(code_)->second;
    }else{
        code_ = 400;
        codeState = CODE_STATE.find(400)->second;
    }
    buffer_.Append("HTTP/1.1 "+ std::to_string(code_) + " " +codeState+"\r\n");
   
} 
//添加响应头
void HttpResponse::AddHeader_(Buffer &buffer_){
    buffer_.Append("Connection:");
    if(isKeepAlive_){
        buffer_.Append("keep-alive\r\n");
        buffer_.Append("keep-alive: max=6, timeout=120\r\n");
    }else{
        buffer_.Append("close\r\n");
    }
    buffer_.Append("Content-type:"+GetFileType_()+"\r\n");
}
//获得响应文件的类型
std::string HttpResponse::GetFileType_(){
    //获得文件后缀
    std::string::size_type loc=path_.find_last_of('.');
    if(loc==std::string::npos){
        return "text/plain";//没找到后缀默认为纯文本
    }else{
        std::string suffix=path_.substr(loc);
        if(SUFFIX_TYPE.count(suffix)){
            return SUFFIX_TYPE.find(suffix)->second;
        }
    }
    return "text/plain";
}

void HttpResponse::AddContent(Buffer &buffer_){
    //打开文件
    int srcFd=open((srcDir_+path_).data(),O_RDONLY);
    assert(srcFd);
    int* mRet=(int*)mmap(nullptr,statBuf_.st_size,PROT_READ,MAP_PRIVATE,srcFd,0);
    if(*mRet==-1){
        //建立内存映射区失败
        return;
    }
    mmFile_=reinterpret_cast<char*>(mRet);
    close(srcFd);//关闭文件描述符
    buffer_.Append("Content-length: " + std::to_string(statBuf_.st_size) + "\r\n\r\n");
}

//释放内存映射区
void HttpResponse::UnmapFile_(){
    if(mmFile_){
        munmap(mmFile_,statBuf_.st_size);
        mmFile_=nullptr;
    }
}

//展示响应结果
void HttpResponse::ShowResponseResult(Buffer &buffer_){
    std::cout<<"ShowResponseResult:"<<std::endl;
    buffer_.ShowData();
    //std::cout<<"mmap:"<<std::endl;
    //if(mmFile_)std::cout<<mmFile_;
}
