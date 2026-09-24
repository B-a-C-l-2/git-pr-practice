#pragma once

#include "config.hpp"
#include <string>
#include <memory>   // for unique_ptr

class BaseObject;
namespace fileio {
    bool writeObject(std::ostream& os, const BaseObject& object);
    std::unique_ptr<BaseObject> readObject(std::istream& is);
}

enum class Type {
    Object = 0,
    Book = 1,
    Fragile = 2
};

class BaseObject {
private:
    Type type = Type::Object;                       // 物品类型

    static std::string checkDescription(std::string _description);

protected:
    std::string description = DEFAULT_DESCRIPTION;  // 物品描述

public:
    explicit BaseObject(Type _type = Type::Object, std::string _description = DEFAULT_DESCRIPTION);
    BaseObject(const BaseObject&) = default;
    BaseObject(BaseObject&&) noexcept = default;
    BaseObject& operator=(const BaseObject&) = default;
    BaseObject& operator=(BaseObject&&) noexcept = default;
    virtual ~BaseObject() = default;

    // getter
    const std::string& getDescription() const;
    virtual long long getPrice() const = 0;

    // update
    void updateDescription(std::string newDescription);
    
    bool isObject() const;
    bool isBook() const;
    bool isFragile() const;

    // 持久化
    friend bool fileio::writeObject(std::ostream& os, const BaseObject& object);
    friend std::unique_ptr<BaseObject> fileio::readObject(std::istream& is);
};