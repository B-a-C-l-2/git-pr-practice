#pragma once

#include <cstddef>  // for size_t
#include <string>

constexpr long long DEFAULT_PRICE = 1500;   // 单位为“分”

constexpr size_t MIN_PASSWORD_LENGTH = 6;
constexpr size_t MAX_PASSWORD_LENGTH = 20;

inline const std::string DEFAULT_DESCRIPTION = "该物品暂无描述。";
constexpr size_t MAX_DESCRIPTION_LENGTH = 1000;

inline const std::string DEFAULT_PATH = "data/system.txt";