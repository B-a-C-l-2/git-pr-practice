#pragma once

#include "BaseUser.hpp"

class Courier : public BaseUser {
private:
    std::string phone;      // 电话

    static std::string checkPhone(std::string _phone);

    // 方便数据恢复
    Courier(
        std::string _userName,
        HashedPassword _password,
        const bool _banned,
        std::string _name,
        const long long _balance,
        std::string _phone
    );

public:
    Courier(
        std::string _userName,
        const std::string& _password,
        std::string _name,
        std::string _phone
    );

    // getter
    const std::string& getPhone() const;

    // 持久化
    friend std::unique_ptr<BaseUser> fileio::readUser(std::istream& is);

    // 网络传输: Protocol 需要从网络数据重建快递员对象。
    friend class Protocol;
};