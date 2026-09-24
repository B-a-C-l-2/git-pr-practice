#include "fileIO.hpp"
#include <filesystem>
#include <sstream>

namespace fileio {
    // ---- 辅助 ----
    bool writeString(std::ostream& os, const std::string& str) {
        os << str.size() << " ";
        os.write(str.data(), static_cast<std::streamsize>(str.size()));
        os << " ";
        return os.good();
    }

    std::unique_ptr<std::string> readString(std::istream& is) {
        size_t size;
        is >> size;
        is.ignore(1);

        std::string str;
        str.resize(size);
        is.read(str.data(), static_cast<std::streamsize>(size));
        is.ignore(1);

        if (!is.good()) return nullptr;

        return std::make_unique<std::string>(std::move(str));
    }

    bool writeTime(std::ostream& os, const std::optional<time_t>& t) {
        if (t != std::nullopt) {
            os << true << " ";
            os << *t << " ";
        } else {
            os << false << " ";
        }
        return os.good();
    }

    std::optional<time_t> readTime(std::istream& is) {
        bool hasTime;
        is >> hasTime;

        if (!hasTime) return std::nullopt;

        time_t t;
        is >> t;

        if (!is.good()) return std::nullopt;

        return t;
    }

    bool writeCourier(std::ostream& os, const std::optional<std::string>& courier) {
        if (courier != std::nullopt) {
            os << true << " ";
            os << *courier << " ";
        } else {
            os << false << " ";
        }
        return os.good();
    }

    std::optional<std::string> readCourier(std::istream& is) {
        bool hasCourier;
        is >> hasCourier;

        if (!hasCourier) return std::nullopt;

        std::string courier;
        is >> courier;

        if (!is.good()) return std::nullopt;

        return courier;
    }

    // ---- HashedPassword ----
    bool writePassword(std::ostream& os, const HashedPassword& password) {
        os << password.hash << " ";
        os << password.length << " ";
        return os.good();
    }

    std::unique_ptr<HashedPassword> readPassword(std::istream& is) {
        size_t hash;
        size_t length;

        is >> hash >> length;

        if (!is.good()) return nullptr;

        return std::unique_ptr<HashedPassword>(
            new HashedPassword(hash, length)
        );
    }

    // ---- BaseUser ----
    bool writeUser(std::ostream& os, const BaseUser& user) {
        os << static_cast<int>(user.permission) << " ";
        os << user.banned << " ";
        if (!writeString(os, user.userName)) return false;
        if (!writeString(os, user.name)) return false;
        os << user.balance << " ";
        if (!writePassword(os, user.password)) return false;
        if (user.isAdmin()) {}
        if (user.isUser()) {
            const User* u = dynamic_cast<const User*>(&user);
            if (!u) return false;
            if (!writeString(os, u->getAddress())) return false;
            if (!writeString(os, u->getPhone())) return false;
        }
        if (user.isCourier()) {
            const Courier* u = dynamic_cast<const Courier*>(&user);
            if (!u) return false;
            if (!writeString(os, u->getPhone())) return false;
        }
        return os.good();
    }

    std::unique_ptr<BaseUser> readUser(std::istream& is) {
        int permission;
        is >> permission;

        bool banned;
        is >> banned;

        std::unique_ptr<std::string> userName = readString(is);
        if (!userName) return nullptr;

        std::unique_ptr<std::string> name = readString(is);
        if (!name) return nullptr;

        long long balance;
        is >> balance;

        std::unique_ptr<HashedPassword> password = readPassword(is);
        if (!password) return nullptr;

        if (!is.good()) return nullptr;

        if (permission == static_cast<int>(Permission::Admin)) {
            return std::unique_ptr<BaseUser>(
                new Admin(
                    *userName,
                    std::move(*password),
                    banned,
                    *name,
                    balance
                )
            );
        }

        if (permission == static_cast<int>(Permission::User)) {
            std::unique_ptr<std::string> address = readString(is);
            if (!address) return nullptr;

            std::unique_ptr<std::string> phone = readString(is);
            if (!phone) return nullptr;

            if (!is.good()) return nullptr;

            return std::unique_ptr<User>(
                new User(
                    *userName,
                    std::move(*password),
                    banned,
                    *name,
                    balance,
                    *address,
                    *phone
                )
            );
        }

        if (permission == static_cast<int>(Permission::Courier)) {
            std::unique_ptr<std::string> phone = readString(is);
            if (!phone) return nullptr;

            if (!is.good()) return nullptr;

            return std::unique_ptr<Courier>(
                new Courier(
                    *userName,
                    std::move(*password),
                    banned,
                    *name,
                    balance,
                    *phone
                )
            );
        }

        return nullptr;
    }

    // ---- Object ----
    bool writeObject(std::ostream& os, const BaseObject& object) {
        os << static_cast<int>(object.type) << " ";
        if (!writeString(os, object.description)) return false;
        if (object.isObject()) {
            const Object* o = dynamic_cast<const Object*>(&object);
            os << o->getWeight() << " ";
        }
        if (object.isBook()) {
            const Book* o = dynamic_cast<const Book*>(&object);
            os << o->getCopy() << " ";
        }
        if (object.isFragile()) {
            const Fragile* o = dynamic_cast<const Fragile*>(&object);
            os << o->getWeight() << " ";
        }
        return os.good();
    }

