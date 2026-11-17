#include "errorcodes.h"

namespace errors
{
namespace
{
QString hex16(quint32 value)
{
    return QStringLiteral("%1").arg(value, 4, 16, QLatin1Char('0')).toUpper();
}
} // namespace

QString codeText(ErrorCode code)
{
    const quint32 raw = static_cast<quint32>(code);
    return QStringLiteral("0x%1_%2").arg(hex16(raw >> 16), hex16(raw & 0xFFFF));
}

QString defaultMessage(ErrorCode code)
{
    switch (code) {
    case ErrorCode::Success:
        return QStringLiteral("操作成功");
    case ErrorCode::EmptyInput:
        return QStringLiteral("输入不能为空");
    case ErrorCode::ModelNotConfigured:
        return QStringLiteral("尚未配置模型，请先在设置中填写 Base URL 与模型名");
    case ErrorCode::ApiKeyMissing:
        return QStringLiteral("该服务需要 API Key，请先在设置中填写");
    case ErrorCode::InvalidUrl:
        return QStringLiteral("Base URL 不是合法地址");
    case ErrorCode::InvalidPersona:
        return QStringLiteral("人设文件无效或不可读");
    case ErrorCode::InvalidImage:
        return QStringLiteral("图片无法解码或格式不支持");
    case ErrorCode::MemoryLimitReached:
        return QStringLiteral("记忆条目已达上限，请先清理");
    case ErrorCode::IncompleteCharacterPack:
        return QStringLiteral("角色包不完整，缺少人设或形象");
    case ErrorCode::ConnectionFailed:
        return QStringLiteral("无法连接模型服务，请检查网络与服务地址");
    case ErrorCode::RequestTimeout:
        return QStringLiteral("请求超时，请稍后重试");
    case ErrorCode::AuthFailed:
        return QStringLiteral("鉴权失败，请检查 API Key 或权限");
    case ErrorCode::ModelNotFound:
        return QStringLiteral("模型不存在，请检查模型名");
    case ErrorCode::RateLimited:
        return QStringLiteral("请求过于频繁或额度不足，请稍后重试");
    case ErrorCode::ServerError:
        return QStringLiteral("模型服务异常，请稍后重试");
    case ErrorCode::StreamParseFailed:
        return QStringLiteral("流式响应解析失败，回复可能中断");
    case ErrorCode::EmptyResponse:
        return QStringLiteral("模型返回了空内容");
    case ErrorCode::FileReadFailed:
        return QStringLiteral("读取文件失败，请检查路径与权限");
    case ErrorCode::FileWriteFailed:
        return QStringLiteral("写入文件失败，请检查磁盘与权限");
    case ErrorCode::DiskFull:
        return QStringLiteral("磁盘空间不足");
    case ErrorCode::PermissionDenied:
        return QStringLiteral("没有访问权限");
    case ErrorCode::DataCorrupted:
        return QStringLiteral("数据文件损坏，已尝试回退或备份");
    case ErrorCode::FileNameConflict:
        return QStringLiteral("名称与现有文件冲突，请换一个名字");
    case ErrorCode::Busy:
        return QStringLiteral("当前有任务进行中，请稍后再试");
    case ErrorCode::UserAborted:
        return QStringLiteral("已中止");
    case ErrorCode::InvalidState:
        return QStringLiteral("当前状态不允许该操作");
    case ErrorCode::ConfirmationRequired:
        return QStringLiteral("该操作不可撤销，需要二次确认");
    case ErrorCode::Unknown:
        break;
    }
    return QStringLiteral("未知错误，请查看日志");
}

AppError make(ErrorCode code, const QString &detail, const QString &message)
{
    AppError error;
    error.code = code;
    error.message = message.isEmpty() ? defaultMessage(code) : message;
    error.detail = detail;
    return error;
}

ErrorCode fromHttpStatus(int status)
{
    if (status == 401 || status == 403)
        return ErrorCode::AuthFailed;
    if (status == 404)
        return ErrorCode::ModelNotFound;
    if (status == 429)
        return ErrorCode::RateLimited;
    if (status >= 500)
        return ErrorCode::ServerError;
    if (status >= 400)
        return ErrorCode::Unknown;
    return ErrorCode::Success;
}

AppError fromHttpStatus(int status, const QString &body)
{
    return make(fromHttpStatus(status), body.left(500));
}

ErrorCode fromNetworkError(int error)
{
    // QNetworkReply::NetworkError：Timeout=4，其余按连接失败处理。
    if (error == 4)
        return ErrorCode::RequestTimeout;
    if (error == 1 || error == 2 || error == 3 || error == 99 || error == 5)
        return ErrorCode::ConnectionFailed;
    return ErrorCode::ConnectionFailed;
}
} // namespace errors

QString AppError::codeText() const
{
    return errors::codeText(code);
}

QString AppError::toString() const
{
    QString text = codeText() + QStringLiteral(" ") + message;
    if (!detail.isEmpty())
        text += QStringLiteral("（") + detail.left(200) + QStringLiteral("）");
    return text;
}
