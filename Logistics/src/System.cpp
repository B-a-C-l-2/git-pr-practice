#include "System.hpp"
#include <stdexcept>    // for std::invalid_argument

Result System::addUser(std::unique_ptr<BaseUser> user) {
    std::string userName = user->getUserName();
    if (getUser(userName))
        return {ErrorCode::AddUserFailed, "用户已存在"};

    users.emplace(userName, std::move(user));
    return ErrorCode::Success;
}

Result System::addExpress(Express express) {
    unsigned long long trackingNo = express.getTrackingNo();
    if (getExpress(trackingNo))
        return {ErrorCode::AddExpressFailed, "快递已存在"};

    std::string sender = express.getSender();
    if (!getNormalUser(sender))
        return {ErrorCode::AddExpressFailed, "寄件人不存在或已被封禁"};

    std::string receiver = express.getReceiver();
    if (!getNormalUser(receiver))
        return {ErrorCode::AddExpressFailed, "收件人不存在或已被封禁"};

    std::optional<time_t> sendTimeOpt = express.getSendTime();
    if (!sendTimeOpt.has_value())
        return {ErrorCode::AddExpressFailed, "寄送时间无效"};

    time_t sendTime = *sendTimeOpt;

    expresses.emplace(trackingNo, std::move(express));
    noBySender.emplace(sender, trackingNo);
    noByReceiver.emplace(receiver, trackingNo);
    noBySendTime.emplace(sendTime, trackingNo);
    return ErrorCode::Success;
}

const BaseUser* System::getUser(const std::string& userName) const {
    auto it = users.find(userName);
    return it != users.end() ? it->second.get() : nullptr;
}

const BaseUser* System::getNormalUser(const std::string& userName) const {
    const BaseUser* u = getUser(userName);
    if (!u) return nullptr;
    return !u->isBanned() ? u : nullptr;
}

const Express* System::getExpress(const unsigned long long trackingNo) const {
    auto it = expresses.find(trackingNo);
    return it != expresses.end() ? &it->second : nullptr;
}

Result System::authenticate(const std::string& userName, const std::string& password) const {
    const BaseUser* u = getNormalUser(userName);
    if (!u)
        return {ErrorCode::LoginFailed, "用户不存在或已被封禁"};
    if (!u->verifyPassword(password))
        return {ErrorCode::LoginFailed, "密码错误"};
    return ErrorCode::Success;
}

bool System::isAdmin(const std::string& userName) const {
    const BaseUser* u = getNormalUser(userName);
    return u && u->isAdmin();
}

bool System::isUser(const std::string& userName) const {
    const BaseUser* u = getNormalUser(userName);
    return u && u->isUser();
}

bool System::isCourier(const std::string& userName) const {
    const BaseUser* u = getNormalUser(userName);
    return u && u->isCourier();
}

bool System::isBanned(const std::string& userName) const {
    const BaseUser* u = getUser(userName);
    return u && u->isBanned();
}

std::optional<std::pair<Permission, bool>> System::queryIdentity(const std::string& userName) const {
    const BaseUser* u = getUser(userName);
    if (!u) return std::nullopt;
    return std::make_pair(u->getPermission(), u->isBanned());
}

Result System::registerUser(
    std::string userName,
    const std::string& password,
    std::string name,
    std::string address,
    std::string phone
) {
    if (getUser(userName))
        return {ErrorCode::AddUserFailed, "用户名被占用"};

    try {
        return addUser(
            std::make_unique<User>(
                std::move(userName),
                password,
                std::move(name),
                std::move(address),
                std::move(phone)
            )
        );
    } catch (const std::invalid_argument& e) {
        return {ErrorCode::AddUserFailed, e.what()};
    }
}

Result System::changePassword(
    const std::string& actor,
    const std::string& target,
    const std::string& newPassword
) {
    if (!getUser(target))
        return {ErrorCode::ChangePasswordFailed, "用户不存在"};
    if (actor != target && !isAdmin(actor))
        return {ErrorCode::ChangePasswordFailed, "权限不足"};

    try {
        users.at(target)->changePassword(newPassword);
    } catch (const std::invalid_argument& e) {
        return {ErrorCode::ChangePasswordFailed, e.what()};
    }

    return ErrorCode::Success;
}

