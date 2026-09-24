#include "Session.hpp"
#include "Protocol.hpp"
#include "Object.hpp"
#include "Book.hpp"
#include "Fragile.hpp"

Session::Session(Client& _client) : client(_client) {}

Result Session::login(std::string userName, const std::string& password) {
    if (isLoggedIn())
        return {ErrorCode::LoginFailed, "未退出登录状态"};

    std::string resp = client.call(Protocol::CMD_LOGIN, {userName, password});
    if (!Protocol::isOk(resp)) {
        return Protocol::deserializeResult(Protocol::extractData(resp));
    }
    // 解析 OK|permission|banned, 缓存身份避免后续误判.
    // 如果服务端是老协议只回 OK, 此处回落为默认 User/未封禁.
    auto parts = Protocol::split(Protocol::extractData(resp), '|');
    Permission perm = Permission::User;
    bool banned = false;
    if (parts.size() >= 1 && !parts[0].empty()) {
        try { perm = static_cast<Permission>(std::stoi(parts[0])); } catch (...) {}
    }
    if (parts.size() >= 2 && !parts[1].empty()) {
        banned = parts[1] == "1";
    }
    currentUserName = std::move(userName);
    currentPermission = perm;
    currentBanned = banned;
    return ErrorCode::Success;
}

void Session::logout() {
    currentUserName.clear();
    currentPermission = Permission::User;
    currentBanned = false;
}

bool Session::isLoggedIn() const {
    return !currentUserName.empty();
}

bool Session::currentIsAdmin() const {
    return isLoggedIn() && currentPermission == Permission::Admin;
}

bool Session::currentIsUser() const {
    return isLoggedIn() && currentPermission == Permission::User;
}

bool Session::currentIsCourier() const {
    return isLoggedIn() && currentPermission == Permission::Courier;
}

bool Session::currentIsBanned() const {
    return isLoggedIn() && currentBanned;
}

const std::string& Session::getCurrentUserName() const {
    return currentUserName;
}

Result Session::registerUser(
    std::string userName,
    const std::string& password,
    std::string name,
    std::string address,
    std::string phone
) {
    std::string resp = client.call(Protocol::CMD_REGISTER,
        {userName, password, name, address, phone});
    if (Protocol::isOk(resp)) return ErrorCode::Success;
    return Protocol::deserializeResult(Protocol::extractData(resp));
}

Result Session::changeMyPassword(const std::string& newPassword) {
    if (!isLoggedIn())
        return {ErrorCode::ChangePasswordFailed, "用户未登录"};
    std::string resp = client.call(Protocol::CMD_CHANGE_PASSWORD,
        {currentUserName, currentUserName, newPassword});
    if (Protocol::isOk(resp)) return ErrorCode::Success;
    return Protocol::deserializeResult(Protocol::extractData(resp));
}

std::optional<long long> Session::queryMyBalance() const {
    std::string resp = client.call(Protocol::CMD_QUERY_BALANCE, {currentUserName});
    if (Protocol::isOk(resp)) {
        return std::stoll(Protocol::extractData(resp));
    }
    return std::nullopt;
}

Result Session::recharge(const long long money) {
    std::string resp = client.call(Protocol::CMD_RECHARGE,
        {currentUserName, std::to_string(money)});
    if (Protocol::isOk(resp)) return ErrorCode::Success;
    return Protocol::deserializeResult(Protocol::extractData(resp));
}

