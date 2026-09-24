#include "UI.hpp"
#include "User.hpp"
#include "Courier.hpp"
#include "Object.hpp"
#include "Book.hpp"
#include "Fragile.hpp"
#include <iostream>
#include <vector>
#include <limits>
#include <sstream>
#include <iomanip>
#include <optional>

std::string UI::formatTime(time_t t) {
    struct tm timeInfo;
    localtime_s(&timeInfo, &t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeInfo);
    return std::string(buf);
}

bool UI::consumedAll(std::istringstream& ss) {
    ss >> std::ws;
    return ss.eof();
}

std::optional<time_t> UI::parseTime(const std::string& s) {
    // 先尝试只输日期 YYYY-MM-DD，自动补 00:00:00
    {
        std::tm t = {};
        std::istringstream ss(s);
        ss >> std::get_time(&t, "%Y-%m-%d");
        if (!ss.fail() && consumedAll(ss)) {
            time_t result = mktime(&t);
            if (result != -1) {
                std::cout << "  (已自动补齐为 " << formatTime(result) << ")\n";
                return result;
            }
        }
    }
    // 再尝试完整格式 YYYY-MM-DD HH:MM:SS
    {
        std::tm t = {};
        std::istringstream ss(s);
        ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
        if (!ss.fail() && consumedAll(ss)) {
            time_t result = mktime(&t);
            if (result != -1) return result;
        }
    }
    return std::nullopt;
}

std::string UI::statusToString(Status s) {
    switch (s) {
        case Status::Signed:        return "已签收";
        case Status::Unsigned:      return "未签收";
        case Status::PendingPickup: return "待揽收";
    }
    return "未知";
}

void UI::printSeparator() {
    std::cout << "----------------------------------------\n";
}

void UI::printExpress(const Express* e) {
    if (!e) return;
    std::cout << "  单号: " << e->getTrackingNo()
              << " | 状态: " << statusToString(e->getStatus())
              << " | 寄件人: " << e->getSender()
              << " | 收件人: " << e->getReceiver() << "\n";
    std::cout << "    寄送时间: "
              << (e->getSendTime().has_value() ? formatTime(*e->getSendTime()) : "N/A")
              << " | 签收时间: "
              << (e->getReceiveTime().has_value() ? formatTime(*e->getReceiveTime()) : "N/A")
              << "\n";
    std::cout << "    快递员: " << (e->getCourier().empty() ? "未 分 配" : e->getCourier())
              << " | 物品描述: " << e->getObject()->getDescription()
              << " | 费用: " << e->getPrice() / 100.0 << "元\n";
}

void UI::printUser(const BaseUser* u) {
    if (!u) return;
    std::cout << "  用户名: " << u->getUserName()
              << " | 姓名: " << u->getName()
              << " | 余额: " << u->getBalance() / 100.0 << "元"
              << " | 权限: " << (u->isAdmin() ? "管理员" : u->isCourier() ? "快递员" : "普通用户")
              << " | 状态: " << (u->isBanned() ? "已封禁" : "正常") << "\n";
    if (const User* user = dynamic_cast<const User*>(u)) {
        std::cout << "    地址: " << user->getAddress()
                  << " | 电话: " << user->getPhone() << "\n";
    }
    else if (const Courier* courier = dynamic_cast<const Courier*>(u)) {
        std::cout << "    电话: " << courier->getPhone() << "\n";
    }
}

void UI::clearInputBuffer() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void UI::printGuestMenu() {
    printSeparator();
    std::cout << "【未登录】\n";
    std::cout << "  1. 注册\n";
    std::cout << "  2. 登录\n";
    std::cout << "  0. 退出程序\n";
    printSeparator();
    std::cout << "请选择: ";
}

void UI::printUserMenu(const std::string& name) {
    printSeparator();
    std::cout << "【普通用户 - " << name << "】\n";
    std::cout << "  1. 查询余额\n";
    std::cout << "  2. 充值\n";
    std::cout << "  3. 寄快递\n";
    std::cout << "  4. 签收快递\n";
    std::cout << "  5. 批量签收\n";
    std::cout << "  6. 查询我寄出的快递\n";
    std::cout << "  7. 查询我收到的快递\n";
    std::cout << "  8. 查询待收快递\n";
    std::cout << "  9. 按寄件人查询\n";
    std::cout << " 10. 按收件人查询\n";
    std::cout << " 11. 按时间范围查询\n";
    std::cout << " 12. 按单号查询\n";
    std::cout << " 13. 修改快递描述\n";
    std::cout << " 14. 修改密码\n";
    std::cout << "  0. 退出登录\n";
    printSeparator();
    std::cout << "请选择: ";
}

