#pragma once

#include "Protocol.hpp"
#include <string>
#include <winsock2.h>

/*
Client 类封装客户端 socket 通信。
负责与服务器建立 TCP 连接、发送请求、接收响应。
所有网络操作均封装为成员函数，满足"不允许出现非类成员函数"的要求。
sendRequest 内部使用 sendAll 处理 TCP partial send, recv 循环按 '\n'
切分响应行, 跨多次 recv 的剩余字节由 buffer 暂存, 防止丢失.
*/

class Client {
private:
    std::string serverIp;
    int serverPort;
    SOCKET sock;
    bool connected;
    std::string recvBuffer;     // 跨 recv 的接收缓冲, 用于按 '\n' 切分响应

    bool initWinsock() const;
    bool connectToServer();
    void disconnect();
    bool sendAll(const std::string& msg);

public:
    Client(const std::string& ip, int port);
    ~Client();
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // 连接状态
    bool isConnected() const;

    // 发送请求并接收响应
    std::string sendRequest(const std::string& request);

    // 便捷方法: 发送命令+参数，返回响应
    std::string call(const std::string& command, const std::vector<std::string>& args);
};