std::optional<long long> System::queryBalance(const std::string& userName) const {
    const BaseUser* u = getNormalUser(userName);
    if (!u) return std::nullopt;
    return u->getBalance();
}

Result System::recharge(const std::string& userName, const long long money) {
    if (!getNormalUser(userName))
        return {ErrorCode::RechargeFailed, "用户不存在或已被封禁"};

    try {
        users.at(userName)->income(money);
    } catch (const std::invalid_argument& e) {
        return {ErrorCode::RechargeFailed, e.what()};
    }

    return ErrorCode::Success;
}

Result System::deduct(const std::string& userName, const long long money) {
    if (!getNormalUser(userName))
        return {ErrorCode::DeductFailed, "用户不存在或已被封禁"};

    try {
        if (!users.at(userName)->pay(money))
            return {ErrorCode::DeductFailed, "余额不足"};
        companyBalance += money;
    } catch (const std::invalid_argument& e) {
        return {ErrorCode::DeductFailed, e.what()};
    }

    return ErrorCode::Success;
}

SendResult System::sendExpress(
    std::string sender,
    std::string receiver,
    std::unique_ptr<BaseObject> object
) {
    if (!getNormalUser(sender))
        return {{ErrorCode::AddExpressFailed, "寄件人不存在或已被封禁"}, 0};
    if (!getNormalUser(receiver))
        return {{ErrorCode::AddExpressFailed, "收件人不存在或已被封禁"}, 0};
    if (receiver == sender)
        return {{ErrorCode::AddExpressFailed, "收件人不能是自己"}, 0};
    
    long long price = object->getPrice();
    Result ded = deduct(sender, price);
    if (!ded.ok())
        return {ded, 0};

    // 保存寄件人用户名，防止 move 后丢失
    const std::string senderName = sender;

    Result add = addExpress(
        Express(nextTrackingNo, std::move(sender), std::move(receiver), std::move(object)
    ));

    if (!add.ok()) {
        users.at(senderName)->income(price);   // 退款
        companyBalance -= price;               // 回滚公司账户
        return {add, 0};
    }

    return {ErrorCode::Success, nextTrackingNo++};
}

Result System::signExpress(
    const std::string& actor,
    const unsigned long long trackingNo
) {
    if (!getUser(actor))
        return {ErrorCode::SignExpressFailed, "用户不存在"};

    const Express* e = getExpress(trackingNo);
    if (!e)
        return {ErrorCode::SignExpressFailed, "快递不存在"};
    if (e->getReceiver() != actor)
        return {ErrorCode::SignExpressFailed, "不能签收其他用户的快递"};
    if (!expresses.at(trackingNo).sign())
        return {ErrorCode::SignExpressFailed, "快递不在快递员身上"};

    return ErrorCode::Success;
}

BatchResult System::signMany(
    const std::string& actor,
    const std::vector<unsigned long long>& trackingNos
) {
    size_t totalRequested = trackingNos.size();
    size_t succeeded = 0;
    std::vector<std::pair<unsigned long long, Result>> failures;

    for (unsigned long long trackingNo : trackingNos) {
        Result sign = signExpress(actor, trackingNo);
        if (sign.ok())
            succeeded++;
        else
            failures.emplace_back(trackingNo, std::move(sign));
    }
    return {totalRequested, succeeded, failures};
}

Result System::updateDescription(
    const std::string& actor,
    const unsigned long long trackingNo,
    std::string newDescription
) {
    if (!getUser(actor))
        return {ErrorCode::UpdateDescriptionFailed, "用户不存在"};

    const Express* e = getExpress(trackingNo);
    if (!e)
        return {ErrorCode::UpdateDescriptionFailed, "快递不存在"};
    if (actor != e->getSender())
        return {ErrorCode::UpdateDescriptionFailed, "只能修改自己寄出的快递"};

    expresses.at(trackingNo).updateDescription(std::move(newDescription));
    return ErrorCode::Success;
}

std::vector<const Express*> System::querySent(
    const std::string& actor
) const {
    if (!getUser(actor)) return {};

    auto range = noBySender.equal_range(actor);
    return collectExpresses(range.first, range.second);
}

std::vector<const Express*> System::queryReceive(
    const std::string& actor
) const {
    if (!getUser(actor)) return {};

    auto range = noByReceiver.equal_range(actor);
    return collectExpresses(range.first, range.second);
}

