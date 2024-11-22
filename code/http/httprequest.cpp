#include "httprequest.h"

//默认可以请求的资源
const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index","/register","/login",
    "/welcome","/video","/picture"};

HttpRequest::HttpRequest(){
    Init();
}
void HttpRequest::Init(){
    method_=path_=version_=body_="";
    header_.clear();
    post_.clear();
    state_=REQUEST_LINE;

}
HttpRequest::~HttpRequest(){}
//解析请求行
bool HttpRequest:: ParseRequestLine_(const std::string &line){
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch matches;
    if(std::regex_match(line,matches,pattern)){
        //匹配成功
        method_=matches[1];
        path_=matches[2];
        version_=matches[3];
        state_=REQUEST_HEADER;//修改状态
        return true;
    }
    return false;;
}
//解析请求头
void HttpRequest::ParseRequestHeader_(const std::string &line){
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch matches;
    if(std::regex_match(line,matches,pattern)){
        header_[matches[1]]=matches[2];
    }else{
        state_=REQUEST_BODY;
    }
   
}
//解析请求体
void HttpRequest::ParseRequestBody_(const std::string &line){
    body_=line;
    std::cout<<"before ParsePost---body_:"<<std::endl;
    std::cout<<body_<<std::endl;
    ParsePost_();//解析键值对
    state_=FINISH;
}

