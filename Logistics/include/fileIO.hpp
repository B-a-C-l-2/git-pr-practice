#include "config.hpp"
#include "HashedPassword.hpp"
#include "BaseUser.hpp"
#include "User.hpp"
#include "Admin.hpp"
#include "BaseObject.hpp"
#include "Object.hpp"
#include "Book.hpp"
#include "Fragile.hpp"
#include "Express.hpp"
#include "System.hpp"
#include <optional>     // for optional
#include <iostream>
#include <fstream>

/*
在各类中需要使用友元函数
说明：fileIO需要完整读写各类（HashedPassword、BaseUser、User、Admin、
Object、Express、System）的所有私有数据成员以完成持久化。这些成员不允
许被外部随意修改，否则将破坏数据一致性；而若提供public setter，又会暴
露内部实现并降低封装。因此使用友元，仅允许fileIO在持久化场景下直接访问。
*/

namespace fileio {
    // 辅助
    bool writeString(std::ostream& os, const std::string& str);
    std::unique_ptr<std::string> readString(std::istream& is);
    bool writeTime(std::ostream& os, const std::optional<time_t>& t);
    std::optional<time_t> readTime(std::istream& is);
    bool writeCourier(std::ostream& os, const std::optional<std::string>& courier);
    std::optional<std::string> readCourier(std::istream& is);

    bool writePassword(std::ostream& os, const HashedPassword& password);
    std::unique_ptr<HashedPassword> readPassword(std::istream& is);
    bool writeUser(std::ostream& os, const BaseUser& user);
    std::unique_ptr<BaseUser> readUser(std::istream& is);
    bool writeObject(std::ostream& os, const BaseObject& object);
    std::unique_ptr<BaseObject> readObject(std::istream& is);
    bool writeExpress(std::ostream& os, const Express& express);
    std::unique_ptr<Express> readExpress(std::istream& is);

    // for System
    bool saveAll(const System& sys, const std::string& path);
    bool loadAll(System& sys, const std::string& path);
}