SendResult Session::sendExpress(
    std::string receiver,
    std::unique_ptr<BaseObject> object
) {
    if (!object) return {{ErrorCode::AddExpressFailed, "物品为空"}, 0};
    int type = 0;
    long long param = 0;
    if (object->isBook()) {
        type = 1;
        param = dynamic_cast<const Book*>(object.get())->getCopy();
    } else if (object->isFragile()) {
        type = 2;
        param = dynamic_cast<const Fragile*>(object.get())->getWeight();
    } else {
        type = 0;
        param = dynamic_cast<const Object*>(object.get())->getWeight();
    }
    std::string resp = client.call(Protocol::CMD_SEND_EXPRESS,
        {currentUserName, receiver, std::to_string(type), object->getDescription(), std::to_string(param)});
    if (Protocol::isOk(resp)) {
        return Protocol::deserializeSendResult(Protocol::extractData(resp));
    }
    return {Protocol::deserializeResult(Protocol::extractData(resp)), 0};
}

Result Session::signExpress(
    const unsigned long long trackingNo
) {
    std::string resp = client.call(Protocol::CMD_SIGN_EXPRESS,
        {currentUserName, std::to_string(trackingNo)});
    if (Protocol::isOk(resp)) return ErrorCode::Success;
    return Protocol::deserializeResult(Protocol::extractData(resp));
}

BatchResult Session::signMany(
    const std::vector<unsigned long long>& trackingNos
) {
    std::vector<std::string> args = {currentUserName};
    for (auto no : trackingNos) args.push_back(std::to_string(no));
    std::string resp = client.call(Protocol::CMD_SIGN_MANY, args);
    if (Protocol::isOk(resp)) {
        return Protocol::deserializeBatchResult(Protocol::extractData(resp));
    }
    BatchResult br;
    br.totalRequested = trackingNos.size();
    br.failures.emplace_back(0, Protocol::deserializeResult(Protocol::extractData(resp)));
    return br;
}

Result Session::updateDescription(
    const unsigned long long trackingNo,
    std::string newDescription
) {
    std::string resp = client.call(Protocol::CMD_UPDATE_DESCRIPTION,
        {currentUserName, std::to_string(trackingNo), newDescription});
    if (Protocol::isOk(resp)) return ErrorCode::Success;
    return Protocol::deserializeResult(Protocol::extractData(resp));
}

std::vector<const Express*> Session::queryExpressList(
    const std::string& cmd,
    const std::vector<std::string>& args
) const {
    std::string resp = client.call(cmd, args);
    lastExpresses.clear();
    if (!Protocol::isOk(resp)) return {};
    lastExpresses = Protocol::deserializeExpressList(Protocol::extractData(resp));
    std::vector<const Express*> result;
    for (auto& e : lastExpresses) result.push_back(e.get());
    return result;
}

std::vector<const BaseUser*> Session::queryUserList(
    const std::string& cmd,
    const std::vector<std::string>& args
) const {
    std::string resp = client.call(cmd, args);
    lastUsers.clear();
    if (!Protocol::isOk(resp)) return {};
    lastUsers = Protocol::deserializeUserList(Protocol::extractData(resp));
    std::vector<const BaseUser*> result;
    for (auto& u : lastUsers) result.push_back(u.get());
    return result;
}

const Express* Session::querySingleExpress(
    const std::string& cmd,
    const std::vector<std::string>& args
) const {
    std::string resp = client.call(cmd, args);
    lastExpress.reset();
    if (!Protocol::isOk(resp)) return nullptr;
    lastExpress = Protocol::deserializeExpress(Protocol::extractData(resp));
    return lastExpress.get();
}

std::vector<const Express*> Session::queryMySent() const {
    return queryExpressList(Protocol::CMD_QUERY_SENT, {currentUserName});
}

std::vector<const Express*> Session::queryMyReceive() const {
    return queryExpressList(Protocol::CMD_QUERY_RECEIVE, {currentUserName});
}

std::vector<const Express*> Session::queryMyPending() const {
    return queryExpressList(Protocol::CMD_QUERY_PENDING, {currentUserName});
}

std::vector<const Express*> Session::queryMyBySender(
    const std::string& sender
) const {
    return queryExpressList(Protocol::CMD_QUERY_BY_SENDER, {currentUserName, sender});
}

