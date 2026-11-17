#include "services/characterpack.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFuture>
#include <QPromise>
#include <QRunnable>
#include <QStringList>
#include <QThreadPool>
#include <QVector>

#include <array>
#include <functional>
#include <memory>
#include <utility>

namespace
{
constexpr quint32 localHeaderSignature = 0x04034b50;
constexpr quint32 centralHeaderSignature = 0x02014b50;
constexpr quint32 endOfCentralSignature = 0x06054b50;
constexpr quint16 zipMethodStored = 0;
constexpr quint16 utf8Flag = 0x0800;

quint32 crc32(const QByteArray &data)
{
    // 局部静态初始化线程安全：异步导出可能并发进入这里。
    static const std::array<quint32, 256> table = [] {
        std::array<quint32, 256> values{};
        for (quint32 i = 0; i < 256; ++i) {
            quint32 value = i;
            for (int bit = 0; bit < 8; ++bit)
                value = (value & 1U) ? (0xEDB88320U ^ (value >> 1)) : (value >> 1);
            values[i] = value;
        }
        return values;
    }();
    quint32 crc = 0xFFFFFFFFU;
    for (const char ch : data)
        crc = table[(crc ^ static_cast<quint8>(ch)) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFU;
}

void appendU16(QByteArray *out, quint16 value)
{
    out->append(static_cast<char>(value & 0xFF));
    out->append(static_cast<char>((value >> 8) & 0xFF));
}

void appendU32(QByteArray *out, quint32 value)
{
    out->append(static_cast<char>(value & 0xFF));
    out->append(static_cast<char>((value >> 8) & 0xFF));
    out->append(static_cast<char>((value >> 16) & 0xFF));
    out->append(static_cast<char>((value >> 24) & 0xFF));
}

quint16 readU16(const QByteArray &data, int offset)
{
    return static_cast<quint16>(static_cast<quint8>(data.at(offset))
                                | (static_cast<quint16>(static_cast<quint8>(data.at(offset + 1))) << 8));
}

quint32 readU32(const QByteArray &data, int offset)
{
    return static_cast<quint32>(static_cast<quint8>(data.at(offset)))
           | (static_cast<quint32>(static_cast<quint8>(data.at(offset + 1))) << 8)
           | (static_cast<quint32>(static_cast<quint8>(data.at(offset + 2))) << 16)
           | (static_cast<quint32>(static_cast<quint8>(data.at(offset + 3))) << 24);
}

struct CentralEntry
{
    QString name;
    quint16 method = 0;
    quint32 crc = 0;
    quint32 compressedSize = 0;
    quint32 uncompressedSize = 0;
    quint32 localOffset = 0;
};

bool isSafeRelativePath(const QString &path)
{
    if (path.isEmpty() || QDir::isAbsolutePath(path))
        return false;
    const QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        if (part == QLatin1String("..") || part == QLatin1String("."))
            return false;
    }
    return true;
}

void collectFiles(const QString &baseDir, const QString &relative, QStringList *files)
{
    const QDir dir(baseDir);
    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        const QString childRelative = relative.isEmpty() ? entry.fileName()
                                                        : relative + QLatin1Char('/') + entry.fileName();
        if (entry.isDir())
            collectFiles(entry.absoluteFilePath(), childRelative, files);
        else
            files->append(childRelative);
    }
}

bool readZipEntries(const QString &zipPath, QVector<CentralEntry> *entries, QString *error)
{
    QFile file(zipPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("无法读取角色包：%1").arg(file.errorString());
        return false;
    }
    const QByteArray data = file.readAll();
    if (data.size() < 22 || data.size() > 256 * 1024 * 1024) {
        if (error)
            *error = QStringLiteral("角色包为空或过大");
        return false;
    }

    int eocdOffset = -1;
    for (int offset = data.size() - 22; offset >= 0 && offset >= data.size() - 22 - 65535; --offset) {
        if (readU32(data, offset) == endOfCentralSignature) {
            eocdOffset = offset;
            break;
        }
    }
    if (eocdOffset < 0) {
        if (error)
            *error = QStringLiteral("不是有效的 zip 角色包");
        return false;
    }

    const quint16 total = readU16(data, eocdOffset + 10);
    const quint32 centralOffset = readU32(data, eocdOffset + 16);
    int cursor = int(centralOffset);
    for (quint16 index = 0; index < total; ++index) {
        if (cursor + 46 > data.size() || readU32(data, cursor) != centralHeaderSignature) {
            if (error)
                *error = QStringLiteral("角色包目录损坏");
            return false;
        }
        const quint16 nameLength = readU16(data, cursor + 28);
        const quint16 extraLength = readU16(data, cursor + 30);
        const quint16 commentLength = readU16(data, cursor + 32);
        if (cursor + 46 + nameLength + extraLength + commentLength > data.size()) {
            if (error)
                *error = QStringLiteral("角色包目录损坏");
            return false;
        }

        CentralEntry entry;
        entry.method = readU16(data, cursor + 10);
        entry.crc = readU32(data, cursor + 16);
        entry.compressedSize = readU32(data, cursor + 20);
        entry.uncompressedSize = readU32(data, cursor + 24);
        entry.localOffset = readU32(data, cursor + 42);
        entry.name = QString::fromUtf8(data.mid(cursor + 46, nameLength));
        cursor += 46 + nameLength + extraLength + commentLength;

        if (entry.name.endsWith(QLatin1Char('/')))
            continue;
        entries->append(entry);
    }
    return true;
}