void UI::printCourierMenu(const std::string& name) {
    printSeparator();
    std::cout << "【快递员 - " << name << "】\n";
    std::cout << "  1. 揽收快递\n";
    std::cout << "  2. 批量揽收\n";
    std::cout << "  3. 查询未揽收快递\n";
    std::cout << "  4. 查询已揽收快递\n";
    std::cout << "  5. 按寄件人查询已揽收\n";
    std::cout << "  6. 按收件人查询已揽收\n";
    std::cout << "  7. 按时间范围查询已揽收\n";
    std::cout << "  8. 按状态查询已揽收\n";
    std::cout << "  9. 按单号查询已揽收\n";
    std::cout << " 10. 查询已投递快递\n";
    std::cout << " 11. 按寄件人查询已投递\n";
    std::cout << " 12. 按收件人查询已投递\n";
    std::cout << " 13. 按时间范围查询已投递\n";
    std::cout << " 14. 按状态查询已投递\n";
    std::cout << " 15. 按单号查询已投递\n";
    std::cout << " 16. 修改密码\n";
    std::cout << " 17. 切换为用户模式\n";
    std::cout << "  0. 退出登录\n";
    printSeparator();
    std::cout << "请选择: ";
}

void UI::printAdminMenu(const std::string& name) {
    printSeparator();
    std::cout << "【管理员 - " << name << "】\n";
    std::cout << "  1. 查询所有用户\n";
    std::cout << "  2. 查询所有快递\n";
    std::cout << "  3. 按用户查询快递\n";
    std::cout << "  4. 按时间范围查询快递\n";
    std::cout << "  5. 按单号查询快递\n";
    std::cout << "  6. 添加快递员\n";
    std::cout << "  7. 分配快递员\n";
    std::cout << "  8. 封禁用户\n";
    std::cout << "  9. 封禁快递员\n";
    std::cout << " 10. 解封用户\n";
    std::cout << " 11. 查询所有快递员\n";
    std::cout << " 12. 按快递员查询快递\n";
    std::cout << " 13. 修改密码\n";
    std::cout << " 14. 切换为用户模式\n";
    std::cout << "  0. 退出登录\n";
    printSeparator();
    std::cout << "请选择: ";
}

bool UI::handleGuest() {
    printGuestMenu();
    int choice;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        clearInputBuffer();
        return true;
    }

    if (choice == 1) {
        std::string userName, password, name, address, phone;
        std::cout << "用户名: ";
        std::cin >> userName;
        std::cout << "密码: ";
        std::cin >> password;
        std::cout << "姓名: ";
        std::cin >> name;
        clearInputBuffer();
        std::cout << "地址: ";
        std::getline(std::cin, address);
        std::cout << "电话: ";
        std::cin >> phone;
        Result res = session.registerUser(userName, password, name, address, phone);
        std::cout << (res.ok() ? "注册成功！" : res.message) << "\n";
    }
    else if (choice == 2) {
        std::string userName, password;
        std::cout << "用户名: ";
        std::cin >> userName;
        std::cout << "密码: ";
        std::cin >> password;
        Result res = session.login(userName, password);
        std::cout << (res.ok() ? "登录成功！" : res.message) << "\n";
    }
    else if (choice == 0) {
        return false;
    }
    return true;
}

void UI::handleAdmin() {
    printAdminMenu(session.getCurrentUserName());
    int choice;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        clearInputBuffer();
        return;
    }

    handleAdminChoice(choice);
}

