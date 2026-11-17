/**
 * \file src/services/errorcodes.h
 * \brief 统一错误码、错误对象与 HTTP / 网络错误映射。
 */

#ifndef ERRORCODES_H
#define ERRORCODES_H

#include <QString>
#include <QtGlobal>

/**
 * \brief 统一错误码：32 位，高 16 位类别、低 16 位序号，见方案设计 7.1。
 */
enum class ErrorCode : quint32
{
    Success = 0x00000000, ///< 成功。

    EmptyInput = 0x00010001,        ///< 输入为空。
    ModelNotConfigured = 0x00010002, ///< 模型未配置。
    ApiKeyMissing = 0x00010003,     ///< 缺少 API Key。
    InvalidUrl = 0x00010004,        ///< 接口地址非法。
    InvalidPersona = 0x00010005,    ///< 人设非法。
    InvalidImage = 0x00010006,      ///< 图片非法。
    MemoryLimitReached = 0x00010007, ///< 记忆条目已达上限。
    IncompleteCharacterPack = 0x00010008, ///< 角色包不完整。

    ConnectionFailed = 0x00020001, ///< 连接失败。
    RequestTimeout = 0x00020002,   ///< 请求超时。
    AuthFailed = 0x00020003,       ///< 鉴权失败。
    ModelNotFound = 0x00020004,    ///< 模型不存在。
    RateLimited = 0x00020005,      ///< 触发限流。
    ServerError = 0x00020006,      ///< 服务端错误。
    StreamParseFailed = 0x00020007, ///< 流式响应解析失败。
    EmptyResponse = 0x00020008,    ///< 模型返回空内容。

    FileReadFailed = 0x00030001,   ///< 文件读取失败。
    FileWriteFailed = 0x00030002,  ///< 文件写入失败。
    DiskFull = 0x00030003,         ///< 磁盘已满。
    PermissionDenied = 0x00030004, ///< 权限不足。
    DataCorrupted = 0x00030005,    ///< 数据损坏。
    FileNameConflict = 0x00030006, ///< 文件名冲突。

    Busy = 0x00040001,             ///< 忙。
    UserAborted = 0x00040002,      ///< 用户中止。
    InvalidState = 0x00040003,     ///< 状态非法。
    ConfirmationRequired = 0x00040004, ///< 需要用户确认。

    Unknown = 0x00090001,          ///< 未知错误。
};

/**
 * \brief 统一错误对象：错误码 + 可读原因 + 原始输出。
 */
struct AppError
{
    ErrorCode code = ErrorCode::Success; ///< 错误码。
    QString message;                     ///< 可读原因。
    QString detail;                      ///< 原始输出 / 细节。

    /// 是否成功（错误码为 Success）。
    bool ok() const { return code == ErrorCode::Success; }
    /// 错误码的十六进制可读形式。
    QString codeText() const;
    /// 错误对象的完整可读形式（错误码 + 原因 + 细节）。
    QString toString() const;
};

/**
 * \brief 错误码工具集。
 */
namespace errors
{
/**
 * \brief 十六进制可读形式，如 0x0002_0003。
 * @param code 错误码。
 * @return 可读错误码文本。
 */
QString codeText(ErrorCode code);

/**
 * \brief 该错误码的默认中文提示。
 * @param code 错误码。
 * @return 默认提示文本。
 */
QString defaultMessage(ErrorCode code);

/**
 * \brief 构造错误对象。
 * @param code 错误码。
 * @param detail 原始细节。
 * @param message 可读原因；为空时取默认提示。
 * @return 构造好的错误对象。
 */
AppError make(ErrorCode code, const QString &detail = QString(), const QString &message = QString());

/**
 * \brief HTTP 状态码 → 错误码。
 *
 * 401/403 鉴权、404 模型、429 限流、5xx 服务端。
 * @param status HTTP 状态码。
 * @return 对应错误码。
 */
ErrorCode fromHttpStatus(int status);

/**
 * \brief HTTP 状态码与响应体 → 错误对象，写入日志时使用。
 * @param status HTTP 状态码。
 * @param body 响应体。
 * @return 成功 / 失败判定的错误对象。
 */
AppError fromHttpStatus(int status, const QString &body);

/**
 * \brief 网络层错误 → 错误码。
 * @param error QNetworkReply::NetworkError 的整型值。
 * @return 对应错误码。
 */
ErrorCode fromNetworkError(int error);
} // namespace errors

#endif // ERRORCODES_H
