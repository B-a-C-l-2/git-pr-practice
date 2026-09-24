#include "Protocol.hpp"
#include "Object.hpp"
#include "Book.hpp"
#include "Fragile.hpp"
#include "User.hpp"
#include "Admin.hpp"
#include "Courier.hpp"
#include <stdexcept>

/*
Protocol 友元不可替代性说明：
Protocol 负责在 socket 通信中对业务对象进行序列化与反序列化。
反序列化时需要调用各类的恢复构造函数（如 Express 的私有恢复构造、
BaseUser 及其子类的 protected 恢复构造、HashedPassword 的私有恢复构造），
这些构造函数不对外暴露是为了防止业务代码随意构造处于非法状态的对象。
若不为 Protocol 开放友元，则只能将恢复构造函数或大量 setter 设为 public，
这会严重破坏封装并增加数据不一致的风险。因此 Protocol 作为友元是必要的。
*/

const char* const Protocol::CMD_LOGIN = "LOGIN";
const char* const Protocol::CMD_REGISTER = "REGISTER";
const char* const Protocol::CMD_CHANGE_PASSWORD = "CHANGE_PASSWORD";
const char* const Protocol::CMD_QUERY_BALANCE = "QUERY_BALANCE";
const char* const Protocol::CMD_RECHARGE = "RECHARGE";
const char* const Protocol::CMD_SEND_EXPRESS = "SEND_EXPRESS";
const char* const Protocol::CMD_SIGN_EXPRESS = "SIGN_EXPRESS";
const char* const Protocol::CMD_SIGN_MANY = "SIGN_MANY";
const char* const Protocol::CMD_UPDATE_DESCRIPTION = "UPDATE_DESCRIPTION";
const char* const Protocol::CMD_QUERY_SENT = "QUERY_SENT";
const char* const Protocol::CMD_QUERY_RECEIVE = "QUERY_RECEIVE";
const char* const Protocol::CMD_QUERY_PENDING = "QUERY_PENDING";
const char* const Protocol::CMD_QUERY_BY_SENDER = "QUERY_BY_SENDER";
const char* const Protocol::CMD_QUERY_BY_RECEIVER = "QUERY_BY_RECEIVER";
const char* const Protocol::CMD_QUERY_BY_TIME = "QUERY_BY_TIME";
const char* const Protocol::CMD_QUERY_BY_TRACKING_NO = "QUERY_BY_TRACKING_NO";
const char* const Protocol::CMD_PICKUP = "PICKUP";
const char* const Protocol::CMD_PICKUP_MANY = "PICKUP_MANY";
const char* const Protocol::CMD_QUERY_PENDING_PICKUP = "QUERY_PENDING_PICKUP";
const char* const Protocol::CMD_QUERY_PICKED_UP = "QUERY_PICKED_UP";
const char* const Protocol::CMD_QUERY_PICKED_UP_BY_SENDER = "QUERY_PICKED_UP_BY_SENDER";
const char* const Protocol::CMD_QUERY_PICKED_UP_BY_RECEIVER = "QUERY_PICKED_UP_BY_RECEIVER";
const char* const Protocol::CMD_QUERY_PICKED_UP_BY_TIME = "QUERY_PICKED_UP_BY_TIME";
const char* const Protocol::CMD_QUERY_PICKED_UP_BY_STATUS = "QUERY_PICKED_UP_BY_STATUS";
const char* const Protocol::CMD_QUERY_PICKED_UP_BY_TRACKING_NO = "QUERY_PICKED_UP_BY_TRACKING_NO";
const char* const Protocol::CMD_QUERY_DELIVERED = "QUERY_DELIVERED";
const char* const Protocol::CMD_QUERY_DELIVERED_BY_SENDER = "QUERY_DELIVERED_BY_SENDER";
const char* const Protocol::CMD_QUERY_DELIVERED_BY_RECEIVER = "QUERY_DELIVERED_BY_RECEIVER";
const char* const Protocol::CMD_QUERY_DELIVERED_BY_TIME = "QUERY_DELIVERED_BY_TIME";
const char* const Protocol::CMD_QUERY_DELIVERED_BY_STATUS = "QUERY_DELIVERED_BY_STATUS";
const char* const Protocol::CMD_QUERY_DELIVERED_BY_TRACKING_NO = "QUERY_DELIVERED_BY_TRACKING_NO";
const char* const Protocol::CMD_ADD_COURIER = "ADD_COURIER";
const char* const Protocol::CMD_BAN_COURIER = "BAN_COURIER";
const char* const Protocol::CMD_ASSIGN_COURIER = "ASSIGN_COURIER";
const char* const Protocol::CMD_BAN_USER = "BAN_USER";
const char* const Protocol::CMD_UNBAN_USER = "UNBAN_USER";
const char* const Protocol::CMD_QUERY_ALL_USERS = "QUERY_ALL_USERS";
const char* const Protocol::CMD_QUERY_ALL_EXPRESS = "QUERY_ALL_EXPRESS";
const char* const Protocol::CMD_QUERY_ALL_COURIER = "QUERY_ALL_COURIER";
const char* const Protocol::CMD_QUERY_ALL_BY_USER = "QUERY_ALL_BY_USER";
const char* const Protocol::CMD_QUERY_ALL_BY_TIME = "QUERY_ALL_BY_TIME";
const char* const Protocol::CMD_QUERY_ALL_BY_TRACKING_NO = "QUERY_ALL_BY_TRACKING_NO";
const char* const Protocol::CMD_QUERY_ALL_BY_COURIER = "QUERY_ALL_BY_COURIER";