void UI::handleAdminChoice(int choice) {
    if (choice == 1) {
        auto users = session.queryAllUsers();
        std::cout << "共有 " << users.size() << " 位用户:\n";
        for (const auto* u : users) printUser(u);
    }
    else if (choice == 2) {
        auto expresses = session.queryAllExpress();
        std::cout << "共有 " << expresses.size() << " 条快递记录:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 3) {
        std::string userName;
        std::cout << "用户名: ";
        std::cin >> userName;
        auto expresses = session.queryAllByUser(userName);
        std::cout << "该用户相关快递 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 4) {
        std::string startStr, endStr;
        std::cout << "开始时间 (格式 YYYY-MM-DD[HH:MM:SS]): ";
        clearInputBuffer();
        std::getline(std::cin, startStr);
        auto startOpt = parseTime(startStr);
        if (!startOpt) { std::cout << "时间格式错误！\n"; return; }
        std::cout << "结束时间 (格式 YYYY-MM-DD[HH:MM:SS]): ";
        std::getline(std::cin, endStr);
        auto endOpt = parseTime(endStr);
        if (!endOpt) { std::cout << "时间格式错误！\n"; return; }
        time_t start = *startOpt;
        time_t end = *endOpt;
        auto expresses = session.queryAllByTime(start, end);
        std::cout << "该时间段内快递 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 5) {
        unsigned long long trackingNo;
        std::cout << "快递单号: ";
        if (!(std::cin >> trackingNo)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "输入错误！\n";
            return;
        }
        const Express* e = session.queryAllByTrackingNo(trackingNo);
        if (e) printExpress(e);
        else std::cout << "快递不存在！\n";
    }
    else if (choice == 6) {
        std::string userName, password, name, phone;
        std::cout << "用户名: ";
        std::cin >> userName;
        std::cout << "密码: ";
        std::cin >> password;
        std::cout << "姓名: ";
        std::cin >> name;
        std::cout << "电话: ";
        std::cin >> phone;
        Result res = session.addCourier(userName, password, name, phone);
        std::cout << (res.ok() ? "添加快递员成功！" : res.message) << "\n";
    }
    else if (choice == 7) {
        std::string userName;
        unsigned long long trackingNo;
        std::cout << "快递员用户名: ";
        std::cin >> userName;
        std::cout << "快递单号: ";
        if (!(std::cin >> trackingNo)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "输入错误！\n";
            return;
        }
        Result res = session.assignCourier(userName, trackingNo);
        std::cout << (res.ok() ? "分配成功！" : res.message) << "\n";
    }
    else if (choice == 8) {
        std::string userName;
        std::cout << "要封禁的用户名: ";
        std::cin >> userName;
        Result res = session.banUser(userName);
        std::cout << (res.ok() ? "封禁成功！" : res.message) << "\n";
    }
    else if (choice == 9) {
        std::string userName;
        std::cout << "要封禁的快递员用户名: ";
        std::cin >> userName;
        Result res = session.banCourier(userName);
        std::cout << (res.ok() ? "封禁成功！" : res.message) << "\n";
    }
    else if (choice == 10) {
        std::string userName;
        std::cout << "要解封的用户名: ";
        std::cin >> userName;
        Result res = session.unbanUser(userName);
        std::cout << (res.ok() ? "解封成功！" : res.message) << "\n";
    }
    else if (choice == 11) {
        auto couriers = session.queryAllCourier();
        std::cout << "共有 " << couriers.size() << " 位快递员:\n";
        for (const auto* c : couriers) printUser(c);
    }
    else if (choice == 12) {
        std::string courierName;
        std::cout << "快递员用户名: ";
        std::cin >> courierName;
        auto expresses = session.queryAllByCourier(courierName);
        std::cout << "该快递员相关快递 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 13) {
        std::string newPassword;
        std::cout << "新密码: ";
        std::cin >> newPassword;
        Result res = session.changeMyPassword(newPassword);
        std::cout << (res.ok() ? "修改成功！" : res.message) << "\n";
    }
    else if (choice == 14) {
        while (true) {
            printUserMenu(session.getCurrentUserName());
            int userChoice;
            if (!(std::cin >> userChoice)) {
                std::cin.clear();
                clearInputBuffer();
                continue;
            }
            if (userChoice == 0) break;
            handleUserChoice(userChoice);
        }
    }
    else if (choice == 0) {
        session.logout();
        std::cout << "已退出登录。\n";
    }
}

void UI::handleUser() {
    printUserMenu(session.getCurrentUserName());
    int choice;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        clearInputBuffer();
        return;
    }
    handleUserChoice(choice);
}

