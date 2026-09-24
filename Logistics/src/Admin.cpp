#include "Admin.hpp"

Admin::Admin(
    std::string _userName,
    HashedPassword _password,
    const bool _banned,
    std::string _name,
    const long long _balance
)
: BaseUser(std::move(_userName), std::move(_password), _banned, std::move(_name), Permission::Admin, _balance)
{}

Admin::Admin(
    std::string _userName,
    const std::string& _password,
    std::string _name
)
: BaseUser(std::move(_userName), _password, std::move(_name), Permission::Admin)
{}