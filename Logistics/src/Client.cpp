#include "Client.hpp"
#include <iostream>

bool Client::initWinsock() const {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

bool Client::connectToServer() {
    if (!initWinsock()) return false;
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(serverPort);
    addr.sin_addr.s_addr = inet_addr(serverIp.c_str());

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        sock = INVALID_SOCKET;
        return false;
    }
    return true;
}

void Client::disconnect() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}

bool Client::sendAll(const std::string& msg) {
    // TCP send 不保证一次发完全部字节, 此处循环直至完成或出错.
    size_t total = 0;
    while (total < msg.size()) {
        int sent = send(sock,
            msg.c_str() + total,
            static_cast<int>(msg.size() - total),
            0);
        if (sent == SOCKET_ERROR || sent == 0) return false;
        total += static_cast<size_t>(sent);
    }
    return true;
}

Client::Client(const std::string& ip, int port)
    : serverIp(ip), serverPort(port), sock(INVALID_SOCKET), connected(false) {
    connected = connectToServer();
}

Client::~Client() {
    disconnect();
    WSACleanup();
}

bool Client::isConnected() const {
    return connected && sock != INVALID_SOCKET;
}

std::string Client::sendRequest(const std::string& request) {
    if (!isConnected()) return "ERR|" + Protocol::serializeResult({ErrorCode::LoginFailed, "未连接到服务器"});

    std::string msg = request + "\n";
    if (!sendAll(msg)) {
        disconnect();
        connected = false;
        return "ERR|" + Protocol::serializeResult({ErrorCode::LoginFailed, "发送失败"});
    }

    // 接收响应直至完整一行 '\n'; 跨 recv 剩余字节由 recvBuffer 保留,
    // 不丢弃下一条响应可能已到达的字节.
    size_t pos;
    while ((pos = recvBuffer.find('\n')) == std::string::npos) {
        char buf[1024];
        int received = recv(sock, buf, sizeof(buf) - 1, 0);
        if (received <= 0) {
            disconnect();
            connected = false;
            return "ERR|" + Protocol::serializeResult({ErrorCode::LoginFailed, "接收失败"});
        }
        recvBuffer.append(buf, static_cast<size_t>(received));
    }
    std::string response = recvBuffer.substr(0, pos);
    recvBuffer.erase(0, pos + 1);
    return response;
}

std::string Client::call(const std::string& command, const std::vector<std::string>& args) {
    // 命令本身不需要 escape (命令名不含 '|' 或换行).
    // 业务参数必须 escape, 否则用户输入中的 '|' 会破坏请求字段边界.
    std::string request = command;
    for (const auto& arg : args) {
        request += "|" + Protocol::escapeField(arg);
    }
    return sendRequest(request);
}
