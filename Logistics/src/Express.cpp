#include "Express.hpp"
#include <stdexcept>    // for std::invalid_argument

Express::Express(
    const unsigned long long _trackingNo,
    const Status _status,
    std::string _sender,
    std::string _receiver,
    std::optional<time_t> _sendTime,
    std::optional<time_t> _receiveTime,
    std::optional<std::string> _courier,
    std::unique_ptr<BaseObject> _object
)
: trackingNo(_trackingNo)
, status(_status)
, sender(std::move(_sender))
, receiver(std::move(_receiver))
, sendTime(std::move(_sendTime))
, receiveTime(std::move(_receiveTime))
, courier(std::move(_courier))
, object(std::move(_object))
{
    if (sender == receiver)
        throw std::invalid_argument("寄件人和收件人不能是同一人！");
}

Express::Express(
    const unsigned long long _trackingNo,
    std::string _sender,
    std::string _receiver,
    std::unique_ptr<BaseObject> _object
)
: trackingNo(_trackingNo)
, status(Status::PendingPickup)
, sender(std::move(_sender))
, receiver(std::move(_receiver))
, sendTime(time(nullptr))
, receiveTime(std::nullopt)
, courier(std::nullopt)
, object(std::move(_object))
{
    if (sender == receiver)
        throw std::invalid_argument("寄件人和收件人不能是同一人！");
}

unsigned long long Express::getTrackingNo() const {
    return trackingNo;
}

Status Express::getStatus() const {
    return status;
}

const std::string& Express::getSender() const {
    return sender;
}

const std::string& Express::getReceiver() const {
    return receiver;
}

std::optional<time_t> Express::getSendTime() const {
    return sendTime;
}

std::optional<time_t> Express::getReceiveTime() const {
    return receiveTime;
}

const std::string& Express::getCourier() const {
    static std::string empty;
    if (courier == std::nullopt) return empty;
    return *courier;
}

const BaseObject* Express::getObject() const {
    return object.get();
}

long long Express::getPrice() const {
    return object.get()->getPrice();
}

bool Express::sign() {
    if (!isPickedUp()) return false;        // 非“揽收”状态则拒绝
    status = Status::Signed;
    receiveTime = time(nullptr);
    return true;
}

bool Express::pickup() {
    if (!isPendingPickup()) return false;   // 不是“待揽收”则拒绝
    status = Status::Unsigned;
    return true;
}

void Express::updateDescription(std::string newDescription) {
    object->updateDescription(std::move(newDescription));
}

void Express::assignCourier(std::string _courier) {
    courier = std::move(_courier);
}

bool Express::isReceived() const {
    return status == Status::Signed;
}

bool Express::isPickedUp() const {
    return status == Status::Unsigned;
}

bool Express::isPendingPickup() const {
    return status == Status::PendingPickup;
}