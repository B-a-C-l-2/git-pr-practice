#include "Server.hpp"
#include "Protocol.hpp"
#include "User.hpp"
#include "Admin.hpp"
#include "Courier.hpp"
#include "Object.hpp"
#include "Book.hpp"
#include "Fragile.hpp"
#include <iostream>
#include <sstream>

bool Server::initWinsock() const {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

bool Server::startListen() {
    listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(listenSock);
        listenSock = INVALID_SOCKET;
        return false;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listenSock);
        listenSock = INVALID_SOCKET;
        return false;
    }
    return true;
}

Server::Server(System& _sys, int _port)
    : sys(_sys), port(_port), listenSock(INVALID_SOCKET), running(false) {}

Server::~Server() {
    stop();
    WSACleanup();
}

void Server::start() {
    if (!initWinsock()) {
        std::cerr << "Winsock 初始化失败！\n";
        return;
    }
    if (!startListen()) {
        std::cerr << "监听端口 " << port << " 失败！\n";
        return;
    }
    running = true;
    std::cout << "服务器已启动，监听端口 " << port << " ...\n";
    acceptLoop();
}

void Server::stop() {
    running = false;
    if (listenSock != INVALID_SOCKET) {
        closesocket(listenSock);
        listenSock = INVALID_SOCKET;
    }
}

void Server::acceptLoop() {
    while (running) {
        sockaddr_in clientAddr{};
        int addrLen = sizeof(clientAddr);
        SOCKET clientSock = accept(listenSock, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientSock == INVALID_SOCKET) {
            if (running) std::cerr << "accept 失败\n";
            continue;
        }
        std::thread t(&Server::handleClient, this, clientSock);
        t.detach();
    }
}

bool Server::sendAll(SOCKET clientSock, const std::string& msg) const {
    // TCP send 可能只发出一部分字节, 此处循环直至全部发送或出错.
    size_t total = 0;
    while (total < msg.size()) {
        int sent = send(clientSock,
            msg.c_str() + total,
            static_cast<int>(msg.size() - total),
            0);
        if (sent == SOCKET_ERROR || sent == 0) return false;
        total += static_cast<size_t>(sent);
    }
    return true;
}

void Server::handleClient(SOCKET clientSock) {
    char buf[1024];
    // 跨 recv 的接收缓冲, 用于按 '\n' 切分请求行, 不丢弃 '\n' 后剩余字节.
    std::string buffer;
    while (running) {
        size_t pos;
        while ((pos = buffer.find('\n')) == std::string::npos) {
            int received = recv(clientSock, buf, sizeof(buf) - 1, 0);
            if (received <= 0) {
                closesocket(clientSock);
                return;
            }
            buf[received] = '\0';
            buffer.append(buf, static_cast<size_t>(received));
        }
        std::string request = buffer.substr(0, pos);
        buffer.erase(0, pos + 1);

        std::string response = processRequest(request);
        response += "\n";
        if (!sendAll(clientSock, response)) {
            closesocket(clientSock);
            return;
        }
    }
    closesocket(clientSock);
}

static std::vector<std::string> parseArgs(const std::string& request) {
    // split 按 '|' 切分原始字节, 命令本身保持原样,
    // 业务参数在客户端已 escape, 此处需 unescape 后再交给业务层.
    auto raw = Protocol::split(request, '|');
    for (size_t i = 1; i < raw.size(); ++i) {
        raw[i] = Protocol::unescapeField(raw[i]);
    }
    return raw;
}

