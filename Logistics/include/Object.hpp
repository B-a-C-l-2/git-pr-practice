#pragma once

#include "BaseObject.hpp"

class Object : public BaseObject {
private:
    static const long long unitPrice = 500;     // 单位为“分”
    long long weight;                           // 重量

    static const long long checkWeight(const long long _weight);

public:
    Object(std::string _description, const long long _weight);

    // getter
    long long getWeight() const;
    long long getPrice() const override;

    // 持久化
    friend std::unique_ptr<BaseObject> fileio::readObject(std::istream& is);
};