#pragma once

#include "BaseObject.hpp"

class Fragile : public BaseObject {
private:
    static const long long unitPrice = 800;     // 单位为“分”
    long long weight;                           // 重量

    static const long long checkWeight(const long long _weight);

public:
    Fragile(std::string _description, const long long _weight);

    // getter
    long long getWeight() const;
    long long getPrice() const override;

    // 持久化
    friend std::unique_ptr<BaseObject> fileio::readObject(std::istream& is);
};