std::string Server::processRequest(const std::string& request) {
    // 串行化所有业务请求, 避免多个 client 线程同时读写 System 内部容器.
    // 锁只覆盖业务处理, 不覆盖 recv/send, 以免一个慢客户端阻塞所有人.
    std::lock_guard<std::mutex> lock(requestMutex);

    auto args = parseArgs(request);
    if (args.empty()) return Protocol::errResponse({ErrorCode::LoginFailed, "空请求"});
    const std::string& cmd = args[0];

    try {
        if (cmd == Protocol::CMD_LOGIN) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            Result r = sys.authenticate(args[1], args[2]);
            if (!r.ok()) return Protocol::errResponse(r);
            // 登录成功一次性下发身份, 客户端 Session 据此缓存权限, 不再用
            // "尝试调用管理员/快递员接口是否 OK" 这种不可靠方式判断角色.
            auto idOpt = sys.queryIdentity(args[1]);
            int perm = static_cast<int>(Permission::User);
            int banned = 0;
            if (idOpt.has_value()) {
                perm = static_cast<int>(idOpt->first);
                banned = idOpt->second ? 1 : 0;
            }
            return Protocol::okResponse(std::to_string(perm) + "|" + std::to_string(banned));
        }
        if (cmd == Protocol::CMD_REGISTER) {
            if (args.size() < 6) return Protocol::errResponse({ErrorCode::AddUserFailed, "参数不足"});
            Result r = sys.registerUser(args[1], args[2], args[3], args[4], args[5]);
            if (r.ok()) return Protocol::okResponse();
            return Protocol::errResponse(r);
        }
        if (cmd == Protocol::CMD_CHANGE_PASSWORD) {
            if (args.size() < 4) return Protocol::errResponse({ErrorCode::ChangePasswordFailed, "参数不足"});
            Result r = sys.changePassword(args[1], args[2], args[3]);
            if (r.ok()) return Protocol::okResponse();
            return Protocol::errResponse(r);
        }
        if (cmd == Protocol::CMD_QUERY_BALANCE) {
            if (args.size() < 2) return Protocol::errResponse({ErrorCode::RechargeFailed, "参数不足"});
            auto bal = sys.queryBalance(args[1]);
            if (bal.has_value()) return Protocol::okResponse(std::to_string(*bal));
            return Protocol::errResponse({ErrorCode::RechargeFailed, "查询失败"});
        }
        if (cmd == Protocol::CMD_RECHARGE) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::RechargeFailed, "参数不足"});
            Result r = sys.recharge(args[1], std::stoll(args[2]));
            if (r.ok()) return Protocol::okResponse();
            return Protocol::errResponse(r);
        }
        if (cmd == Protocol::CMD_SEND_EXPRESS) {
            if (args.size() < 6) return Protocol::errResponse({ErrorCode::AddExpressFailed, "参数不足"});
            std::string sender = args[1];
            std::string receiver = args[2];
            int type = std::stoi(args[3]);
            std::string desc = args[4];
            long long param = std::stoll(args[5]);
            std::unique_ptr<BaseObject> obj;
            if (type == 0) obj = std::make_unique<Object>(desc, param);
            else if (type == 1) obj = std::make_unique<Book>(desc, param);
            else obj = std::make_unique<Fragile>(desc, param);
            SendResult sr = sys.sendExpress(sender, receiver, std::move(obj));
            if (sr.result.ok()) return Protocol::okResponse(Protocol::serializeSendResult(sr));
            return Protocol::errResponse(sr.result);
        }
        if (cmd == Protocol::CMD_SIGN_EXPRESS) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::SignExpressFailed, "参数不足"});
            Result r = sys.signExpress(args[1], std::stoull(args[2]));
            if (r.ok()) return Protocol::okResponse();
            return Protocol::errResponse(r);
        }
        if (cmd == Protocol::CMD_SIGN_MANY) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::SignExpressFailed, "参数不足"});
            std::vector<unsigned long long> nos;
            for (size_t i = 2; i < args.size(); ++i) nos.push_back(std::stoull(args[i]));
            BatchResult br = sys.signMany(args[1], nos);
            return Protocol::okResponse(Protocol::serializeBatchResult(br));
        }
        if (cmd == Protocol::CMD_UPDATE_DESCRIPTION) {
            if (args.size() < 4) return Protocol::errResponse({ErrorCode::UpdateDescriptionFailed, "参数不足"});
            Result r = sys.updateDescription(args[1], std::stoull(args[2]), args[3]);
            if (r.ok()) return Protocol::okResponse();
            return Protocol::errResponse(r);
        }
        if (cmd == Protocol::CMD_QUERY_SENT) {
            if (args.size() < 2) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            auto list = sys.querySent(args[1]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_RECEIVE) {
            if (args.size() < 2) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            auto list = sys.queryReceive(args[1]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_PENDING) {
            if (args.size() < 2) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            auto list = sys.queryPending(args[1]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_BY_SENDER) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            auto list = sys.queryBySender(args[1], args[2]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_BY_RECEIVER) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            auto list = sys.queryByReceiver(args[1], args[2]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_BY_TIME) {
            if (args.size() < 4) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            auto list = sys.queryByTime(args[1], static_cast<time_t>(std::stoll(args[2])), static_cast<time_t>(std::stoll(args[3])));
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_BY_TRACKING_NO) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            const Express* e = sys.queryByTrackingNo(args[1], std::stoull(args[2]));
            if (e) return Protocol::okResponse(Protocol::serializeExpress(*e));
            return Protocol::errResponse({ErrorCode::LoginFailed, "快递不存在"});
        }
        if (cmd == Protocol::CMD_PICKUP) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            Result r = sys.pickup(args[1], std::stoull(args[2]));
            if (r.ok()) return Protocol::okResponse();
            return Protocol::errResponse(r);
        }
        if (cmd == Protocol::CMD_PICKUP_MANY) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            std::vector<unsigned long long> nos;
            for (size_t i = 2; i < args.size(); ++i) nos.push_back(std::stoull(args[i]));
            BatchResult br = sys.pickupMany(args[1], nos);
            return Protocol::okResponse(Protocol::serializeBatchResult(br));
        }
        if (cmd == Protocol::CMD_QUERY_PENDING_PICKUP) {
            if (args.size() < 2) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            auto list = sys.queryPendingPickup(args[1]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_PICKED_UP) {
            if (args.size() < 2) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            auto list = sys.queryPickedUp(args[1]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_PICKED_UP_BY_SENDER) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            auto list = sys.queryPickedUpBySender(args[1], args[2]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_PICKED_UP_BY_RECEIVER) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            auto list = sys.queryPickedUpByReceiver(args[1], args[2]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_PICKED_UP_BY_TIME) {
            if (args.size() < 4) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            auto list = sys.queryPickedUpByTime(args[1], static_cast<time_t>(std::stoll(args[2])), static_cast<time_t>(std::stoll(args[3])));
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_PICKED_UP_BY_STATUS) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            auto list = sys.queryPickedUpByStatus(args[1], static_cast<Status>(std::stoi(args[2])));
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_PICKED_UP_BY_TRACKING_NO) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            const Express* e = sys.queryPickedUpByTrackingNo(args[1], std::stoull(args[2]));
            if (e) return Protocol::okResponse(Protocol::serializeExpress(*e));
            return Protocol::errResponse({ErrorCode::PickupFailed, "快递不存在"});
        }
        if (cmd == Protocol::CMD_QUERY_DELIVERED) {
            if (args.size() < 2) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            auto list = sys.queryDelivered(args[1]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_DELIVERED_BY_SENDER) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            auto list = sys.queryDeliveredBySender(args[1], args[2]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_DELIVERED_BY_RECEIVER) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            auto list = sys.queryDeliveredByReceiver(args[1], args[2]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_DELIVERED_BY_TIME) {
            if (args.size() < 4) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            auto list = sys.queryDeliveredByTime(args[1], static_cast<time_t>(std::stoll(args[2])), static_cast<time_t>(std::stoll(args[3])));
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_DELIVERED_BY_STATUS) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            auto list = sys.queryDeliveredByStatus(args[1], static_cast<Status>(std::stoi(args[2])));
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_DELIVERED_BY_TRACKING_NO) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::PickupFailed, "参数不足"});
            const Express* e = sys.queryDeliveredByTrackingNo(args[1], std::stoull(args[2]));
            if (e) return Protocol::okResponse(Protocol::serializeExpress(*e));
            return Protocol::errResponse({ErrorCode::PickupFailed, "快递不存在"});
        }
        if (cmd == Protocol::CMD_ADD_COURIER) {
            if (args.size() < 6) return Protocol::errResponse({ErrorCode::AddCourierFailed, "参数不足"});
            Result r = sys.addCourier(args[1], args[2], args[3], args[4], args[5]);
            if (r.ok()) return Protocol::okResponse();
            return Protocol::errResponse(r);
        }
        if (cmd == Protocol::CMD_BAN_COURIER) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::BanFailed, "参数不足"});
            Result r = sys.banCourier(args[1], args[2]);
            if (r.ok()) return Protocol::okResponse();
            return Protocol::errResponse(r);
        }
        if (cmd == Protocol::CMD_ASSIGN_COURIER) {
            if (args.size() < 4) return Protocol::errResponse({ErrorCode::AssignCourierFailed, "参数不足"});
            Result r = sys.assignCourier(args[1], args[2], std::stoull(args[3]));
            if (r.ok()) return Protocol::okResponse();
            return Protocol::errResponse(r);
        }
        if (cmd == Protocol::CMD_BAN_USER) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::BanFailed, "参数不足"});
            Result r = sys.banUser(args[1], args[2]);
            if (r.ok()) return Protocol::okResponse();
            return Protocol::errResponse(r);
        }
        if (cmd == Protocol::CMD_UNBAN_USER) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::UnbanFailed, "参数不足"});
            Result r = sys.unbanUser(args[1], args[2]);
            if (r.ok()) return Protocol::okResponse();
            return Protocol::errResponse(r);
        }
        if (cmd == Protocol::CMD_QUERY_ALL_USERS) {
            if (args.size() < 2) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            auto list = sys.queryAllUsers(args[1]);
            return Protocol::okResponse(Protocol::serializeUserList(list));
        }
        if (cmd == Protocol::CMD_QUERY_ALL_EXPRESS) {
            if (args.size() < 2) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            auto list = sys.queryAllExpress(args[1]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_ALL_COURIER) {
            if (args.size() < 2) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            auto list = sys.queryAllCourier(args[1]);
            return Protocol::okResponse(Protocol::serializeUserList(list));
        }
        if (cmd == Protocol::CMD_QUERY_ALL_BY_USER) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            auto list = sys.queryAllByUser(args[1], args[2]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_ALL_BY_TIME) {
            if (args.size() < 4) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            auto list = sys.queryAllByTime(args[1], static_cast<time_t>(std::stoll(args[2])), static_cast<time_t>(std::stoll(args[3])));
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
        if (cmd == Protocol::CMD_QUERY_ALL_BY_TRACKING_NO) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            const Express* e = sys.queryAllByTrackingNo(args[1], std::stoull(args[2]));
            if (e) return Protocol::okResponse(Protocol::serializeExpress(*e));
            return Protocol::errResponse({ErrorCode::LoginFailed, "快递不存在"});
        }
        if (cmd == Protocol::CMD_QUERY_ALL_BY_COURIER) {
            if (args.size() < 3) return Protocol::errResponse({ErrorCode::LoginFailed, "参数不足"});
            auto list = sys.queryAllByCourier(args[1], args[2]);
            return Protocol::okResponse(Protocol::serializeExpressList(list));
        }
    } catch (const std::exception& e) {
        return Protocol::errResponse({ErrorCode::LoginFailed, std::string("服务器内部错误: ") + e.what()});
    }

    return Protocol::errResponse({ErrorCode::LoginFailed, "未知命令"});
}
