#include "HashedPassword.hpp"
#include <stdexcept>    // for std::invalid_argument

HashedPassword::HashedPassword(const size_t _hash, const size_t _length)
    : hash(_hash), length(_length) {}

HashedPassword::HashedPassword(const std::string& password) {
    if (password.length() < MIN_PASSWORD_LENGTH || password.length() > MAX_PASSWORD_LENGTH)
        throw std::invalid_argument("密码长度应为" + std::to_string(MIN_PASSWORD_LENGTH) + "~" + std::to_string(MAX_PASSWORD_LENGTH) + "！");

    hash = std::hash<std::string>()(password);
    length = password.length();
}

HashedPassword::HashedPassword(HashedPassword&& other) noexcept
    : hash(other.hash), length(other.length) {
    other.hash = 0;
    other.length = 0;
}

HashedPassword& HashedPassword::operator=(HashedPassword&& other) noexcept {
    if (this != &other) {
        hash = other.hash;
        length = other.length;
        other.hash = 0;
        other.length = 0;
    }
    return *this;
}

bool HashedPassword::verify(const std::string& input) const {
    size_t inputHash = std::hash<std::string>()(input);
    size_t inputLength = input.length();
    return hash == inputHash && length == inputLength;
}

void HashedPassword::change(const std::string& newPassword) {
    *this = HashedPassword(newPassword);
}