#pragma once

#include "config.hpp"
#include "BaseUser.hpp"
#include "User.hpp"
#include "Admin.hpp"
#include "BaseObject.hpp"
#include "Express.hpp"
#include "Result.hpp"
#include <string>
#include <ctime>
#include <vector>
#include <map>
#include <memory>       // for unique_ptr
#include <optional>     // for optional
#include <utility>      // for pair
#include <iosfwd>       // for istream/ostream

class System;
namespace fileio {
    bool saveAll(const System& sys, const std::string& path);
    bool loadAll(System& sys, const std::string& path);
}

/*
如果一个系统只有一个管理员，那么数据规模庞大后，管理难度大，于是设计时，允许
存在多个管理员。但是，要求中提到“扣除的金额转到管理员的账号”，在当前设计下，
需要讨论转移到哪位管理员。引入“公司账户”，可以有效解决此问题。
*/
class System {
private:
    unsigned long long nextTrackingNo = 1;                              // 下一个分配的快递单号
    long long companyBalance = 0;                                       // 公司账户

    // 主存储
    std::map<std::string, std::unique_ptr<BaseUser>> users;             // 用户列表
    std::map<unsigned long long, Express> expresses;                    // 快递列表

    // 二级索引
    std::multimap<std::string, unsigned long long> noBySender;          // 寄件人 -> 快递单号
    std::multimap<std::string, unsigned long long> noByReceiver;        // 收件人 -> 快递单号
    std::multimap<time_t, unsigned long long> noBySendTime;             // 寄送时间 -> 快递单号
    std::multimap<std::string, unsigned long long> noByCourier;         // 快递员 -> 快递单号

    // 增加
    Result addUser(std::unique_ptr<BaseUser> user);
    Result addExpress(Express express);

    // 查找
    const BaseUser* getUser(const std::string& userName) const;
    const BaseUser* getNormalUser(const std::string& userName) const;
    const Express* getExpress(const unsigned long long trackingNo) const;

    // 范围收集: 给定map的迭代器区间, 把对应Express*取出, 可选过滤
    template<typename Iter, typename Pred>
    std::vector<const Express*> collectExpresses(Iter first, Iter last, Pred pred) const {
        std::vector<const Express*> result;
        for (auto it = first; it != last; it++) {
            const Express* e = getExpress(it->second);
            if (e && pred(e)) {
                result.push_back(e);
            }
        }
        return result;
    }

    // 重载: 不需要过滤时的便捷版
    template<typename Iter>
    std::vector<const Express*> collectExpresses(Iter first, Iter last) const {
        return collectExpresses(first, last, [](const Express*){ return true; });
    }

public:
    System() = default;
    System(const System&) = delete;
    System(System&&) noexcept = default;
    System& operator=(const System&) = delete;
    System& operator=(System&&) noexcept = default;
    ~System() = default;

    // 验证
    Result authenticate(const std::string& userName, const std::string& password) const;
    bool isAdmin(const std::string& userName) const;
    bool isUser(const std::string& userName) const;
    bool isCourier(const std::string& userName) const;
    bool isBanned(const std::string& userName) const;
    // 查询身份: 返回 {permission, banned}, 找不到返回 std::nullopt.
    // 用于网络登录响应一次性下发当前用户角色, 避免客户端用"尝试调用"判身份.
    std::optional<std::pair<Permission, bool>> queryIdentity(const std::string& userName) const;

    // 游客操作
    // 注册
    Result registerUser(
        std::string userName,
        const std::string& password,
        std::string name,
        std::string address,
        std::string phone
    );

    // 普通用户操作
    // 修改密码
    Result changePassword(
        const std::string& actor,
        const std::string& target,
        const std::string& newPassword
    );
    // 余额管理
    std::optional<long long> queryBalance(const std::string& userName) const;
    Result recharge(const std::string& userName, const long long money);
    Result deduct(const std::string& userName, const long long money);
    // 发送/接收快递
    SendResult sendExpress(
        std::string sender,
        std::string receiver,
        std::unique_ptr<BaseObject> object
    );
    Result signExpress(
        const std::string& actor,
        const unsigned long long trackingNo
    );
    BatchResult signMany(
        const std::string& actor,
        const std::vector<unsigned long long>& trackingNos
    );
    // 修改快递描述
    Result updateDescription(
        const std::string& actor,
        const unsigned long long trackingNo,
        std::string newDescription
    );
    // 查询快递
    std::vector<const Express*> querySent(          // 所有我寄出的
        const std::string& actor
    ) const;
    std::vector<const Express*> queryReceive(       // 所有给我的
        const std::string& actor
    ) const;
    std::vector<const Express*> queryPending(       // 我待收的
        const std::string& actor
    ) const;
    std::vector<const Express*> queryBySender(      // sender寄给我的
        const std::string& actor,
        const std::string& sender
    ) const;
    std::vector<const Express*> queryByReceiver(    // 我寄给receiver的
        const std::string& actor,
        const std::string& receiver
    ) const;
    std::vector<const Express*> queryByTime(        // 根据时间 (sendTime)
        const std::string& actor,
        const time_t start, const time_t end
    ) const;
    const Express* queryByTrackingNo(               // 根据快递单号
        const std::string& actor,
        const unsigned long long trackingNo
    ) const;

