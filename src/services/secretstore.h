/**
 * \file src/services/secretstore.h
 * \brief 敏感字符串保护：Windows DPAPI 加密，其它平台原样返回。
 */

#ifndef SECRETSTORE_H
#define SECRETSTORE_H

#include <QString>

/**
 * \brief 敏感字符串保护：Windows 用 DPAPI（当前用户），其它平台原样返回。
 */
namespace secrets
{
/**
 * \brief 加密敏感字符串，返回可直接写入 JSON 的形式。
 * @param plain 明文字符串。
 * @return Windows 下为 base64 密文；失败返回空。
 */
QString protect(const QString &plain);

/**
 * \brief 还原 protect() 的结果。
 * @param encoded protect() 得到的字符串。
 * @return 明文字符串；失败或平台不支持时返回空。
 */
QString unprotect(const QString &encoded);

/**
 * \brief 当前平台是否具备真正的加密能力。
 * @return 是否支持加密。
 */
bool available();
} // namespace secrets

#endif // SECRETSTORE_H
