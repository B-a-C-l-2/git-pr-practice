#include "BaseUser.hpp"
#include <stdexcept>    // for std::invalid_argument

std::string BaseUser::checkUserName(std::string _userName) {
    if (_userName.empty())
        throw std::invalid_argument("用户名不能为空！");
    return _userName;
}

std::string BaseUser::checkName(std::string _name) {
    if (_name.empty())
        throw std::invalid_argument("姓名不能为空！");
    return _name;
}

BaseUser::BaseUser(
    std::string _userName,
    HashedPassword _password,
    const bool _banned,
    std::string _name,
    const Permission _permission,
    const long long _balance
)
: permission(_permission)
, password(std::move(_password))
, banned(_banned)
, userName(std::move(_userName))
, name(std::move(_name))
, balance(_balance)
{}

BaseUser::BaseUser(
    std::string _userName,
    const std::string& _password,
    std::string _name,
    const Permission _permission
)
: permission(_permission)
, password(_password)
, banned(false)
, userName(checkUserName(std::move(_userName)))
, name(checkName(std::move(_name)))
, balance(0)
{}

const std::string& BaseUser::getUserName() const {
    return userName;
}

const std::string& BaseUser::getName() const {
    return name;
}

long long BaseUser::getBalance() const {
    return balance;
}

Permission BaseUser::getPermission() const {
    return permission;
}

void BaseUser::ban() {
    banned = true;
}

void BaseUser::unban() {
    banned = false;
}

bool BaseUser::pay(const long long money) {
    if (money < 0)
        throw std::invalid_argument("支付金额不能为负数！");
    if (balance < money)
        return false;

    balance -= money;
    return true;
}

void BaseUser::income(const long long money) {
    if (money < 0)
        throw std::invalid_argument("收入金额不能为负数！");

    balance += money;
}

bool BaseUser::verifyPassword(const std::string& input) const {
    return password.verify(input);
}

void BaseUser::changePassword(const std::string& newPassword) {
    password.change(newPassword);
}

bool BaseUser::isUser() const {
    return permission == Permission::User;
}

bool BaseUser::isAdmin() const {
    return permission == Permission::Admin;
}

bool BaseUser::isCourier() const {
    return permission == Permission::Courier;
}

bool BaseUser::isBanned() const {
    return banned;
}