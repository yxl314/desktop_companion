#ifndef PERSONASTORE_H
#define PERSONASTORE_H

#include <QList>
#include <QObject>
#include <QString>

/// 一套人设：提示词正文来自导入的 .md 文件或内置默认人设。
struct Persona
{
    QString id;
    QString name;
    QString prompt;
    bool builtin = false;
};

class PersonaStore : public QObject
{
    Q_OBJECT

public:
    explicit PersonaStore(const QString &dirPath = QString(), QObject *parent = nullptr);

    QString dirPath() const { return m_dirPath; }
    QList<Persona> personas() const { return m_personas; }

    /// 返回指定 id 的人设；id 为空或不存在时返回第一个可用人设。
    Persona find(const QString &personaId) const;

    /// 把外部 .md 导入人设目录；失败时通过 error 返回原因。
    bool importFrom(const QString &filePath, QString *error);

    /// 删除导入的人设；内置人设不可删除。
    bool remove(const QString &personaId);

signals:
    void changed();

private:
    void reload();

    QString m_dirPath;
    QList<Persona> m_personas;
};

#endif // PERSONASTORE_H
