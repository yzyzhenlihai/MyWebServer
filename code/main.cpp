#include<iostream>
#include"./server/webserver.h"
int main(){

    WebServer server(1666,1024,60000,10,3306,"yuzy","123456","myserver",10,true);
    server.Start();
}