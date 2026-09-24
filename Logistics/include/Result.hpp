#pragma once

#include <string>
#include <vector>
#include <utility>

enum class ErrorCode {
    Success = 0,
    AddUserFailed,
    AddCourierFailed,
    AddExpressFailed,
    ChangePasswordFailed,
    RechargeFailed,
    DeductFailed,
    UpdateDescriptionFailed,
    SignExpressFailed,
    LoginFailed,
    PickupFailed,
    AssignCourierFailed,
    BanFailed,
    UnbanFailed,
};

struct Result {
    ErrorCode code;
    std::string message;

    Result(ErrorCode _code);
    Result(ErrorCode _code, const std::string& _message);

    static std::string defaultMessage(ErrorCode code);
    bool ok() const;
};

struct SendResult {
    Result result;
    unsigned long long trackingNo = 0;
};

struct BatchResult {
    size_t totalRequested = 0;
    size_t succeeded = 0;
    std::vector<std::pair<unsigned long long, Result>> failures;
};
