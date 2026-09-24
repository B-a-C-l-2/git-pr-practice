#include "Fragile.hpp"
#include <stdexcept>    // for std::invalid_argument

const long long Fragile::checkWeight(const long long _weight) {
    if (_weight < 0)
        throw std::invalid_argument("易碎品重量不能为负！");
    return _weight;
}

Fragile::Fragile(std::string _description, const long long _weight)
: BaseObject(Type::Fragile, std::move(_description))
, weight(checkWeight(_weight))
{}

long long Fragile::getWeight() const {
    return weight;
}

long long Fragile::getPrice() const {
    return unitPrice * weight;
}