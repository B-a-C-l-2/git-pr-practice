#pragma once

#include "config.hpp"
#include "Client.hpp"
#include "Result.hpp"
#include "Express.hpp"
#include "BaseUser.hpp"
#include <string>
#include <optional>
#include <vector>
#include <memory>

/*
Session 类在客户端维护当前登录状态，并将所有业务操作转发给服务器。
内部使用 Client 发送请求，并缓存反序列化后的查询结果，确保返回的指针
在 UI 打印期间保持有效。
登录成功后, 由服务器一次性下发当前用户的权限和封禁状态, Session 据此
缓存身份, 后续 currentIsAdmin / currentIsCourier / currentIsBanned 等
判断直接读缓存, 不再通过"尝试调用业务接口是否 OK"来推断角色 (那种方式
会把"权限不足但服务端返回空列表"误判为有权限).
*/

class Session {
private:
    Client& client;                                    // 网络客户端
    std::string currentUserName;                        // 当前登录用户，空串=未登录
    Permission currentPermission = Permission::User;    // 当前用户权限缓存
    bool currentBanned = false;                         // 当前用户是否被封禁缓存

    // 缓存上次查询结果，保证返回的裸指针在 UI 使用期间有效
    mutable std::vector<std::unique_ptr<Express>> lastExpresses;
    mutable std::vector<std::unique_ptr<BaseUser>> lastUsers;
    mutable std::unique_ptr<Express> lastExpress;
    mutable std::unique_ptr<BaseUser> lastUser;

    // 辅助: 查询 Express 列表
    std::vector<const Express*> queryExpressList(const std::string& cmd,
        const std::vector<std::string>& args) const;
    // 辅助: 查询 User 列表
    std::vector<const BaseUser*> queryUserList(const std::string& cmd,
        const std::vector<std::string>& args) const;
    // 辅助: 查询单个 Express
    const Express* querySingleExpress(const std::string& cmd,
        const std::vector<std::string>& args) const;

public:
    Session(Client& _client);
    Session() = delete;
    Session(const Session&) = delete;
    Session(Session&&) = default;
    Session& operator=(const Session&) = delete;
    Session& operator=(Session&&) = default;

    Result login(std::string userName, const std::string& password);
    void logout();
    bool isLoggedIn() const;
    bool currentIsAdmin() const;
    bool currentIsUser() const;
    bool currentIsCourier() const;
    bool currentIsBanned() const;
    const std::string& getCurrentUserName() const;

    // 游客操作
    Result registerUser(
        std::string userName,
        const std::string& password,
        std::string name,
        std::string address,
        std::string phone
    );

    // 普通用户操作
    Result changeMyPassword(const std::string& newPassword);
    std::optional<long long> queryMyBalance() const;
    Result recharge(const long long money);
    SendResult sendExpress(
        std::string receiver,
        std::unique_ptr<BaseObject> object
    );
    Result signExpress(
        const unsigned long long trackingNo
    );
    BatchResult signMany(
        const std::vector<unsigned long long>& trackingNos
    );
    Result updateDescription(
        const unsigned long long trackingNo,
        std::string newDescription
    );
    std::vector<const Express*> queryMySent() const;
    std::vector<const Express*> queryMyReceive() const;
    std::vector<const Express*> queryMyPending() const;
    std::vector<const Express*> queryMyBySender(
        const std::string& sender
    ) const;
    std::vector<const Express*> queryMyByReceiver(
        const std::string& receiver
    ) const;
    std::vector<const Express*> queryMyByTime(
        const time_t start, const time_t end
    ) const;
    const Express* queryMyByTrackingNo(
        const unsigned long long trackingNo
    ) const;

    // 快递员操作
    Result pickup(
        const unsigned long long trackingNo
    );
    BatchResult pickupMany(
        const std::vector<unsigned long long>& trackingNos
    );
    std::vector<const Express*> queryPendingPickup() const;
    std::vector<const Express*> queryPickedUp() const;
    std::vector<const Express*> queryPickedUpBySender(
        const std::string& sender
    ) const;
    std::vector<const Express*> queryPickedUpByReceiver(
        const std::string& receiver
    ) const;
    std::vector<const Express*> queryPickedUpByTime(
        const time_t start, const time_t end
    ) const;
    std::vector<const Express*> queryPickedUpByStatus(
        const Status status
    ) const;
    const Express* queryPickedUpByTrackingNo(
        const unsigned long long trackingNo
    ) const;

    std::vector<const Express*> queryDelivered() const;
    std::vector<const Express*> queryDeliveredBySender(
        const std::string& sender
    ) const;
    std::vector<const Express*> queryDeliveredByReceiver(
        const std::string& receiver
    ) const;
    std::vector<const Express*> queryDeliveredByTime(
        const time_t start, const time_t end
    ) const;
    std::vector<const Express*> queryDeliveredByStatus(
        const Status status
    ) const;
    const Express* queryDeliveredByTrackingNo(
        const unsigned long long trackingNo
    ) const;

    Result addCourier(
        std::string userName,
        const std::string& password,
        std::string name,
        std::string phone
    );
    Result banCourier(
        const std::string& userName
    );
    Result assignCourier(
        const std::string& userName,
        const unsigned long long trackingNo
    );
    Result banUser(
        const std::string& userName
    );
    Result unbanUser(
        const std::string& userName
    );
    std::vector<const BaseUser*> queryAllUsers() const;
    std::vector<const Express*> queryAllExpress() const;
    std::vector<const BaseUser*> queryAllCourier() const;
    std::vector<const Express*> queryAllByUser(
        const std::string& userName
    ) const;
    std::vector<const Express*> queryAllByTime(
        const time_t start, const time_t end
    ) const;
    const Express* queryAllByTrackingNo(
        const unsigned long long trackingNo
    ) const;
    std::vector<const Express*> queryAllByCourier(
        const std::string& courier
    ) const;
};
