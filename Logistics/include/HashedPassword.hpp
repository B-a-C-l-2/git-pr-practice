#pragma once

#include "config.hpp"
#include <string>
#include <memory>   // for unique_ptr
#include <iosfwd>   // for istream/ostream

class HashedPassword;
namespace fileio {
    bool writePassword(std::ostream& os, const HashedPassword& password);
    std::unique_ptr<HashedPassword> readPassword(std::istream& is);
}

class HashedPassword {
private:
    size_t hash;
    size_t length;

    // 方便数据恢复
    HashedPassword(const size_t _hash, const size_t _length);

public:
    explicit HashedPassword(const std::string& password);
    HashedPassword() = delete;
    HashedPassword(const HashedPassword&) = delete;
    HashedPassword(HashedPassword&& other) noexcept;
    HashedPassword& operator=(const HashedPassword&) = delete;
    HashedPassword& operator=(HashedPassword&& other) noexcept;
    ~HashedPassword() = default;

    // 密码验证
    bool verify(const std::string& input) const;

    // 密码修改
    void change(const std::string& newPassword);

    // 持久化
    friend bool fileio::writePassword(std::ostream& os, const HashedPassword& password);
    friend std::unique_ptr<HashedPassword> fileio::readPassword(std::istream& is);

    // 网络传输: Protocol 需要从网络数据重建密码哈希对象，
    // 若不设为友元，则必须提供 public 恢复构造函数或 setter，
    // 会暴露内部实现并降低封装。
    friend class Protocol;
};