#pragma once

#include "BaseUser.hpp"

class Admin : public BaseUser {
private:
    // 方便数据恢复
    Admin(
        std::string _userName,
        HashedPassword _password,
        const bool _banned,
        std::string _name,
        const long long _balance
    );

public:
    Admin(
        std::string _userName,
        const std::string& _password,
        std::string _name
    );

    // 持久化
    friend std::unique_ptr<BaseUser> fileio::readUser(std::istream& is);

    // 网络传输: Protocol 需要从网络数据重建管理员对象。
    friend class Protocol;
};