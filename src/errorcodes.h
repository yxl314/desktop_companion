#ifndef ERRORCODES_H
#define ERRORCODES_H

#include <QString>
#include <QtGlobal>

/// 统一错误码：32 位，高 16 位类别、低 16 位序号，见方案设计 7.1。
enum class ErrorCode : quint32
{
    Success = 0x00000000,

    EmptyInput = 0x00010001,
    ModelNotConfigured = 0x00010002,
    ApiKeyMissing = 0x00010003,
    InvalidUrl = 0x00010004,
    InvalidPersona = 0x00010005,
    InvalidImage = 0x00010006,
    MemoryLimitReached = 0x00010007,
    IncompleteCharacterPack = 0x00010008,

    ConnectionFailed = 0x00020001,
    RequestTimeout = 0x00020002,
    AuthFailed = 0x00020003,
    ModelNotFound = 0x00020004,
    RateLimited = 0x00020005,
    ServerError = 0x00020006,
    StreamParseFailed = 0x00020007,
    EmptyResponse = 0x00020008,

    FileReadFailed = 0x00030001,
    FileWriteFailed = 0x00030002,
    DiskFull = 0x00030003,
    PermissionDenied = 0x00030004,
    DataCorrupted = 0x00030005,
    FileNameConflict = 0x00030006,

    Busy = 0x00040001,
    UserAborted = 0x00040002,
    InvalidState = 0x00040003,
    ConfirmationRequired = 0x00040004,

    Unknown = 0x00090001,
};

/// 统一错误对象：错误码 + 可读原因 + 原始输出。
struct AppError
{
    ErrorCode code = ErrorCode::Success;
    QString message;
    QString detail;

    bool ok() const { return code == ErrorCode::Success; }
    QString codeText() const;
    QString toString() const;
};

namespace errors
{
/// 十六进制可读形式，如 0x0002_0003。
QString codeText(ErrorCode code);

/// 该错误码的默认中文提示。
QString defaultMessage(ErrorCode code);

/// 构造错误；message 为空时取默认提示。
AppError make(ErrorCode code, const QString &detail = QString(), const QString &message = QString());

/// HTTP 状态码 → 错误码（401/403 鉴权、404 模型、429 限流、5xx 服务端）。
ErrorCode fromHttpStatus(int status);

/// 成功 / 失败判定，写入日志时使用。
AppError fromHttpStatus(int status, const QString &body);

/// 网络层错误（QNetworkReply::NetworkError 的整型值）→ 错误码。
ErrorCode fromNetworkError(int error);
} // namespace errors

#endif // ERRORCODES_H