void UI::handleUserChoice(int choice) {
    if (choice == 1) {
        auto balance = session.queryMyBalance();
        if (balance.has_value())
            std::cout << "当前余额: " << *balance / 100.0 << "元\n";
        else
            std::cout << "查询失败！\n";
    }
    else if (choice == 2) {
        long double money;
        std::cout << "充值金额 (单位: 元): ";
        if (!(std::cin >> money)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "输入错误！\n";
            return;
        }
        Result res = session.recharge(static_cast<long long>(money * 100));
        std::cout << (res.ok() ? "充值成功！" : res.message) << "\n";
    }
    else if (choice == 3) {
        std::string receiver, description;
        std::cout << "收件人用户名: ";
        std::cin >> receiver;
        clearInputBuffer();
        std::cout << "物品描述: ";
        std::getline(std::cin, description);

        std::cout << "请选择物品类型:\n";
        std::cout << "  1. 普通物品\n";
        std::cout << "  2. 书籍\n";
        std::cout << "  3. 易碎品\n";
        std::cout << "请选择: ";
        int typeChoice;
        if (!(std::cin >> typeChoice)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "输入错误！\n";
            return;
        }

        std::unique_ptr<BaseObject> object;
        try {
            if (typeChoice == 1) {
                long long weight;
                std::cout << "重量: ";
                if (!(std::cin >> weight)) {
                    std::cin.clear();
                    clearInputBuffer();
                    std::cout << "输入错误！\n";
                    return;
                }
                object = std::make_unique<Object>(description, weight);
            }
            else if (typeChoice == 2) {
                long long copy;
                std::cout << "数量: ";
                if (!(std::cin >> copy)) {
                    std::cin.clear();
                    clearInputBuffer();
                    std::cout << "输入错误！\n";
                    return;
                }
                object = std::make_unique<Book>(description, copy);
            }
            else if (typeChoice == 3) {
                long long weight;
                std::cout << "重量: ";
                if (!(std::cin >> weight)) {
                    std::cin.clear();
                    clearInputBuffer();
                    std::cout << "输入错误！\n";
                    return;
                }
                object = std::make_unique<Fragile>(description, weight);
            }
            else {
                std::cout << "无效选择！\n";
                return;
            }
        } catch (const std::invalid_argument& e) {
            std::cout << e.what() << "\n";
            return;
        }
        SendResult res = session.sendExpress(
            receiver,
            std::move(object)
        );
        if (res.result.ok())
            std::cout << "寄件成功！快递单号: " << res.trackingNo << "\n";
        else
            std::cout << res.result.message << "\n";
    }
    else if (choice == 4) {
        unsigned long long trackingNo;
        std::cout << "快递单号: ";
        if (!(std::cin >> trackingNo)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "输入错误！\n";
            return;
        }
        Result res = session.signExpress(trackingNo);
        std::cout << (res.ok() ? "签收成功！" : res.message) << "\n";
    }
    else if (choice == 5) {
        std::vector<unsigned long long> nos;
        std::cout << "输入快递单号 (用空格分隔，输入0结束): ";
        while (true) {
            unsigned long long no;
            if (!(std::cin >> no)) {
                std::cin.clear();
                clearInputBuffer();
                std::cout << "输入错误，已清空！\n";
                break;
            }
            if (no == 0) break;
            nos.push_back(no);
        }
        BatchResult res = session.signMany(nos);
        std::cout << "请求: " << res.totalRequested
                  << " | 成功: " << res.succeeded
                  << " | 失败: " << res.failures.size() << "\n";
        for (const auto& [no, result] : res.failures) {
            std::cout << "  单号 " << no << ": " << result.message << "\n";
        }
    }
    else if (choice == 6) {
        auto expresses = session.queryMySent();
        std::cout << "你寄出了 " << expresses.size() << " 条快递:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 7) {
        auto expresses = session.queryMyReceive();
        std::cout << "你收到了 " << expresses.size() << " 条快递:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 8) {
        auto expresses = session.queryMyPending();
        std::cout << "你有 " << expresses.size() << " 条待收快递:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 9) {
        std::string sender;
        std::cout << "寄件人用户名: ";
        std::cin >> sender;
        auto expresses = session.queryMyBySender(sender);
        std::cout << "查询到 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 10) {
        std::string receiver;
        std::cout << "收件人用户名: ";
        std::cin >> receiver;
        auto expresses = session.queryMyByReceiver(receiver);
        std::cout << "查询到 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 11) {
        std::string startStr, endStr;
        std::cout << "开始时间 (格式 YYYY-MM-DD[HH:MM:SS]): ";
        clearInputBuffer();
        std::getline(std::cin, startStr);
        auto startOpt = parseTime(startStr);
        if (!startOpt) { std::cout << "时间格式错误！\n"; return; }
        std::cout << "结束时间 (格式 YYYY-MM-DD[HH:MM:SS]): ";
        std::getline(std::cin, endStr);
        auto endOpt = parseTime(endStr);
        if (!endOpt) { std::cout << "时间格式错误！\n"; return; }
        time_t start = *startOpt;
        time_t end = *endOpt;
        auto expresses = session.queryMyByTime(start, end);
        std::cout << "查询到 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 12) {
        unsigned long long trackingNo;
        std::cout << "快递单号: ";
        if (!(std::cin >> trackingNo)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "输入错误！\n";
            return;
        }
        const Express* e = session.queryMyByTrackingNo(trackingNo);
        if (e) printExpress(e);
        else std::cout << "快递不存在或不属于你！\n";
    }
    else if (choice == 13) {
        unsigned long long trackingNo;
        std::string newDescription;
        std::cout << "快递单号: ";
        if (!(std::cin >> trackingNo)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "输入错误！\n";
            return;
        }
        clearInputBuffer();
        std::cout << "新描述: ";
        std::getline(std::cin, newDescription);
        Result res = session.updateDescription(trackingNo, newDescription);
        std::cout << (res.ok() ? "修改成功！" : res.message) << "\n";
    }
    else if (choice == 14) {
        std::string newPassword;
        std::cout << "新密码: ";
        std::cin >> newPassword;
        Result res = session.changeMyPassword(newPassword);
        std::cout << (res.ok() ? "修改成功！" : res.message) << "\n";
    }
    else if (choice == 0) {
        session.logout();
        std::cout << "已退出登录。\n";
    }
}