    std::unique_ptr<BaseObject> readObject(std::istream& is) {
        int type;
        is >> type;

        std::unique_ptr<std::string> description = readString(is);
        if (!description) return nullptr;

        if (!is.good()) return nullptr;

        if (type == static_cast<int>(Type::Object)) {            
            long long weight;
            is >> weight;

            return std::unique_ptr<BaseObject>(
                new Object(
                    *description,
                    weight
                )
            );
        }
        if (type == static_cast<int>(Type::Book)) {
            long long copy;
            is >> copy;

            return std::unique_ptr<BaseObject>(
                new Book(
                    *description,
                    copy
                )
            );
        }
        if (type == static_cast<int>(Type::Fragile)) {
            long long weight;
            is >> weight;

            return std::unique_ptr<BaseObject>(
                new Fragile(
                    *description,
                    weight
                )
            );
        }

        return nullptr;
    }

    // ---- Express ----
    bool writeExpress(std::ostream& os, const Express& express) {
        os << express.trackingNo << " ";
        os << static_cast<int>(express.status) << " ";
        if (!writeString(os, express.sender)) return false;
        if (!writeString(os, express.receiver)) return false;
        if (!writeTime(os, express.sendTime)) return false;
        if (!writeTime(os, express.receiveTime)) return false;
        if (!writeCourier(os, express.courier)) return false;
        if (!writeObject(os, *express.object)) return false;
        return os.good();
    }

    std::unique_ptr<Express> readExpress(std::istream& is) {
        unsigned long long trackingNo;
        is >> trackingNo;

        int status;
        is >> status;

        std::unique_ptr<std::string> sender = readString(is);
        if (!sender) return nullptr;

        std::unique_ptr<std::string> receiver = readString(is);
        if (!receiver) return nullptr;

        std::optional<time_t> sendTime = readTime(is);
        std::optional<time_t> receiveTime = readTime(is);

        std::optional<std::string> courier = readCourier(is);

        std::unique_ptr<BaseObject> object = readObject(is);
        if (!object) return nullptr;

        if (!is.good()) return nullptr;

        return std::unique_ptr<Express>(
            new Express(
                trackingNo,
                static_cast<Status>(status),
                *sender,
                *receiver,
                std::move(sendTime),
                std::move(receiveTime),
                std::move(courier),
                std::move(object)
            )
        );
    }

    // ---- System ----
    bool saveAll(const System& sys, const std::string& path) {
        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::error_code code;
            std::filesystem::create_directories(p.parent_path(), code);
            if (code) return false;
        }

        std::ofstream out(p, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!out) return false;

        out << "# Phase2 v1\n\n";

        out << "META\n"
            << "nextTrackingNo " << sys.nextTrackingNo << "\n"
            << "companyBalance " << sys.companyBalance << "\n"
            << "END\n\n";

        out << "USERS " << sys.users.size() << "\n";
        for (const auto& kv : sys.users) {
            if (!writeUser(out, *kv.second)) return false;
            out << "\n";
        }
        out << "END\n\n";

        out << "EXPRESSES " << sys.expresses.size() << "\n";
        for (const auto& kv : sys.expresses) {
            if (!writeExpress(out, kv.second)) return false;
            out << "\n";
        }
        out << "END\n\n";

        out.flush();
        return out.good();
    }

    bool loadAll(System& sys, const std::string& path) {
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in) return false;

        std::string line;
        std::getline(in, line);
        if (line != "# Phase2 v1") return false;

        System temp;

        std::string marker;
        in >> marker;
        if (marker != "META") return false;

        in >> marker >> temp.nextTrackingNo;
        if (marker != "nextTrackingNo" || !in) return false;

        in >> marker >> temp.companyBalance;
        if (marker != "companyBalance" || !in) return false;

        in >> marker;
        if (marker != "END") return false;

        in >> marker;
        if (marker != "USERS") return false;

        size_t userCount;
        in >> userCount;
        if (!in) return false;

        for (size_t i = 0; i < userCount; ++i) {
            auto u = readUser(in);
            if (!u) return false;

            auto [_, inserted] = temp.users.emplace(u->getUserName(), std::move(u));
            if (!inserted) return false;  // 重复用户名
        }

        in >> marker;
        if (marker != "END") return false;

        in >> marker;
        if (marker != "EXPRESSES") return false;

        size_t expressCount;
        in >> expressCount;
        if (!in) return false;

        unsigned long long maxTrackingNo = 0;
        for (size_t i = 0; i < expressCount; ++i) {
            auto e = readExpress(in);
            if (!e) return false;

            auto trackingNo = e->getTrackingNo();
            auto sender = e->getSender();
            auto receiver = e->getReceiver();
            auto sendTime = e->getSendTime();
            auto courier = e->getCourier();

            auto [_, inserted] = temp.expresses.emplace(trackingNo, std::move(*e));
            if (!inserted) return false;  // 重复单号

            temp.noBySender.emplace(sender, trackingNo);
            temp.noByReceiver.emplace(receiver, trackingNo);
            if (sendTime.has_value()) {
                temp.noBySendTime.emplace(*sendTime, trackingNo);
            }
            if (courier != "") {
                temp.noByCourier.emplace(courier, trackingNo);
            }

            if (trackingNo > maxTrackingNo) {
                maxTrackingNo = trackingNo;
            }
        }

        in >> marker;
        if (marker != "END" || !in.good()) return false;

        // 修正 nextTrackingNo，防止文件被篡改导致单号冲突
        if (temp.nextTrackingNo <= maxTrackingNo) {
            temp.nextTrackingNo = maxTrackingNo + 1;
        }

        sys = std::move(temp);
        return true;
    }
}