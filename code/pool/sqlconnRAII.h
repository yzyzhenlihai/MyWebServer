#ifndef _SQLCONNRAII_H_
#define _SQLCONNRAII_H_
#include<mysql/mysql.h>
#include "sqlconnpool.h"
//利用RAII机制,获取资源以及释放资源
class SqlConnRAII{
public:
    SqlConnRAII(MYSQL **sql,SqlConnPool *connpool){
        SqlConnPool_=connpool;
        *sql=connpool->GetConn();
        sql_=*sql;//保存这条连接，后续要归还的
    }
    ~SqlConnRAII(){
        //归还连接
        if(sql_){
            SqlConnPool_->BackConn(sql_);
        }
        
    }
private:
    MYSQL *sql_;
    SqlConnPool *SqlConnPool_;
};


#endif