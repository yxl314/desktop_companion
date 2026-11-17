#ifndef SECRETSTORE_H
#define SECRETSTORE_H

#include <QString>

/// 敏感字符串保护：Windows 用 DPAPI（当前用户），其它平台原样返回。
namespace secrets
{
/// 返回可直接写入 JSON 的字符串（Windows 下为 base64 密文）；失败返回空。
QString protect(const QString &plain);

/// 还原 protect() 的结果；失败或平台不支持时返回空。
QString unprotect(const QString &encoded);

/// 当前平台是否具备真正的加密能力。
bool available();
} // namespace secrets

#endif // SECRETSTORE_H
