#pragma once

#include "System.hpp"
#include <winsock2.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

/*
Server 类封装服务器端 socket 通信与请求分发。
负责监听端口、接受客户端连接、解析命令并调用 System 完成业务处理，
最后将结果序列化后返回客户端。每个连接使用独立线程处理，支持并发访问。
由于 System 内部容器及索引未加锁, processRequest 内部对 System 的访问
统一通过 requestMutex 串行化, 防止多线程并发导致数据竞争或迭代失效.
*/

class Server {
private:
    System& sys;
    int port;
    SOCKET listenSock;
    std::atomic<bool> running;
    std::mutex requestMutex;        // 串行化对 sys 的业务调用

    bool initWinsock() const;
    bool startListen();
    void acceptLoop();
    void handleClient(SOCKET clientSock);

    // 网络收发: 处理 TCP partial send / recv 边界
    bool sendAll(SOCKET clientSock, const std::string& msg) const;

    // 请求处理: 解析命令，调用 System，返回响应字符串
    std::string processRequest(const std::string& request);

public:
    Server(System& _sys, int _port);
    ~Server();
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void start();
    void stop();
};
