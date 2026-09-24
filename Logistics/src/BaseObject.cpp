#include "BaseObject.hpp"
#include <stdexcept>    // for std::invalid_argument

std::string BaseObject::checkDescription(std::string _description) {
    if (_description.length() > MAX_DESCRIPTION_LENGTH)
        throw std::invalid_argument(
            "物品描述长度不应超过" + std::to_string(MAX_DESCRIPTION_LENGTH) + "！"
        );
    return !_description.empty() ? _description : DEFAULT_DESCRIPTION;
}

BaseObject::BaseObject(Type _type, std::string _description)
: type(_type)
, description(checkDescription(std::move(_description)))
{}

const std::string& BaseObject::getDescription() const {
    return description;
}

void BaseObject::updateDescription(std::string newDescription) {
    description = checkDescription(std::move(newDescription));
}

bool BaseObject::isObject() const {
    return type == Type::Object;
}

bool BaseObject::isBook() const {
    return type == Type::Book;
}

bool BaseObject::isFragile() const {
    return type == Type::Fragile;
}