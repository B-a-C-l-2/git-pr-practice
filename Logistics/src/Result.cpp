#include "Result.hpp"

Result::Result(ErrorCode _code) : code(_code), message(defaultMessage(_code)) {}

Result::Result(ErrorCode _code, const std::string& _message)
: code(_code)
, message(defaultMessage(_code) + ":" + _message)   // "默认文案:附带信息"
{}

std::string Result::defaultMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success :
            return "成功";
        case ErrorCode::AddUserFailed :
            return "添加用户失败";
        case ErrorCode::AddCourierFailed :
            return "添加快递员失败";
        case ErrorCode::AddExpressFailed :
            return "添加快递失败";
        case ErrorCode::ChangePasswordFailed :
            return "修改密码失败";
        case ErrorCode::RechargeFailed :
            return "充值失败";
        case ErrorCode::DeductFailed :
            return "支付失败";
        case ErrorCode::UpdateDescriptionFailed :
            return "修改物品描述失败";
        case ErrorCode::SignExpressFailed :
            return "快递签收失败";
        case ErrorCode::LoginFailed :
            return "登录失败";
        case ErrorCode::PickupFailed :
            return "揽收失败";
        case ErrorCode::AssignCourierFailed :
            return "分配快递员失败";
        case ErrorCode::BanFailed :
            return "封禁失败";
        case ErrorCode::UnbanFailed :
            return "解封失败";
        default:
            break;
    }

    return "未知失败";
}

bool Result::ok() const {
    return code == ErrorCode::Success;
}