std::vector<const Express*> Session::queryMyByReceiver(
    const std::string& receiver
) const {
    return queryExpressList(Protocol::CMD_QUERY_BY_RECEIVER, {currentUserName, receiver});
}

std::vector<const Express*> Session::queryMyByTime(
    const time_t start, const time_t end
) const {
    return queryExpressList(Protocol::CMD_QUERY_BY_TIME,
        {currentUserName, std::to_string(start), std::to_string(end)});
}

const Express* Session::queryMyByTrackingNo(
    const unsigned long long trackingNo
) const {
    return querySingleExpress(Protocol::CMD_QUERY_BY_TRACKING_NO,
        {currentUserName, std::to_string(trackingNo)});
}

Result Session::pickup(
    const unsigned long long trackingNo
) {
    std::string resp = client.call(Protocol::CMD_PICKUP,
        {currentUserName, std::to_string(trackingNo)});
    if (Protocol::isOk(resp)) return ErrorCode::Success;
    return Protocol::deserializeResult(Protocol::extractData(resp));
}

BatchResult Session::pickupMany(
    const std::vector<unsigned long long>& trackingNos
) {
    std::vector<std::string> args = {currentUserName};
    for (auto no : trackingNos) args.push_back(std::to_string(no));
    std::string resp = client.call(Protocol::CMD_PICKUP_MANY, args);
    if (Protocol::isOk(resp)) {
        return Protocol::deserializeBatchResult(Protocol::extractData(resp));
    }
    BatchResult br;
    br.totalRequested = trackingNos.size();
    br.failures.emplace_back(0, Protocol::deserializeResult(Protocol::extractData(resp)));
    return br;
}

std::vector<const Express*> Session::queryPendingPickup() const {
    return queryExpressList(Protocol::CMD_QUERY_PENDING_PICKUP, {currentUserName});
}

std::vector<const Express*> Session::queryPickedUp() const {
    return queryExpressList(Protocol::CMD_QUERY_PICKED_UP, {currentUserName});
}

std::vector<const Express*> Session::queryPickedUpBySender(
    const std::string& sender
) const {
    return queryExpressList(Protocol::CMD_QUERY_PICKED_UP_BY_SENDER, {currentUserName, sender});
}

std::vector<const Express*> Session::queryPickedUpByReceiver(
    const std::string& receiver
) const {
    return queryExpressList(Protocol::CMD_QUERY_PICKED_UP_BY_RECEIVER, {currentUserName, receiver});
}

std::vector<const Express*> Session::queryPickedUpByTime(
    const time_t start, const time_t end
) const {
    return queryExpressList(Protocol::CMD_QUERY_PICKED_UP_BY_TIME,
        {currentUserName, std::to_string(start), std::to_string(end)});
}

std::vector<const Express*> Session::queryPickedUpByStatus(
    const Status status
) const {
    return queryExpressList(Protocol::CMD_QUERY_PICKED_UP_BY_STATUS,
        {currentUserName, std::to_string(static_cast<int>(status))});
}

const Express* Session::queryPickedUpByTrackingNo(
    const unsigned long long trackingNo
) const {
    return querySingleExpress(Protocol::CMD_QUERY_PICKED_UP_BY_TRACKING_NO,
        {currentUserName, std::to_string(trackingNo)});
}

std::vector<const Express*> Session::queryDelivered() const {
    return queryExpressList(Protocol::CMD_QUERY_DELIVERED, {currentUserName});
}

std::vector<const Express*> Session::queryDeliveredBySender(
    const std::string& sender
) const {
    return queryExpressList(Protocol::CMD_QUERY_DELIVERED_BY_SENDER, {currentUserName, sender});
}

std::vector<const Express*> Session::queryDeliveredByReceiver(
    const std::string& receiver
) const {
    return queryExpressList(Protocol::CMD_QUERY_DELIVERED_BY_RECEIVER, {currentUserName, receiver});
}