std::vector<const Express*> System::queryPending(
    const std::string& actor
) const {
    if (!getUser(actor)) return {};

    auto range = noByReceiver.equal_range(actor);
    return collectExpresses(range.first, range.second, [](const Express* e) {
        return e->isPickedUp();
    });
}

std::vector<const Express*> System::queryBySender(
    const std::string& actor,
    const std::string& sender
) const {
    if (!getUser(actor)) return {};

    auto range = noByReceiver.equal_range(actor);
    return collectExpresses(range.first, range.second, [&](const Express* e) {
        return e->getSender() == sender;
    });
}

std::vector<const Express*> System::queryByReceiver(
    const std::string& actor,
    const std::string& receiver
) const {
    if (!getUser(actor)) return {};

    auto range = noBySender.equal_range(actor);
    return collectExpresses(range.first, range.second, [&](const Express* e) {
        return e->getReceiver() == receiver;
    });
}

std::vector<const Express*> System::queryByTime(
    const std::string& actor,
    const time_t start, const time_t end
) const {
    if (!getUser(actor)) return {};

    auto lower = noBySendTime.lower_bound(start);
    auto upper = noBySendTime.upper_bound(end);
    return collectExpresses(lower, upper, [&](const Express* e) {
        return e->getSender() == actor || e->getReceiver() == actor;
    });
}

const Express* System::queryByTrackingNo(
    const std::string& actor,
    const unsigned long long trackingNo
) const {
    if (!getUser(actor)) return {};

    const Express* e = getExpress(trackingNo);
    if (!e) return nullptr;
    if (actor != e->getSender() && actor != e->getReceiver())
        // 不属于当前用户
        return nullptr;

    return e;
}

Result System::pickup(
    const std::string& actor,
    const unsigned long long trackingNo
) {
    if (!isCourier(actor))
        return {ErrorCode::PickupFailed, "您不是快递员"};

    const Express* e = getExpress(trackingNo);
    if (!e)
        return {ErrorCode::PickupFailed, "快递不存在"};
    if (actor != e->getCourier())
        return {ErrorCode::PickupFailed, "该快递不由您负责"};
    if (!expresses.at(trackingNo).pickup())
        return {ErrorCode::PickupFailed, "快递已被揽收或送达"};

    long long price = expresses.at(trackingNo).getPrice();
    long long courierShare = price / 2;
    users.at(actor)->income(courierShare);
    companyBalance -= courierShare;
    return ErrorCode::Success;
}

BatchResult System::pickupMany(
    const std::string& actor,
    const std::vector<unsigned long long>& trackingNos
) {
    size_t totalRequested = trackingNos.size();
    size_t succeeded = 0;
    std::vector<std::pair<unsigned long long, Result>> failures;

    for (unsigned long long trackingNo : trackingNos) {
        Result sign = pickup(actor, trackingNo);
        if (sign.ok())
            succeeded++;
        else
            failures.emplace_back(trackingNo, std::move(sign));
    }
    return {totalRequested, succeeded, failures};
}

std::vector<const Express*> System::queryPendingPickup(
    const std::string& actor
) const {
    if (!isCourier(actor)) return {};

    auto range = noByCourier.equal_range(actor);
    return collectExpresses(range.first, range.second, [](const Express* e) {
        return e->isPendingPickup();
    });
}

std::vector<const Express*> System::queryPickedUp(
    const std::string& actor
) const {
    if (!isCourier(actor)) return {};

    auto range = noByCourier.equal_range(actor);
    return collectExpresses(range.first, range.second, [](const Express* e) {
        return e->isPickedUp();
    });
}

std::vector<const Express*> System::queryPickedUpBySender(
    const std::string& actor,
    const std::string& sender
) const {
    if (!isCourier(actor)) return {};

    auto range = noByCourier.equal_range(actor);
    return collectExpresses(range.first, range.second, [&](const Express* e) {
        return e->isPickedUp() && e->getSender() == sender;
    });
}

std::vector<const Express*> System::queryPickedUpByReceiver(
    const std::string& actor,
    const std::string& receiver
) const {
    if (!isCourier(actor)) return {};

    auto range = noByCourier.equal_range(actor);
    return collectExpresses(range.first, range.second, [&](const Express* e) {
        return e->isPickedUp() && e->getReceiver() == receiver;
    });
}