bool readEntryData(const QByteArray &data, const CentralEntry &entry, QByteArray *content, QString *error)
{
    if (entry.method != zipMethodStored) {
        if (error)
            *error = QStringLiteral("角色包包含压缩条目（请用本软件重新导出）：%1").arg(entry.name);
        return false;
    }
    const int offset = int(entry.localOffset);
    if (offset + 30 > data.size() || readU32(data, offset) != localHeaderSignature) {
        if (error)
            *error = QStringLiteral("角色包条目损坏：%1").arg(entry.name);
        return false;
    }
    const quint16 nameLength = readU16(data, offset + 26);
    const quint16 extraLength = readU16(data, offset + 28);
    const int dataOffset = offset + 30 + nameLength + extraLength;
    if (dataOffset + int(entry.compressedSize) > data.size()) {
        if (error)
            *error = QStringLiteral("角色包条目数据不完整：%1").arg(entry.name);
        return false;
    }
    *content = data.mid(dataOffset, int(entry.compressedSize));
    if (crc32(*content) != entry.crc) {
        if (error)
            *error = QStringLiteral("角色包条目校验失败：%1").arg(entry.name);
        return false;
    }
    return true;
}

/// 把函数对象包装成 QRunnable 交给全局线程池执行（不引入 Qt6::Concurrent 依赖）。
class FunctionRunnable : public QRunnable
{
public:
    explicit FunctionRunnable(std::function<void()> function)
        : m_function(std::move(function))
    {
        setAutoDelete(true);
    }

    void run() override { m_function(); }

private:
    std::function<void()> m_function;
};

/// 在全局线程池执行 fn，结果通过 QPromise/QFuture 返回。
template <typename Result, typename Function>
QFuture<Result> runOnPool(Function &&function)
{
    auto promise = std::make_shared<QPromise<Result>>();
    promise->start();
    QFuture<Result> future = promise->future();
    QThreadPool::globalInstance()->start(
        new FunctionRunnable([promise, fn = std::forward<Function>(function)]() mutable {
            promise->addResult(fn());
            promise->finish();
        }));
    return future;
}
} // namespace