std::vector<std::string> Protocol::split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        parts.push_back(item);
    }
    // std::getline 不会为以 delim 结尾的串补一个空 item, 但我们的协议
    // 不依赖结尾空字段, 故此处不补
    return parts;
}

std::string Protocol::join(const std::vector<std::string>& parts, char delim) {
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += delim;
        result += parts[i];
    }
    return result;
}

std::string Protocol::escapeField(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '|':  out += "\\p";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

std::string Protocol::unescapeField(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char nxt = s[i + 1];
            switch (nxt) {
                case '\\': out += '\\'; ++i; break;
                case 'p':  out += '|';  ++i; break;
                case 'n':  out += '\n'; ++i; break;
                case 'r':  out += '\r'; ++i; break;
                default:   out += s[i]; break;  // 未知转义, 原样保留
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

std::string Protocol::serializeResult(const Result& r) {
    return std::to_string(static_cast<int>(r.code)) + "|" + escapeField(r.message);
}

Result Protocol::deserializeResult(const std::string& s) {
    auto parts = split(s, '|');
    if (parts.empty()) return Result(ErrorCode::Success, "");
    int code = 0;
    try { code = std::stoi(parts[0]); } catch (...) { code = 0; }
    std::string msg;
    if (parts.size() >= 2) msg = unescapeField(parts[1]);
    return Result(static_cast<ErrorCode>(code), msg);
}

std::string Protocol::serializeSendResult(const SendResult& sr) {
    // 固定 3 字段: code | escapedMessage | trackingNo
    return std::to_string(static_cast<int>(sr.result.code)) + "|"
        + escapeField(sr.result.message) + "|"
        + std::to_string(sr.trackingNo);
}

SendResult Protocol::deserializeSendResult(const std::string& s) {
    auto parts = split(s, '|');
    if (parts.size() < 3) return {deserializeResult(s), 0};
    int code = 0;
    try { code = std::stoi(parts[0]); } catch (...) { code = 0; }
    std::string msg = unescapeField(parts[1]);
    unsigned long long no = 0;
    try { no = std::stoull(parts[2]); } catch (...) { no = 0; }
    return {Result(static_cast<ErrorCode>(code), msg), no};
}

std::string Protocol::serializeBatchResult(const BatchResult& br) {
    // 格式: totalRequested | succeeded | failureCount
    //       | no1 | code1 | message1
    //       | no2 | code2 | message2 ...
    std::vector<std::string> parts;
    parts.push_back(std::to_string(br.totalRequested));
    parts.push_back(std::to_string(br.succeeded));
    parts.push_back(std::to_string(br.failures.size()));
    for (const auto& f : br.failures) {
        parts.push_back(std::to_string(f.first));
        parts.push_back(std::to_string(static_cast<int>(f.second.code)));
        parts.push_back(escapeField(f.second.message));
    }
    return join(parts, '|');
}

BatchResult Protocol::deserializeBatchResult(const std::string& s) {
    auto parts = split(s, '|');
    if (parts.size() < 3) return {};
    BatchResult br;
    try {
        br.totalRequested = std::stoull(parts[0]);
        br.succeeded = std::stoull(parts[1]);
    } catch (...) {
        return {};
    }
    size_t failureCount = 0;
    try { failureCount = std::stoull(parts[2]); } catch (...) { failureCount = 0; }
    size_t idx = 3;
    for (size_t i = 0; i < failureCount && idx + 2 < parts.size(); ++i) {
        unsigned long long no = 0;
        int code = 0;
        try { no = std::stoull(parts[idx]); } catch (...) { no = 0; }
        try { code = std::stoi(parts[idx + 1]); } catch (...) { code = 0; }
        std::string msg = unescapeField(parts[idx + 2]);
        idx += 3;
        br.failures.emplace_back(no, Result(static_cast<ErrorCode>(code), msg));
    }
    return br;
}

static std::string serializeTime(const std::optional<time_t>& t) {
    if (!t.has_value()) return "N";
    return std::to_string(*t);
}

static std::optional<time_t> deserializeTime(const std::string& s) {
    if (s == "N" || s.empty()) return std::nullopt;
    try {
        return static_cast<time_t>(std::stoll(s));
    } catch (...) {
        return std::nullopt;
    }
}

// 对 optional<string> 序列化为单字段, 已 escape, 故 'N' 这种字符串原值
// 经 escape 后仍是 'N', 与 nullopt 标记重合; 因此约定 nullopt 写成 'N',
// 业务上不会出现长度为 1 且恰为 'N' 的关键字段
static std::string serializeOptionalString(const std::optional<std::string>& s) {
    if (!s.has_value() || s->empty()) return "N";
    return Protocol::escapeField(*s);
}

static std::optional<std::string> deserializeOptionalString(const std::string& s) {
    if (s == "N" || s.empty()) return std::nullopt;
    return Protocol::unescapeField(s);
}

// BaseObject 序列化为 3 字段: type | escapedDescription | param
static std::string serializeBaseObject(const BaseObject& obj) {
    std::string typeStr;
    std::string param;
    if (obj.isObject()) {
        typeStr = "0";
        param = std::to_string(dynamic_cast<const Object&>(obj).getWeight());
    } else if (obj.isBook()) {
        typeStr = "1";
        param = std::to_string(dynamic_cast<const Book&>(obj).getCopy());
    } else {
        typeStr = "2";
        param = std::to_string(dynamic_cast<const Fragile&>(obj).getWeight());
    }
    return typeStr + "|" + Protocol::escapeField(obj.getDescription()) + "|" + param;
}

static std::unique_ptr<BaseObject> deserializeBaseObject(int type, const std::string& escapedDesc, long long param) {
    std::string desc = Protocol::unescapeField(escapedDesc);
    if (type == 0) return std::make_unique<Object>(desc, param);
    if (type == 1) return std::make_unique<Book>(desc, param);
    return std::make_unique<Fragile>(desc, param);
}

// Express 序列化共 10 个字段:
// trackingNo | status | escapedSender | escapedReceiver | sendTime | receiveTime
// | optionalEscapedCourier | objType | escapedDescription | param
std::string Protocol::serializeExpress(const Express& e) {
    std::vector<std::string> parts;
    parts.push_back(std::to_string(e.getTrackingNo()));
    parts.push_back(std::to_string(static_cast<int>(e.getStatus())));
    parts.push_back(escapeField(e.getSender()));
    parts.push_back(escapeField(e.getReceiver()));
    parts.push_back(serializeTime(e.getSendTime()));
    parts.push_back(serializeTime(e.getReceiveTime()));
    parts.push_back(serializeOptionalString(
        e.getCourier().empty() ? std::optional<std::string>() : std::optional<std::string>(e.getCourier())
    ));
    parts.push_back(serializeBaseObject(*e.getObject()));
    return join(parts, '|');
}

std::unique_ptr<Express> Protocol::deserializeExpress(const std::string& s) {
    auto parts = split(s, '|');
    if (parts.size() < 10) return nullptr;
    try {
        unsigned long long trackingNo = std::stoull(parts[0]);
        Status status = static_cast<Status>(std::stoi(parts[1]));
        std::string sender = unescapeField(parts[2]);
        std::string receiver = unescapeField(parts[3]);
        std::optional<time_t> sendTime = deserializeTime(parts[4]);
        std::optional<time_t> receiveTime = deserializeTime(parts[5]);
        std::optional<std::string> courier = deserializeOptionalString(parts[6]);
        int objType = std::stoi(parts[7]);
        long long objParam = std::stoll(parts[9]);
        auto object = deserializeBaseObject(objType, parts[8], objParam);
        if (!object) return nullptr;
        return std::unique_ptr<Express>(new Express(
            trackingNo, status, sender, receiver,
            sendTime, receiveTime, courier, std::move(object)
        ));
    } catch (...) {
        return nullptr;
    }
}

// User 序列化:
// permission | escapedUserName | escapedName | balance | banned | hash | length
// User 多 2 字段: | escapedAddress | escapedPhone   (共 9 字段)
// Courier 多 1 字段: | escapedPhone                  (共 8 字段)
// Admin 不多额外字段                                  (共 7 字段)
// 注意: 把 permission 放在第 0 字段, 方便列表反序列化时按权限取字段数
std::string Protocol::serializeUser(const BaseUser& u) {
    std::vector<std::string> parts;
    parts.push_back(std::to_string(static_cast<int>(u.getPermission())));
    parts.push_back(escapeField(u.getUserName()));
    parts.push_back(escapeField(u.getName()));
    parts.push_back(std::to_string(u.getBalance()));
    parts.push_back(u.isBanned() ? "1" : "0");
    // HashedPassword 内部值: 占位符 0|0, 因为网络传输不携带真实密码
    parts.push_back("0");
    parts.push_back("0");
    if (u.isUser()) {
        const User* user = dynamic_cast<const User*>(&u);
        parts.push_back(escapeField(user->getAddress()));
        parts.push_back(escapeField(user->getPhone()));
    } else if (u.isCourier()) {
        const Courier* courier = dynamic_cast<const Courier*>(&u);
        parts.push_back(escapeField(courier->getPhone()));
    }
    return join(parts, '|');
}

std::unique_ptr<BaseUser> Protocol::deserializeUser(const std::string& s) {
    auto parts = split(s, '|');
    if (parts.size() < 7) return nullptr;
    try {
        Permission perm = static_cast<Permission>(std::stoi(parts[0]));
        std::string userName = unescapeField(parts[1]);
        std::string name = unescapeField(parts[2]);
        long long balance = std::stoll(parts[3]);
        bool banned = parts[4] == "1";
        size_t hash = std::stoull(parts[5]);
        size_t length = std::stoull(parts[6]);
        HashedPassword pwd(hash, length);

        if (perm == Permission::Admin) {
            return std::unique_ptr<BaseUser>(new Admin(userName, std::move(pwd), banned, name, balance));
        } else if (perm == Permission::User) {
            if (parts.size() < 9) return nullptr;
            return std::unique_ptr<BaseUser>(new User(
                userName, std::move(pwd), banned, name, balance,
                unescapeField(parts[7]), unescapeField(parts[8])
            ));
        } else {
            if (parts.size() < 8) return nullptr;
            return std::unique_ptr<BaseUser>(new Courier(
                userName, std::move(pwd), banned, name, balance,
                unescapeField(parts[7])
            ));
        }
    } catch (...) {
        return nullptr;
    }
}

std::string Protocol::serializeExpressList(const std::vector<const Express*>& list) {
    std::vector<std::string> parts;
    parts.push_back(std::to_string(list.size()));
    for (const auto* e : list) {
        if (e) parts.push_back(serializeExpress(*e));
    }
    return join(parts, '|');
}

std::string Protocol::serializeUserList(const std::vector<const BaseUser*>& list) {
    std::vector<std::string> parts;
    parts.push_back(std::to_string(list.size()));
    for (const auto* u : list) {
        if (u) parts.push_back(serializeUser(*u));
    }
    return join(parts, '|');
}

std::vector<std::unique_ptr<Express>> Protocol::deserializeExpressList(const std::string& s) {
    std::vector<std::unique_ptr<Express>> result;
    auto parts = split(s, '|');
    if (parts.empty()) return result;
    size_t count = 0;
    try { count = std::stoull(parts[0]); } catch (...) { return result; }
    constexpr size_t EXPRESS_FIELD_COUNT = 10;
    size_t idx = 1;
    for (size_t i = 0; i < count; ++i) {
        if (idx + EXPRESS_FIELD_COUNT > parts.size()) break;
        std::vector<std::string> subParts(parts.begin() + idx, parts.begin() + idx + EXPRESS_FIELD_COUNT);
        std::string expressStr = join(subParts, '|');
        idx += EXPRESS_FIELD_COUNT;
        auto e = deserializeExpress(expressStr);
        if (e) result.push_back(std::move(e));
    }
    return result;
}

std::vector<std::unique_ptr<BaseUser>> Protocol::deserializeUserList(const std::string& s) {
    std::vector<std::unique_ptr<BaseUser>> result;
    auto parts = split(s, '|');
    if (parts.empty()) return result;
    size_t count = 0;
    try { count = std::stoull(parts[0]); } catch (...) { return result; }
    size_t idx = 1;
    for (size_t i = 0; i < count; ++i) {
        if (idx >= parts.size()) break;
        // 按 permission 字段(第 0 个相对位置)判断字段总数
        Permission perm;
        try {
            perm = static_cast<Permission>(std::stoi(parts[idx]));
        } catch (...) { break; }
        size_t fieldCount = 7;
        if (perm == Permission::User) fieldCount = 9;
        else if (perm == Permission::Courier) fieldCount = 8;
        if (idx + fieldCount > parts.size()) break;
        std::vector<std::string> userParts(parts.begin() + idx, parts.begin() + idx + fieldCount);
        auto u = deserializeUser(join(userParts, '|'));
        if (u) result.push_back(std::move(u));
        idx += fieldCount;
    }
    return result;
}

std::string Protocol::okResponse(const std::string& data) {
    if (data.empty()) return "OK";
    return "OK|" + data;
}

std::string Protocol::errResponse(const Result& r) {
    return "ERR|" + serializeResult(r);
}

bool Protocol::isOk(const std::string& response) {
    // 必须是精确 "OK" 或以 "OK|" 开头, 避免 "OKXXX" 被误判
    return response == "OK" || response.rfind("OK|", 0) == 0;
}

std::string Protocol::extractData(const std::string& response) {
    if (response.rfind("OK|", 0) == 0) {
        return response.substr(3);
    }
    if (response == "OK") {
        return "";
    }
    if (response.rfind("ERR|", 0) == 0) {
        return response.substr(4);
    }
    return response;
}
