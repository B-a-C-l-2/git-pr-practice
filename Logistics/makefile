# 编译器
CXX = g++
# 编译参数
CXXFLAGS = -Wall -std=c++17 -Iinclude
# 链接 Winsock 库
LDFLAGS = -lws2_32
# 目录
OBJ_DIR = build
SRC_DIR = src
INC_DIR = include

# 共享业务实体源文件
COMMON_SRCS = $(SRC_DIR)/Admin.cpp \
              $(SRC_DIR)/BaseObject.cpp \
              $(SRC_DIR)/BaseUser.cpp \
              $(SRC_DIR)/Book.cpp \
              $(SRC_DIR)/Courier.cpp \
              $(SRC_DIR)/Express.cpp \
              $(SRC_DIR)/Fragile.cpp \
              $(SRC_DIR)/HashedPassword.cpp \
              $(SRC_DIR)/Object.cpp \
              $(SRC_DIR)/Result.cpp \
              $(SRC_DIR)/User.cpp \
              $(SRC_DIR)/Protocol.cpp

# 服务器源文件
SERVER_SRCS = server_main.cpp $(SRC_DIR)/System.cpp $(SRC_DIR)/fileIO.cpp $(SRC_DIR)/Server.cpp $(COMMON_SRCS)
SERVER_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(SERVER_SRCS)))
SERVER_TARGET = server.exe

# 客户端源文件
CLIENT_SRCS = client_main.cpp $(SRC_DIR)/Client.cpp $(SRC_DIR)/Session.cpp $(SRC_DIR)/UI.cpp $(COMMON_SRCS)
CLIENT_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(CLIENT_SRCS)))
CLIENT_TARGET = client.exe

# 默认编译两个目标
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# 服务器
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(SERVER_OBJS) -o $(SERVER_TARGET) $(LDFLAGS)

# 客户端
$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CLIENT_OBJS) -o $(CLIENT_TARGET) $(LDFLAGS)

# 编译根目录 cpp
$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 编译 src 下 cpp
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 创建 build 文件夹 (兼容 bash 与 cmd)
$(OBJ_DIR):
	-mkdir $(OBJ_DIR)

# 清理
clean:
	rm -rf $(OBJ_DIR) $(SERVER_TARGET) $(CLIENT_TARGET)

# 重新编译
rebuild: clean all
