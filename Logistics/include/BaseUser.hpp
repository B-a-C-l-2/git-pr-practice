#pragma once

#include "config.hpp"
#include "HashedPassword.hpp"
#include <string>
#include <memory>   // for unique_ptr
#include <iosfwd>   // for istream/ostream

class BaseUser;
namespace fileio {
    bool writeUser(std::ostream& os, const BaseUser& user);
    std::unique_ptr<BaseUser> readUser(std::istream& is);
}

enum class Permission {
    Admin = 0,
    User = 1,
    Courier = 2
};

class BaseUser {
private:
    Permission permission = Permission::User;   // 权限
    HashedPassword password;                    // 密码
    bool banned = false;                        // 软删除

    static std::string checkUserName(std::string _userName);
    static std::string checkName(std::string _name);

protected:
    std::string userName;                       // 用户名
    std::string name;                           // 姓名
    long long balance = 0;                      // 余额 使用“分”为单位

    // 方便数据恢复
    BaseUser(
        std::string _userName,
        HashedPassword _password,
        const bool _banned,
        std::string _name,
        const Permission _permission,
        const long long _balance
    );

    BaseUser(
        std::string _userName,
        const std::string& _password,
        std::string _name,
        const Permission _permission = Permission::User
    );

public:
    BaseUser() = delete;
    BaseUser(const BaseUser&) = delete;
    BaseUser(BaseUser&&) noexcept = default;
    BaseUser& operator=(const BaseUser&) = delete;
    BaseUser& operator=(BaseUser&&) noexcept = default;
    virtual ~BaseUser() = default;

    // getter
    const std::string& getUserName() const;
    const std::string& getName() const;
    long long getBalance() const;
    Permission getPermission() const;
    
    // 余额变动
    bool pay(const long long money);
    void income(const long long money);

    // 封禁
    void ban();
    void unban();

    // 密码相关
    bool verifyPassword(const std::string& input) const;
    void changePassword(const std::string& newPassword);

    bool isUser() const;
    bool isAdmin() const;
    bool isCourier() const;
    bool isBanned() const;

    // 持久化
    friend bool fileio::writeUser(std::ostream& os, const BaseUser& user);
    friend std::unique_ptr<BaseUser> fileio::readUser(std::istream& is);

    // 网络传输: Protocol 需要从网络数据重建用户对象，
    // 若不设为友元，则必须提供 public 恢复构造函数或 setter，
    // 会暴露内部实现并降低封装。
    friend class Protocol;
};