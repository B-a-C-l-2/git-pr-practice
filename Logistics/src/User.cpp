#include "User.hpp"
#include <stdexcept>    // for std::invalid_argument

std::string User::checkAddress(std::string _address) {
    if (_address.empty())
        throw std::invalid_argument("地址不能为空！");
    return _address;
}

std::string User::checkPhone(std::string _phone) {
    if (_phone.empty())
        throw std::invalid_argument("电话不能为空！");
    if (_phone.length() != 11)
        throw std::invalid_argument("电话格式错误！");
    if (_phone[0] != '1')
        throw std::invalid_argument("电话格式错误！");
    if (_phone[1] < '3' || _phone[1] > '9')
        throw std::invalid_argument("电话格式错误！");
    for (char c : _phone) {
        if (!('0' <= c && c <= '9'))
            throw std::invalid_argument("电话格式错误！");
    }

    return _phone;
}

User::User(
    std::string _userName,
    HashedPassword _password,
    const bool _banned,
    std::string _name,
    const long long _balance,
    std::string _address,
    std::string _phone
)
: BaseUser(std::move(_userName), std::move(_password), _banned, std::move(_name), Permission::User, _balance)
, address(std::move(_address))
, phone(std::move(_phone))
{}

User::User(
    std::string _userName,
    const std::string& _password,
    std::string _name,
    std::string _address,
    std::string _phone
)
: BaseUser(std::move(_userName), _password, std::move(_name), Permission::User)
, address(checkAddress(std::move(_address)))
, phone(checkPhone(std::move(_phone)))
{}

const std::string& User::getAddress() const {
    return address;
}

const std::string& User::getPhone() const {
    return phone;
}