#pragma once

#include "BaseObject.hpp"

class Book : public BaseObject {
private:
    static const long long unitPrice = 200;     // 单位为“分”
    long long copy;                             // 单位

    static const long long checkCopy(const long long _copy);

public:
    Book(std::string _description, const long long _copy);

    // getter
    long long getCopy() const;
    long long getPrice() const override;

    // 持久化
    friend std::unique_ptr<BaseObject> fileio::readObject(std::istream& is);
};