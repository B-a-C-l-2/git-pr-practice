#include "Book.hpp"
#include <stdexcept>    // for std::invalid_argument

const long long Book::checkCopy(const long long _copy) {
    if (_copy < 0)
        throw std::invalid_argument("图书册数不能为负！");
    return _copy;
}

Book::Book(std::string _description, const long long _copy)
: BaseObject(Type::Book, std::move(_description))
, copy(checkCopy(_copy))
{}

long long Book::getCopy() const {
    return copy;
}

long long Book::getPrice() const {
    return unitPrice * copy;
}