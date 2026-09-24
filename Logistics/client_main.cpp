#include "Client.hpp"
#include "UI.hpp"
#include <iostream>
#include <windows.h>

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    Client client("127.0.0.1", 8888);
    if (!client.isConnected()) {
        std::cout << "连接服务器失败！请确保服务器已启动。\n";
        return 1;
    }

    std::cout << "已连接到服务器。\n";
    UI ui(client);
    ui.run();

    return 0;
}