void HttpRequest::ParsePost_(){
    if(method_=="POST" && header_["Content-Type"]=="application/x-www-form-urlencoded"){
        ParseFromUrlencoded_();//已经解码成键值对了
        if(path_=="/login.html"){
            //处理登录
            if(!DealLogin()){
                path_="/error.html";
            }
                

        }else if(path_=="/register.html"){
            //处理注册
            if(!DealRegister())path_="/error.html";
            
        }
    }
    
}
//处理登录
bool HttpRequest::DealLogin(){
    std::string name=post_["username"];
    std::string pwd=post_["password"];
    std::cout<<"name:  "<<name<<"    pwd:  "<<pwd<<std::endl;
    LOG_INFO("username = %s   password = %s   is requesting login",name.c_str(),pwd.c_str());
    if(name=="" || pwd=="") return false;
    MYSQL *sql=nullptr;
    SqlConnRAII tmpRAII(&sql,SqlConnPool::getIntance());//获得一条连接
    assert(sql);
    char order[256]={0};//记录查询的sql语句
    snprintf(order,sizeof(order),"select * from user where username = \'%s\' limit 1",name.c_str());//用户名不会重复
    if(mysql_query(sql,order))return false;//返回0表示查询成功
    MYSQL_RES *res=mysql_store_result(sql); //获得结果集
    if(!mysql_num_rows(res)) return false;//没有查到东西
    MYSQL_ROW row;
    bool flag=true;
    //比较密码是否正确
    while((row=mysql_fetch_row(res))){
        std::string password(row[1]);
        if(pwd!=password)flag=false;
    }
    mysql_free_result(res);
    if(flag)LOG_INFO("username = \"%s\"   password = \"%s\"  login successfully!",name,pwd);
    return flag;
}
//处理注册
bool HttpRequest::DealRegister(){
    std::string name=post_["username"];
    std::string pwd=post_["password"];
    if(name=="" || pwd=="") return false;
    MYSQL *sql=nullptr;
    SqlConnRAII tmpRAII(&sql,SqlConnPool::getIntance());//获得一条连接
    assert(sql);
    char order[256]={0};//记录查询的sql语句
    snprintf(order,sizeof(order),"select * from user where username = '%s'",name.c_str());
    if(mysql_query(sql,order)<0) return false;
    MYSQL_RES *res=mysql_store_result(sql); //获得结果集
    if(mysql_num_rows(res)) return false;//查到结果说明用户名已经被注册了
    //插入新用户
    snprintf(order,sizeof(order),"insert into user(username,password) values('%s','%s')",name.c_str(),pwd.c_str());
    LOG_DEBUG("register user's sql is \"%s\"",order);
    if(mysql_query(sql,order)<0) return false;
    LOG_INFO("username = \"%s\"   password = \"%s\"   register successfully!",name.c_str(),pwd.c_str());
    return true;
}
//对post请求体进行url解码
void HttpRequest::ParseFromUrlencoded_(){
    std::string line=body_;
    int l=0,r=0;
    std::string key,value;
    std::ostringstream decode;
    int d;
    for(;r<line.size();r++){
        char ch=line[r];
        switch (ch)
        {
        case '=':
            key=decode.str();
            std::cout<<"key: "<<key<<std::endl;
            decode.str("");        // 重置流内容
            decode.clear();        // 重置状态标志
            l=r+1;
            break;
        case '+':
            decode<<' ';
            break;
        case '&':
            value=decode.str();
            std::cout<<"value: "<<value<<std::endl;
            post_[key]=value;
            decode.str("");        // 重置流内容
            decode.clear();        // 重置状态标志
            l=r+1;
            break;
        case '%':
            //将十六进制转化为十进制
            d=std::stoi(line.substr(r+1,2),nullptr,16);
            decode<<static_cast<char>(d);
            r=r+2;
            break;
        default :
            decode<<ch;
            break;
        }
    }
    if(!post_.count(key)){
        post_[key]=decode.str();
        decode.str("");        // 重置流内容
        decode.clear();        // 重置状态标志
    }
}
void HttpRequest::ParsePath_(){
    if(path_=="/"){
        path_="/index.html";//定位到主页
    }else{
        for(auto t : DEFAULT_HTML){
            if(path_==t){
                path_+=".html";
                break;
            }
        }
    }
}
//解析请求
bool HttpRequest::parse(Buffer &buffer_){
    if(buffer_.GetReadableBytes()<=0) return false;
    //开始解析
    const char CRLF[]="\r\n";
    while(buffer_.GetReadableBytes() && state_!=FINISH){
        char *lineEnd=std::search(buffer_.GetBeginReadPtr(),buffer_.GetBeginWritePtr(),CRLF,CRLF+2);
        std::string line(buffer_.GetBeginReadPtr(),lineEnd-buffer_.GetBeginReadPtr());
        //std::cout<<"read_line:"<<line<<std::endl; 
        switch (state_)
        {
        case REQUEST_LINE:
            if(!ParseRequestLine_(line)) return false;
            //补全path_的资源后缀
            ParsePath_();
            break;
        case REQUEST_HEADER:
            ParseRequestHeader_(line);
            //判断是否为GET请求
            if(buffer_.GetReadableBytes()<=2){
                //std::cout<<"this is GET Method"<<std::endl;
                state_=FINISH;
            }
            break;
        case REQUEST_BODY:
            ParseRequestBody_(line);
            break;
        default:
            break;
        }
        if(lineEnd==buffer_.GetBeginWritePtr())break;
        buffer_.RetrievePartly(lineEnd+2);
    }
    //展示解析结果
    //ShowParseResult();
    return true;
}

//返回连接状态
bool HttpRequest::IsKeepAlive(){
    if(header_.count("Connection")){
        return header_.find("Connection")->second=="keep-alive" && version_=="1.1";
    }
    return false;
}
std::string HttpRequest::GetPath_(){
    return path_;
}

//展示请求报文解析结果
void HttpRequest::ShowParseResult(){
    //打印请求行
    std::cout<<"Method:"<<method_<<" "<<"Path:"<<path_<<" "<<"Version:"<<version_<<std::endl;
    std::cout<<"Header:"<<std::endl;
    //打印请求头
    for(auto header : header_){
        std::cout<<header.first<<":"<<header.second<<std::endl;
    }
    //打印请求体
    std::cout<<"Post:"<<std::endl;
    for(auto post : post_){
        std::cout<<post.first<<":"<<post.second<<std::endl;
    }
}