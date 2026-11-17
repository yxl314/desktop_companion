/**
 * \file src/data/personastore.h
 * \brief 全局人设目录持久化：导入 .md 人设与内置默认人设。
 */

#ifndef PERSONASTORE_H
#define PERSONASTORE_H

#include <QFuture>
#include <QList>
#include <QObject>
#include <QString>

/**
 * \brief 一套全局人设：提示词正文来自导入的 .md 文件或内置默认人设。
 */
struct Persona
{
    QString id;          ///< 人设 id。
    QString name;        ///< 显示名。
    QString prompt;      ///< 提示词正文。
    bool builtin = false; ///< 是否为内置人设（内置人设不可删除）。
};

/**
 * \brief 异步导入人设的结果。
 */
struct PersonaImportResult
{
    bool ok = false; ///< 是否导入成功。
    QString error;   ///< 失败原因（可读文案，失败时非空）。
};

/**
 * \brief 全局人设仓库：扫描人设目录、导入与删除 .md 人设。
 *
 * 构造时扫描目录并加入内置默认人设；本类只做存储，不包含交互规则。
 */
class PersonaStore : public QObject
{
    Q_OBJECT

public:
    /**
     * \brief 构造人设仓库并加载人设目录。
     * @param dirPath 人设目录；为空时使用数据目录下的 personas/。
     * @param parent 父对象。
     */
    explicit PersonaStore(const QString &dirPath = QString(), QObject *parent = nullptr);

    /// 人设目录路径。
    QString dirPath() const { return m_dirPath; }
    /// 当前全部人设（内置人设在最前）。
    QList<Persona> personas() const { return m_personas; }

    /**
     * \brief 按 id 查找人设。
     * @param personaId 人设 id。
     * @return 指定 id 的人设；id 为空或不存在时返回第一个可用人设。
     */
    Persona find(const QString &personaId) const;

    /**
     * \brief 把外部 .md 导入人设目录。
     * @param filePath 外部 .md 路径。
     * @param[out] error 非空时写入失败原因。
     * @return 是否导入成功。
     */
    bool importFrom(const QString &filePath, QString *error);

    /**
     * \brief 异步导入外部 .md 人设（G-H-04：耗时操作不冻结 UI）。
     *
     * 文件复制在全局线程池执行；成功后回到对象所属线程 reload() 并发出 changed()。
     * 错误文案与同步版 importFrom 一致。
     * @param filePath 外部 .md 路径。
     * @return 异步结果。
     */
    QFuture<PersonaImportResult> importFromAsync(const QString &filePath);

    /**
     * \brief 删除导入的人设。
     * @param personaId 人设 id。
     * @return 是否删除成功；内置人设不可删除。
     */
    bool remove(const QString &personaId);

signals:
    /// 人设列表发生变化（导入、删除后发出）。
    void changed();

private:
    void reload();
    /// 把外部 .md 复制进人设目录的实际实现（只做文件 IO，不修改内存列表）。
    bool copyImportFile(const QString &filePath, QString *error) const;

    QString m_dirPath;        ///< 人设目录路径。
    QList<Persona> m_personas; ///< 当前全部人设。
};

#endif // PERSONASTORE_H
