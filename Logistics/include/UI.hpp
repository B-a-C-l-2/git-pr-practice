#pragma once

#include "Client.hpp"
#include "Session.hpp"
#include <ctime>
#include <string>

class UI {
private:
    Session session;

    // 辅助
    static std::string formatTime(time_t time);
    static bool consumedAll(std::istringstream& ss);
    static std::optional<time_t> parseTime(const std::string& s);
    static std::string statusToString(Status status);
    static void printSeparator();
    static void printExpress(const Express* express);
    static void printUser(const BaseUser* user);
    static void clearInputBuffer();

    // 菜单
    static void printGuestMenu();
    static void printUserMenu(const std::string& name);
    static void printAdminMenu(const std::string& name);
    static void printCourierMenu(const std::string& name);

    // 各状态处理，返回 false 表示用户要求退出
    bool handleGuest();
    void handleAdmin();
    void handleAdminChoice(int choice);
    void handleUser();
    void handleUserChoice(int choice);
    void handleCourier();
    void handleCourierChoice(int choice);

public:
    UI(Client& client);
    UI() = delete;
    UI(const UI&) = delete;
    UI(UI&&) = delete;
    UI& operator=(const UI&) = delete;
    UI& operator=(UI&&) = delete;

    void run();
};