std::vector<const Express*> System::queryPickedUpByTime(
    const std::string& actor,
    const time_t start, const time_t end
) const {
    if (!isCourier(actor)) return {};

    auto range = noByCourier.equal_range(actor);
    return collectExpresses(range.first, range.second, [&](const Express* e) {
        return e->isPickedUp() && e->getSendTime() >= start && e->getSendTime() <= end;
    });
}

std::vector<const Express*> System::queryPickedUpByStatus(
    const std::string& actor,
    const Status status
) const {
    if (!isCourier(actor)) return {};

    auto range = noByCourier.equal_range(actor);
    return collectExpresses(range.first, range.second, [&](const Express* e) {
        return e->isPickedUp() && e->getStatus() == status;
    });
}

const Express* System::queryPickedUpByTrackingNo(
    const std::string& actor,
    const unsigned long long trackingNo
) const {
    if (!isCourier(actor)) return nullptr;

    const Express* e = getExpress(trackingNo);
    if (!e || !e->isPickedUp() || e->getCourier() != actor) return nullptr;

    return e;
}

std::vector<const Express*> System::queryDelivered(
    const std::string& actor
) const {
    if (!isCourier(actor)) return {};

    auto range = noByCourier.equal_range(actor);
    return collectExpresses(range.first, range.second, [](const Express* e) {
        return e->isReceived();
    });
}

std::vector<const Express*> System::queryDeliveredBySender(
    const std::string& actor,
    const std::string& sender
) const {
    if (!isCourier(actor)) return {};

    auto range = noByCourier.equal_range(actor);
    return collectExpresses(range.first, range.second, [&](const Express* e) {
        return e->isReceived() && e->getSender() == sender;
    });
}

std::vector<const Express*> System::queryDeliveredByReceiver(
    const std::string& actor,
    const std::string& receiver
) const {
    if (!isCourier(actor)) return {};

    auto range = noByCourier.equal_range(actor);
    return collectExpresses(range.first, range.second, [&](const Express* e) {
        return e->isReceived() && e->getReceiver() == receiver;
    });
}

std::vector<const Express*> System::queryDeliveredByTime(
    const std::string& actor,
    const time_t start, const time_t end
) const {
    if (!isCourier(actor)) return {};

    auto range = noByCourier.equal_range(actor);
    return collectExpresses(range.first, range.second, [&](const Express* e) {
        return e->isReceived() && e->getSendTime() >= start && e->getSendTime() <= end;
    });
}

std::vector<const Express*> System::queryDeliveredByStatus(
    const std::string& actor,
    const Status status
) const {
    if (!isCourier(actor)) return {};

    auto range = noByCourier.equal_range(actor);
    return collectExpresses(range.first, range.second, [&](const Express* e) {
        return e->isReceived() && e->getStatus() == status;
    });
}

const Express* System::queryDeliveredByTrackingNo(
    const std::string& actor,
    const unsigned long long trackingNo
) const {
    if (!isCourier(actor)) return nullptr;

    const Express* e = getExpress(trackingNo);
    if (!e || !e->isReceived() || e->getCourier() != actor) return nullptr;

    return e;
}

Result System::addCourier(
    const std::string& actor,
    std::string userName,
    const std::string& password,
    std::string name,
    std::string phone
) {
    if (!isAdmin(actor))
        return {ErrorCode::AddCourierFailed, "权限不足"};

    if (getUser(userName))
        return {ErrorCode::AddCourierFailed, "用户名被占用"};

    try {
        return addUser(
            std::make_unique<Courier>(
                std::move(userName),
                password,
                std::move(name),
                std::move(phone)
            )
        );
    } catch(const std::invalid_argument& e) {
        return {ErrorCode::AddCourierFailed, e.what()};
    }
}

Result System::banCourier(
    const std::string& actor,
    const std::string& userName
) {
    const BaseUser* u = getUser(userName);
    if (!u)
        return {ErrorCode::BanFailed, "用户不存在"};
    if (!u->isCourier())
        return {ErrorCode::BanFailed, "用户不是快递员"};

    return banUser(actor, userName);
}