    // 快递员操作
    // 揽收快递
    Result pickup(
        const std::string& actor,
        const unsigned long long trackingNo
    );
    BatchResult pickupMany(
        const std::string& actor,
        const std::vector<unsigned long long>& trackingNos
    );
    // 查看快递
    std::vector<const Express*> queryPendingPickup(         // 未揽收
        const std::string& actor
    ) const;

    std::vector<const Express*> queryPickedUp(              // 已揽收
        const std::string& actor
    ) const;
    std::vector<const Express*> queryPickedUpBySender(      // 根据发送人
        const std::string& actor,
        const std::string& sender
    ) const;
    std::vector<const Express*> queryPickedUpByReceiver(    // 根据接收人
        const std::string& actor,
        const std::string& receiver
    ) const;
    std::vector<const Express*> queryPickedUpByTime(        // 根据时间 (sendTime)
        const std::string& actor,
        const time_t start, const time_t end
    ) const;
    std::vector<const Express*> queryPickedUpByStatus(      // 根据快递状态
        const std::string& actor,
        const Status status
    ) const;
    const Express* queryPickedUpByTrackingNo(               // 根据快递单号
        const std::string& actor,
        const unsigned long long trackingNo
    ) const;

    std::vector<const Express*> queryDelivered(             // 已投递
        const std::string& actor
    ) const;
    std::vector<const Express*> queryDeliveredBySender(     // 根据发送人
        const std::string& actor,
        const std::string& sender
    ) const;
    std::vector<const Express*> queryDeliveredByReceiver(   // 根据接收人
        const std::string& actor,
        const std::string& receiver
    ) const;
    std::vector<const Express*> queryDeliveredByTime(       // 根据时间 (sendTime)
        const std::string& actor,
        const time_t start, const time_t end
    ) const;
    std::vector<const Express*> queryDeliveredByStatus(     // 根据快递状态
        const std::string& actor,
        const Status status
    ) const;
    const Express* queryDeliveredByTrackingNo(              // 根据快递单号
        const std::string& actor,
        const unsigned long long trackingNo
    ) const;

    // 管理员操作
    // 管理快递员
    Result addCourier(
        const std::string& actor,
        std::string userName,
        const std::string& password,
        std::string name,
        std::string phone
    );
    Result banCourier(
        const std::string& actor,
        const std::string& userName
    );
    Result assignCourier(
        const std::string& actor,
        const std::string& userName,
        const unsigned long long trackingNo
    );
    // 封禁
    Result banUser(
        const std::string& actor,
        const std::string& userName
    );
    Result unbanUser(
        const std::string& actor,
        const std::string& userName
    );
    // 查询用户/快递/快递员
    std::vector<const BaseUser*> queryAllUsers(     // 所有用户
        const std::string& actor
    ) const;
    std::vector<const Express*> queryAllExpress(    // 所有快递
        const std::string& actor
    ) const;
    std::vector<const BaseUser*> queryAllCourier(   // 所有快递员
        const std::string& actor
    ) const;
    std::vector<const Express*> queryAllByUser(     // 根据用户
        const std::string& actor,
        const std::string& userName
    ) const;
    std::vector<const Express*> queryAllByTime(     // 根据时间 (sendTime)
        const std::string& actor,
        const time_t start, const time_t end
    ) const;
    const Express* queryAllByTrackingNo(            // 根据快递单号
        const std::string& actor,
        const unsigned long long trackingNo
    ) const;
    std::vector<const Express*> queryAllByCourier(  // 根据快递员
        const std::string& actor,
        const std::string& courier
    ) const;

    // 持久化
    friend bool fileio::saveAll(const System& sys, const std::string& path);
    friend bool fileio::loadAll(System& sys, const std::string& path);

    bool saveToFile(const std::string& filePath = DEFAULT_PATH) const;
    bool loadFromFile(const std::string& filePath = DEFAULT_PATH);

    void addAdmin();
};