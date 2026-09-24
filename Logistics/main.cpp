#include "System.hpp"
#include "UI.hpp"
#include <iostream>
#include <windows.h>

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    System sys;
    if (sys.loadFromFile()) {
        std::cout << "数据加载成功。\n";
    } else {
        std::cout << "暂无历史数据，使用全新系统。\n";
        sys.addAdmin();
    }

    UI ui(sys);
    ui.run();

    if (sys.saveToFile()) {
        std::cout << "数据已自动保存。\n";
    } else {
        std::cout << "数据保存失败！\n";
    }

    return 0;
}