void UI::handleCourierChoice(int choice) {
    if (choice == 1) {
        unsigned long long trackingNo;
        std::cout << "快递单号: ";
        if (!(std::cin >> trackingNo)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "输入错误！\n";
            return;
        }
        Result res = session.pickup(trackingNo);
        std::cout << (res.ok() ? "揽收成功！" : res.message) << "\n";
    }
    else if (choice == 2) {
        std::vector<unsigned long long> nos;
        std::cout << "输入快递单号 (用空格分隔，输入0结束): ";
        while (true) {
            unsigned long long no;
            if (!(std::cin >> no)) {
                std::cin.clear();
                clearInputBuffer();
                std::cout << "输入错误，已清空！\n";
                break;
            }
            if (no == 0) break;
            nos.push_back(no);
        }
        BatchResult res = session.pickupMany(nos);
        std::cout << "请求: " << res.totalRequested
                  << " | 成功: " << res.succeeded
                  << " | 失败: " << res.failures.size() << "\n";
        for (const auto& [no, result] : res.failures) {
            std::cout << "  单号 " << no << ": " << result.message << "\n";
        }
    }
    else if (choice == 3) {
        auto expresses = session.queryPendingPickup();
        std::cout << "未揽收快递 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 4) {
        auto expresses = session.queryPickedUp();
        std::cout << "已揽收快递 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 5) {
        std::string sender;
        std::cout << "寄件人用户名: ";
        std::cin >> sender;
        auto expresses = session.queryPickedUpBySender(sender);
        std::cout << "查询到 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 6) {
        std::string receiver;
        std::cout << "收件人用户名: ";
        std::cin >> receiver;
        auto expresses = session.queryPickedUpByReceiver(receiver);
        std::cout << "查询到 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 7) {
        std::string startStr, endStr;
        std::cout << "开始时间 (格式 YYYY-MM-DD[HH:MM:SS]): ";
        clearInputBuffer();
        std::getline(std::cin, startStr);
        auto startOpt = parseTime(startStr);
        if (!startOpt) { std::cout << "时间格式错误！\n"; return; }
        std::cout << "结束时间 (格式 YYYY-MM-DD[HH:MM:SS]): ";
        std::getline(std::cin, endStr);
        auto endOpt = parseTime(endStr);
        if (!endOpt) { std::cout << "时间格式错误！\n"; return; }
        time_t start = *startOpt;
        time_t end = *endOpt;
        auto expresses = session.queryPickedUpByTime(start, end);
        std::cout << "查询到 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 8) {
        std::cout << "状态 (0=已签收, 1=未签收, 2=待揽收): ";
        int s;
        if (!(std::cin >> s)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "输入错误！\n";
            return;
        }
        if (s < 0 || s > 2) {
            std::cout << "无效状态！\n";
            return;
        }
        Status status = static_cast<Status>(s);
        auto expresses = session.queryPickedUpByStatus(status);
        std::cout << "查询到 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 9) {
        unsigned long long trackingNo;
        std::cout << "快递单号: ";
        if (!(std::cin >> trackingNo)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "输入错误！\n";
            return;
        }
        const Express* e = session.queryPickedUpByTrackingNo(trackingNo);
        if (e) printExpress(e);
        else std::cout << "快递不存在或不属于已揽收！\n";
    }
    else if (choice == 10) {
        auto expresses = session.queryDelivered();
        std::cout << "已投递快递 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 11) {
        std::string sender;
        std::cout << "寄件人用户名: ";
        std::cin >> sender;
        auto expresses = session.queryDeliveredBySender(sender);
        std::cout << "查询到 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 12) {
        std::string receiver;
        std::cout << "收件人用户名: ";
        std::cin >> receiver;
        auto expresses = session.queryDeliveredByReceiver(receiver);
        std::cout << "查询到 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 13) {
        std::string startStr, endStr;
        std::cout << "开始时间 (格式 YYYY-MM-DD[HH:MM:SS]): ";
        clearInputBuffer();
        std::getline(std::cin, startStr);
        auto startOpt = parseTime(startStr);
        if (!startOpt) { std::cout << "时间格式错误！\n"; return; }
        std::cout << "结束时间 (格式 YYYY-MM-DD[HH:MM:SS]): ";
        std::getline(std::cin, endStr);
        auto endOpt = parseTime(endStr);
        if (!endOpt) { std::cout << "时间格式错误！\n"; return; }
        time_t start = *startOpt;
        time_t end = *endOpt;
        auto expresses = session.queryDeliveredByTime(start, end);
        std::cout << "查询到 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 14) {
        std::cout << "状态 (0=已签收, 1=未签收, 2=待揽收): ";
        int s;
        if (!(std::cin >> s)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "输入错误！\n";
            return;
        }
        if (s < 0 || s > 2) {
            std::cout << "无效状态！\n";
            return;
        }
        Status status = static_cast<Status>(s);
        auto expresses = session.queryDeliveredByStatus(status);
        std::cout << "查询到 " << expresses.size() << " 条:\n";
        for (const auto* e : expresses) printExpress(e);
    }
    else if (choice == 15) {
        unsigned long long trackingNo;
        std::cout << "快递单号: ";
        if (!(std::cin >> trackingNo)) {
            std::cin.clear();
            clearInputBuffer();
            std::cout << "输入错误！\n";
            return;
        }
        const Express* e = session.queryDeliveredByTrackingNo(trackingNo);
        if (e) printExpress(e);
        else std::cout << "快递不存在或不属于已投递！\n";
    }
    else if (choice == 16) {
        std::string newPassword;
        std::cout << "新密码: ";
        std::cin >> newPassword;
        Result res = session.changeMyPassword(newPassword);
        std::cout << (res.ok() ? "修改成功！" : res.message) << "\n";
    }
    else if (choice == 17) {
        while (true) {
            printUserMenu(session.getCurrentUserName());
            int userChoice;
            if (!(std::cin >> userChoice)) {
                std::cin.clear();
                clearInputBuffer();
                continue;
            }
            if (userChoice == 0) break;
            handleUserChoice(userChoice);
        }
    }
    else if (choice == 0) {
        session.logout();
        std::cout << "已退出登录。\n";
    }
}

void UI::handleCourier() {
    printCourierMenu(session.getCurrentUserName());
    int choice;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        clearInputBuffer();
        return;
    }
    handleCourierChoice(choice);
}

UI::UI(Client& client) : session(client) {}

void UI::run() {
    while (true) {
        if (!session.isLoggedIn()) {
            if (!handleGuest()) break;
        }
        else if (session.currentIsAdmin()) {
            handleAdmin();
        }
        else if (session.currentIsCourier()) {
            handleCourier();
        }
        else {
            handleUser();
        }
    }
}