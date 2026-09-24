#include "Object.hpp"
#include <stdexcept>    // for std::invalid_argument

const long long Object::checkWeight(const long long _weight) {
    if (_weight < 0)
        throw std::invalid_argument("普通快递重量不能为负！");
    return _weight;
}

Object::Object(std::string _description, const long long _weight)
: BaseObject(Type::Object, std::move(_description))
, weight(checkWeight(_weight))
{}

long long Object::getWeight() const {
    return weight;
}

long long Object::getPrice() const {
    return unitPrice * weight;
}