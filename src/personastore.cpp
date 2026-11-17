#include "personastore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>

namespace
{
const char *builtinId = "builtin-default";
const char *builtinName = "小柚（内置）";
const char *builtinPrompt = "你是小柚，一只住在用户桌面上的小精灵。"
                            "说话简短、可爱、带一点调皮，每次回复不超过三句话。"
                            "你了解用户的桌面环境，回答要自然，不要暴露自己是程序。";

QString defaultPersonaDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/personas");
}

/// 从人设文件首行的 Markdown 标题提取名字，没有标题时用文件名。
QString displayName(const QString &path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        const QString firstLine = QString::fromUtf8(file.readLine()).trimmed();
        if (firstLine.startsWith(QLatin1Char('#'))) {
            const QString name = firstLine.mid(1).trimmed();
            if (!name.isEmpty())
                return name;
        }
    }
    return QFileInfo(path).completeBaseName();
}
} // namespace

PersonaStore::PersonaStore(const QString &dirPath, QObject *parent)
    : QObject(parent)
    , m_dirPath(dirPath.isEmpty() ? defaultPersonaDir() : dirPath)
{
    reload();
}

void PersonaStore::reload()
{
    m_personas.clear();
    m_personas.append(Persona{QString::fromLatin1(builtinId), QString::fromUtf8(builtinName),
                              QString::fromUtf8(builtinPrompt), true});

    QDir dir(m_dirPath);
    const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.md")}, QDir::Files, QDir::Name);
    for (const QFileInfo &info : files) {
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const QString prompt = QString::fromUtf8(file.readAll()).trimmed();
        if (prompt.isEmpty())
            continue;
        m_personas.append(Persona{info.completeBaseName(), displayName(info.absoluteFilePath()), prompt, false});
    }
}

Persona PersonaStore::find(const QString &personaId) const
{
    for (const Persona &persona : m_personas) {
        if (persona.id == personaId)
            return persona;
    }
    return m_personas.isEmpty() ? Persona{} : m_personas.first();
}

bool PersonaStore::importFrom(const QString &filePath, QString *error)
{
    QFile source(filePath);
    if (!source.open(QIODevice::ReadOnly)) {
        if (error != nullptr)
            *error = tr("无法读取文件：%1").arg(source.errorString());
        return false;
    }

    const QString id = QFileInfo(filePath).completeBaseName();
    if (id.isEmpty() || id == QLatin1String(builtinId)) {
        if (error != nullptr)
            *error = tr("人设文件名无效");
        return false;
    }

    // 先把内容读进内存：导入源文件可能就在人设目录里，先清空目标会读不到内容。
    const QByteArray content = source.readAll();
    source.close();

    const QString target = m_dirPath + QLatin1Char('/') + id + QStringLiteral(".md");
    QDir().mkpath(m_dirPath);
    QFile output(target);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error != nullptr)
            *error = tr("无法写入人设目录：%1").arg(output.errorString());
        return false;
    }
    output.write(content);
    output.close();

    reload();
    emit changed();
    return true;
}

bool PersonaStore::remove(const QString &personaId)
{
    if (personaId.isEmpty() || personaId == QLatin1String(builtinId))
        return false;

    const QString path = m_dirPath + QLatin1Char('/') + personaId + QStringLiteral(".md");
    if (!QFile::exists(path))
        return false;
    if (!QFile::remove(path))
        return false;

    reload();
    emit changed();
    return true;
}