namespace characterpack
{
bool exportPack(const QString &characterDir, const QString &zipPath, QString *error)
{
    const QDir source(characterDir);
    if (!source.exists()) {
        if (error)
            *error = QStringLiteral("角色目录不存在：%1").arg(characterDir);
        return false;
    }

    const QString id = QFileInfo(characterDir).fileName();
    QStringList files;
    collectFiles(characterDir, QString(), &files);
    if (files.isEmpty()) {
        if (error)
            *error = QStringLiteral("角色目录为空");
        return false;
    }

    QByteArray output;
    QByteArray centralDirectory;
    quint32 localOffset = 0;
    for (const QString &relative : files) {
        QFile file(characterDir + QLatin1Char('/') + relative);
        if (!file.open(QIODevice::ReadOnly)) {
            if (error)
                *error = QStringLiteral("无法读取：%1").arg(relative);
            return false;
        }
        const QByteArray content = file.readAll();
        if (content.size() > 64 * 1024 * 1024) {
            if (error)
                *error = QStringLiteral("角色包单文件过大：%1").arg(relative);
            return false;
        }
        const QByteArray name = (id + QLatin1Char('/') + relative).toUtf8();
        const quint32 crc = crc32(content);

        appendU32(&output, localHeaderSignature);
        appendU16(&output, 20);
        appendU16(&output, utf8Flag);
        appendU16(&output, zipMethodStored);
        appendU16(&output, 0);
        appendU16(&output, 0);
        appendU32(&output, crc);
        appendU32(&output, static_cast<quint32>(content.size()));
        appendU32(&output, static_cast<quint32>(content.size()));
        appendU16(&output, static_cast<quint16>(name.size()));
        appendU16(&output, 0);
        output.append(name);
        output.append(content);

        appendU32(&centralDirectory, centralHeaderSignature);
        appendU16(&centralDirectory, 20);
        appendU16(&centralDirectory, 20);
        appendU16(&centralDirectory, utf8Flag);
        appendU16(&centralDirectory, zipMethodStored);
        appendU16(&centralDirectory, 0);
        appendU16(&centralDirectory, 0);
        appendU32(&centralDirectory, crc);
        appendU32(&centralDirectory, static_cast<quint32>(content.size()));
        appendU32(&centralDirectory, static_cast<quint32>(content.size()));
        appendU16(&centralDirectory, static_cast<quint16>(name.size()));
        appendU16(&centralDirectory, 0);
        appendU16(&centralDirectory, 0);
        appendU16(&centralDirectory, 0);
        appendU16(&centralDirectory, 0);
        appendU32(&centralDirectory, 0);
        appendU32(&centralDirectory, localOffset);
        centralDirectory.append(name);

        localOffset += quint32(30 + name.size() + content.size());
    }

    output.append(centralDirectory);

    appendU32(&output, endOfCentralSignature);
    appendU16(&output, 0);
    appendU16(&output, 0);
    appendU16(&output, static_cast<quint16>(files.size()));
    appendU16(&output, static_cast<quint16>(files.size()));
    appendU32(&output, static_cast<quint32>(centralDirectory.size()));
    appendU32(&output, localOffset);
    appendU16(&output, 0);

    QFile zip(zipPath);
    if (!zip.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = QStringLiteral("无法创建压缩包：%1").arg(zip.errorString());
        return false;
    }
    if (zip.write(output) != output.size()) {
        if (error)
            *error = QStringLiteral("写入压缩包失败");
        return false;
    }
    zip.close();
    return true;
}

bool importPack(const QString &zipPath, const QString &charactersDir,
                QString *newCharacterId, QString *error)
{
    QVector<CentralEntry> entries;
    if (!readZipEntries(zipPath, &entries, error))
        return false;

    QFile file(zipPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("无法读取角色包");
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    QString root;
    bool hasPersona = false;
    bool hasAvatar = false;
    for (const CentralEntry &entry : entries) {
        if (!isSafeRelativePath(entry.name)) {
            if (error)
                *error = QStringLiteral("角色包包含非法路径：%1").arg(entry.name);
            return false;
        }
        const int slash = entry.name.indexOf(QLatin1Char('/'));
        if (slash <= 0) {
            if (error)
                *error = QStringLiteral("角色包结构不合法：应只有一个顶层目录");
            return false;
        }
        const QString entryRoot = entry.name.left(slash);
        if (root.isEmpty())
            root = entryRoot;
        else if (root != entryRoot) {
            if (error)
                *error = QStringLiteral("角色包结构不合法：应只有一个顶层目录");
            return false;
        }
        if (entry.name.endsWith(QStringLiteral(".md")))
            hasPersona = true;
        if (entry.name.contains(QStringLiteral("/avatars/")))
            hasAvatar = true;
    }

    if (root.isEmpty() || !hasPersona || !hasAvatar) {
        if (error)
            *error = QStringLiteral("角色包不完整：缺少人设或形象");
        return false;
    }

    const QString targetDir = charactersDir + QLatin1Char('/') + root;
    if (QFileInfo::exists(targetDir)) {
        if (error)
            *error = QStringLiteral("角色已存在，请先删除或重命名：%1").arg(root);
        return false;
    }

    QDir().mkpath(targetDir);
    const int prefixLength = root.length() + 1;
    for (const CentralEntry &entry : entries) {
        QByteArray content;
        if (!readEntryData(data, entry, &content, error))
            return false;
        const QString relative = entry.name.mid(prefixLength);
        const QString target = targetDir + QLatin1Char('/') + relative;
        QDir().mkpath(QFileInfo(target).absolutePath());
        QFile output(target);
        if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (error)
                *error = QStringLiteral("写入失败：%1").arg(relative);
            return false;
        }
        output.write(content);
        output.close();
    }

    if (newCharacterId)
        *newCharacterId = root;
    return true;
}

QFuture<AsyncExportResult> exportPackAsync(const QString &characterDir, const QString &zipPath)
{
    return runOnPool<AsyncExportResult>([characterDir, zipPath] {
        AsyncExportResult result;
        result.ok = exportPack(characterDir, zipPath, &result.error);
        return result;
    });
}

QFuture<AsyncImportResult> importPackAsync(const QString &zipPath, const QString &charactersDir)
{
    return runOnPool<AsyncImportResult>([zipPath, charactersDir] {
        AsyncImportResult result;
        result.ok = importPack(zipPath, charactersDir, &result.characterId, &result.error);
        return result;
    });
}
} // namespace characterpack
