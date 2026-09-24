#pragma once

#include "config.hpp"
#include "BaseObject.hpp"
#include "Courier.hpp"
#include <string>
#include <ctime>
#include <optional>     // for optional
#include <memory>       // for unique_ptr
#include <iosfwd>       // for istream/ostream

class Express;
namespace fileio {
    bool writeExpress(std::ostream& os, const Express& express);
    std::unique_ptr<Express> readExpress(std::istream& is);
}

enum class Status {
    Signed = 0,
    Unsigned = 1,
    PendingPickup = 2
};

class Express {
private:
    unsigned long long trackingNo;          // 快递单号
    Status status = Status::PendingPickup;  // 快递状态
    std::string sender;                     // 寄件用户 (用户名)
    std::string receiver;                   // 收件用户 (用户名)
    std::optional<time_t> sendTime;         // 寄送时间 (构造对象时，自动取当前时间)
    std::optional<time_t> receiveTime;      // 接收时间 (签收时，自动取当前时间)
    std::optional<std::string> courier;     // 快递员
    std::unique_ptr<BaseObject> object;     // 物品

    // 方便数据恢复
    Express(
        const unsigned long long _trackingNo,
        const Status _status,
        std::string _sender,
        std::string _receiver,
        std::optional<time_t> _sendTime,
        std::optional<time_t> _receiveTime,
        std::optional<std::string> courier,
        std::unique_ptr<BaseObject> _object
    );

public:
    Express(
        const unsigned long long _trackingNo,
        std::string _sender,
        std::string _receiver,
        std::unique_ptr<BaseObject> _object
    );
    Express() = delete;
    Express(const Express&) = delete;
    Express(Express&&) noexcept = default;
    Express& operator=(const Express&) = delete;
    Express& operator=(Express&&) noexcept = default;

    // getter
    unsigned long long getTrackingNo() const;
    Status getStatus() const;
    const std::string& getSender() const;
    const std::string& getReceiver() const;
    std::optional<time_t> getSendTime() const;
    std::optional<time_t> getReceiveTime() const;
    const std::string& getCourier() const;
    const BaseObject* getObject() const;
    long long getPrice() const;

    // update
    bool sign();
    bool pickup();
    void updateDescription(std::string newDescription);
    void assignCourier(std::string _courier);

    // query
    bool isReceived() const;
    bool isPickedUp() const;
    bool isPendingPickup() const;

    // 持久化
    friend bool fileio::writeExpress(std::ostream& os, const Express& express);
    friend std::unique_ptr<Express> fileio::readExpress(std::istream& is);

    // 网络传输: Protocol 需要从网络数据重建快递对象（恢复 sendTime 等私有字段）。
    // 若提供 public setter，会允许外部随意修改核心状态，破坏数据一致性。
    friend class Protocol;
};