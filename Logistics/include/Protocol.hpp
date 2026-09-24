#pragma once

#include "Result.hpp"
#include "Express.hpp"
#include "BaseUser.hpp"
#include <string>
#include <vector>
#include <sstream>

/*
通信协议说明：
采用文本行协议，每条消息以换行符 '\n' 结尾，字段间以 '|' 分隔。
请求格式：COMMAND|arg1|arg2|...
响应格式：OK|...  或  ERR|ErrorCode|message

由于业务字段(用户名、地址、描述、错误信息等)可能含有 '|' 或换行符，
直接拼接会破坏字段边界。因此所有字符串字段在拼接前都必须经过
Protocol::escapeField 转义，解析后再经 Protocol::unescapeField 还原。
转义规则：
    '\\' -> "\\\\"
    '|'  -> "\\p"
    '\n' -> "\\n"
    '\r' -> "\\r"

由于 socket 通信双方需要交换结构化的业务对象，而各对象的私有成员
不允许外部随意修改，故设计一组序列化/反序列化辅助函数。这些函数
仅负责格式转换，不暴露业务逻辑，因此以静态工具函数形式组织在
Protocol 类中。为保持与原有代码风格一致且满足"非必要不使用全局函数"
的要求，将其封装为类的静态成员函数。
*/

class Protocol {
public:
    // 命令常量
    static const char* const CMD_LOGIN;
    static const char* const CMD_REGISTER;
    static const char* const CMD_CHANGE_PASSWORD;
    static const char* const CMD_QUERY_BALANCE;
    static const char* const CMD_RECHARGE;
    static const char* const CMD_SEND_EXPRESS;
    static const char* const CMD_SIGN_EXPRESS;
    static const char* const CMD_SIGN_MANY;
    static const char* const CMD_UPDATE_DESCRIPTION;
    static const char* const CMD_QUERY_SENT;
    static const char* const CMD_QUERY_RECEIVE;
    static const char* const CMD_QUERY_PENDING;
    static const char* const CMD_QUERY_BY_SENDER;
    static const char* const CMD_QUERY_BY_RECEIVER;
    static const char* const CMD_QUERY_BY_TIME;
    static const char* const CMD_QUERY_BY_TRACKING_NO;
    static const char* const CMD_PICKUP;
    static const char* const CMD_PICKUP_MANY;
    static const char* const CMD_QUERY_PENDING_PICKUP;
    static const char* const CMD_QUERY_PICKED_UP;
    static const char* const CMD_QUERY_PICKED_UP_BY_SENDER;
    static const char* const CMD_QUERY_PICKED_UP_BY_RECEIVER;
    static const char* const CMD_QUERY_PICKED_UP_BY_TIME;
    static const char* const CMD_QUERY_PICKED_UP_BY_STATUS;
    static const char* const CMD_QUERY_PICKED_UP_BY_TRACKING_NO;
    static const char* const CMD_QUERY_DELIVERED;
    static const char* const CMD_QUERY_DELIVERED_BY_SENDER;
    static const char* const CMD_QUERY_DELIVERED_BY_RECEIVER;
    static const char* const CMD_QUERY_DELIVERED_BY_TIME;
    static const char* const CMD_QUERY_DELIVERED_BY_STATUS;
    static const char* const CMD_QUERY_DELIVERED_BY_TRACKING_NO;
    static const char* const CMD_ADD_COURIER;
    static const char* const CMD_BAN_COURIER;
    static const char* const CMD_ASSIGN_COURIER;
    static const char* const CMD_BAN_USER;
    static const char* const CMD_UNBAN_USER;
    static const char* const CMD_QUERY_ALL_USERS;
    static const char* const CMD_QUERY_ALL_EXPRESS;
    static const char* const CMD_QUERY_ALL_COURIER;
    static const char* const CMD_QUERY_ALL_BY_USER;
    static const char* const CMD_QUERY_ALL_BY_TIME;
    static const char* const CMD_QUERY_ALL_BY_TRACKING_NO;
    static const char* const CMD_QUERY_ALL_BY_COURIER;

    // 序列化
    static std::string serializeResult(const Result& r);
    static std::string serializeSendResult(const SendResult& sr);
    static std::string serializeBatchResult(const BatchResult& br);
    static std::string serializeExpress(const Express& e);
    static std::string serializeUser(const BaseUser& u);

    // 反序列化
    static Result deserializeResult(const std::string& s);
    static SendResult deserializeSendResult(const std::string& s);
    static BatchResult deserializeBatchResult(const std::string& s);
    static std::unique_ptr<Express> deserializeExpress(const std::string& s);
    static std::unique_ptr<BaseUser> deserializeUser(const std::string& s);

    // 列表包装: OK|COUNT|item1|item2|...
    static std::string serializeExpressList(const std::vector<const Express*>& list);
    static std::string serializeUserList(const std::vector<const BaseUser*>& list);
    static std::vector<std::unique_ptr<Express>> deserializeExpressList(const std::string& s);
    static std::vector<std::unique_ptr<BaseUser>> deserializeUserList(const std::string& s);

    // 辅助: 解析单个字段 (按 '|' 分割)
    static std::vector<std::string> split(const std::string& s, char delim);
    static std::string join(const std::vector<std::string>& parts, char delim);

    // 字段转义/反转义: 字符串字段在拼接进 '|' 分隔协议前必须 escape
    static std::string escapeField(const std::string& s);
    static std::string unescapeField(const std::string& s);

    // 响应包装
    static std::string okResponse(const std::string& data = "");
    static std::string errResponse(const Result& r);
    static bool isOk(const std::string& response);
    static std::string extractData(const std::string& response);

private:
    Protocol() = delete;
};