Result System::assignCourier(
    const std::string& actor,
    const std::string& userName,
    const unsigned long long trackingNo
) {
    if (!isAdmin(actor))
        return {ErrorCode::AssignCourierFailed, "权限不足"};
    if (!isCourier(userName))
        return {ErrorCode::AssignCourierFailed, "该用户不是快递员"};

    const Express* e = getExpress(trackingNo);
    if (!e)
        return {ErrorCode::AssignCourierFailed, "快递不存在"};
    if (e->getCourier() != "")
        return {ErrorCode::AssignCourierFailed, "快递已被分配"};

    expresses.at(trackingNo).assignCourier(userName);
    noByCourier.emplace(userName, trackingNo);
    return ErrorCode::Success;
}

Result System::banUser(
    const std::string& actor,
    const std::string& userName
) {
    if (!isAdmin(actor))
        return {ErrorCode::BanFailed, "权限不足"};
    if (!getUser(userName))
        return {ErrorCode::BanFailed, "用户不存在"};
    if (isAdmin(userName))
        return {ErrorCode::BanFailed, "不可封禁管理员"};

    users.at(userName)->ban();
    return ErrorCode::Success;
}

Result System::unbanUser(
    const std::string& actor,
    const std::string& userName
) {
    if (!isAdmin(actor))
        return {ErrorCode::UnbanFailed, "权限不足"};
    if (!getUser(userName))
        return {ErrorCode::UnbanFailed, "用户不存在"};

    users.at(userName)->unban();
    return ErrorCode::Success;
}

std::vector<const BaseUser*> System::queryAllUsers(
    const std::string& actor
) const {
    if (!isAdmin(actor)) return {};

    std::vector<const BaseUser*> result;
    result.reserve(users.size());
    for (auto it = users.begin(); it != users.end(); it++) {
        result.push_back(it->second.get());
    }
    return result;
}

std::vector<const Express*> System::queryAllExpress(
    const std::string& actor
) const {
    if (!isAdmin(actor)) return {};

    std::vector<const Express*> result;
    result.reserve(expresses.size());
    for (auto it = expresses.begin(); it != expresses.end(); it++) {
        result.push_back(&it->second);
    }
    return result;
}

std::vector<const BaseUser*> System::queryAllCourier(
    const std::string& actor
) const {
    if (!isAdmin(actor)) return {};

    std::vector<const BaseUser*> result;
    for (auto it = users.begin(); it != users.end(); it++) {
        const BaseUser* u = it->second.get();
        if (u->isCourier()) {
            result.push_back(u);
        }
    }
    return result;
}

std::vector<const Express*> System::queryAllByUser(
    const std::string& actor,
    const std::string& userName
) const {
    if (!isAdmin(actor)) return {};

    auto sendRange = noBySender.equal_range(userName);
    auto receiveRange = noByReceiver.equal_range(userName);
    auto result = collectExpresses(sendRange.first, sendRange.second);
    auto received = collectExpresses(receiveRange.first, receiveRange.second);
    result.insert(result.end(), received.begin(), received.end());
    return result;
}

std::vector<const Express*> System::queryAllByTime(
    const std::string& actor,
    const time_t start, const time_t end
) const {
    if (!isAdmin(actor)) return {};

    auto lower = noBySendTime.lower_bound(start);
    auto upper = noBySendTime.upper_bound(end);
    return collectExpresses(lower, upper);
}

const Express* System::queryAllByTrackingNo(
    const std::string& actor,
    const unsigned long long trackingNo
) const {
    if (!isAdmin(actor)) return nullptr;

    return getExpress(trackingNo);
}

std::vector<const Express*> System::queryAllByCourier(
    const std::string& actor,
    const std::string& courier
) const {
    if (!isAdmin(actor)) return {};

    auto range = noByCourier.equal_range(courier);
    return collectExpresses(range.first, range.second);
}

bool System::saveToFile(const std::string& filePath) const {
    return fileio::saveAll(*this, filePath);
}

bool System::loadFromFile(const std::string& filePath) {
    return fileio::loadAll(*this, filePath);
}

void System::addAdmin() {
    static std::string userName = "admin";
    static std::string password = "admin123";
    static std::string name = "Admin";

    if (getUser(userName)) return;

    users.emplace(userName, std::make_unique<Admin>(userName, password, name));
}