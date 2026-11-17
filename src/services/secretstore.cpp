#include "services/secretstore.h"

#include <QByteArray>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

namespace secrets
{

bool available()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

QString protect(const QString &plain)
{
    if (plain.isEmpty())
        return {};
#ifdef Q_OS_WIN
    QByteArray utf8 = plain.toUtf8();
    DATA_BLOB input;
    input.pbData = reinterpret_cast<BYTE *>(utf8.data());
    input.cbData = static_cast<DWORD>(utf8.size());
    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"desktop_companion apiKey", nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &output))
        return {};
    const QByteArray encoded(reinterpret_cast<const char *>(output.pbData), int(output.cbData));
    LocalFree(output.pbData);
    return QString::fromLatin1(encoded.toBase64());
#else
    Q_UNUSED(plain)
    return {};
#endif
}

QString unprotect(const QString &encoded)
{
    if (encoded.isEmpty())
        return {};
#ifdef Q_OS_WIN
    const QByteArray raw = QByteArray::fromBase64(encoded.toLatin1());
    if (raw.isEmpty())
        return {};
    QByteArray buffer = raw;
    DATA_BLOB input;
    input.pbData = reinterpret_cast<BYTE *>(buffer.data());
    input.cbData = static_cast<DWORD>(buffer.size());
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &output))
        return {};
    const QString plain = QString::fromUtf8(reinterpret_cast<const char *>(output.pbData), int(output.cbData));
    LocalFree(output.pbData);
    return plain;
#else
    Q_UNUSED(encoded)
    return {};
#endif
}
} // namespace secrets
