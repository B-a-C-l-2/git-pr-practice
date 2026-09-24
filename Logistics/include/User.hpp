#pragma once

#include "BaseUser.hpp"

class User : public BaseUser {
private:
    std::string address;    // 地址
    std::string phone;      // 电话

    static std::string checkAddress(std::string _address);
    static std::string checkPhone(std::string _phone);

    // 方便数据恢复
    User(
        std::string _userName,
        HashedPassword _password,
        const bool _banned,
        std::string _name,
        const long long _balance,
        std::string _address,
        std::string _phone
    );

public:
    User(
        std::string _userName,
        const std::string& _password,
        std::string _name,
        std::string _address,
        std::string _phone
    );

    // getter
    const std::string& getAddress() const;
    const std::string& getPhone() const;

    // 持久化
    friend std::unique_ptr<BaseUser> fileio::readUser(std::istream& is);

    // 网络传输: Protocol 需要从网络数据重建用户对象。
    // 恢复构造函数为 protected，若暴露为 public 会破坏封装。
    friend class Protocol;
};