std::vector<const Express*> Session::queryDeliveredByTime(
    const time_t start, const time_t end
) const {
    return queryExpressList(Protocol::CMD_QUERY_DELIVERED_BY_TIME,
        {currentUserName, std::to_string(start), std::to_string(end)});
}

std::vector<const Express*> Session::queryDeliveredByStatus(
    const Status status
) const {
    return queryExpressList(Protocol::CMD_QUERY_DELIVERED_BY_STATUS,
        {currentUserName, std::to_string(static_cast<int>(status))});
}

const Express* Session::queryDeliveredByTrackingNo(
    const unsigned long long trackingNo
) const {
    return querySingleExpress(Protocol::CMD_QUERY_DELIVERED_BY_TRACKING_NO,
        {currentUserName, std::to_string(trackingNo)});
}

Result Session::addCourier(
    std::string userName,
    const std::string& password,
    std::string name,
    std::string phone
) {
    std::string resp = client.call(Protocol::CMD_ADD_COURIER,
        {currentUserName, userName, password, name, phone});
    if (Protocol::isOk(resp)) return ErrorCode::Success;
    return Protocol::deserializeResult(Protocol::extractData(resp));
}

Result Session::banCourier(
    const std::string& userName
) {
    std::string resp = client.call(Protocol::CMD_BAN_COURIER,
        {currentUserName, userName});
    if (Protocol::isOk(resp)) return ErrorCode::Success;
    return Protocol::deserializeResult(Protocol::extractData(resp));
}

Result Session::assignCourier(
    const std::string& userName,
    const unsigned long long trackingNo
) {
    std::string resp = client.call(Protocol::CMD_ASSIGN_COURIER,
        {currentUserName, userName, std::to_string(trackingNo)});
    if (Protocol::isOk(resp)) return ErrorCode::Success;
    return Protocol::deserializeResult(Protocol::extractData(resp));
}

Result Session::banUser(
    const std::string& userName
) {
    std::string resp = client.call(Protocol::CMD_BAN_USER,
        {currentUserName, userName});
    if (Protocol::isOk(resp)) return ErrorCode::Success;
    return Protocol::deserializeResult(Protocol::extractData(resp));
}

Result Session::unbanUser(
    const std::string& userName
) {
    std::string resp = client.call(Protocol::CMD_UNBAN_USER,
        {currentUserName, userName});
    if (Protocol::isOk(resp)) return ErrorCode::Success;
    return Protocol::deserializeResult(Protocol::extractData(resp));
}

std::vector<const BaseUser*> Session::queryAllUsers() const {
    return queryUserList(Protocol::CMD_QUERY_ALL_USERS, {currentUserName});
}

std::vector<const Express*> Session::queryAllExpress() const {
    return queryExpressList(Protocol::CMD_QUERY_ALL_EXPRESS, {currentUserName});
}

std::vector<const BaseUser*> Session::queryAllCourier() const {
    return queryUserList(Protocol::CMD_QUERY_ALL_COURIER, {currentUserName});
}

std::vector<const Express*> Session::queryAllByUser(
    const std::string& userName
) const {
    return queryExpressList(Protocol::CMD_QUERY_ALL_BY_USER, {currentUserName, userName});
}

std::vector<const Express*> Session::queryAllByTime(
    const time_t start, const time_t end
) const {
    return queryExpressList(Protocol::CMD_QUERY_ALL_BY_TIME,
        {currentUserName, std::to_string(start), std::to_string(end)});
}

const Express* Session::queryAllByTrackingNo(
    const unsigned long long trackingNo
) const {
    return querySingleExpress(Protocol::CMD_QUERY_ALL_BY_TRACKING_NO,
        {currentUserName, std::to_string(trackingNo)});
}

std::vector<const Express*> Session::queryAllByCourier(
    const std::string& courier
) const {
    return queryExpressList(Protocol::CMD_QUERY_ALL_BY_COURIER, {currentUserName, courier});
}
