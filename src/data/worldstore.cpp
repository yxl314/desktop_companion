#include "data/worldstore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFuture>
#include <QPointer>
#include <QPromise>
#include <QRunnable>
#include <QSaveFile>
#include <QStandardPaths>
#include <QThreadPool>

#include <functional>
#include <memory>
#include <utility>

#include "services/personafile.h"

namespace
{
const char *const translationContext = "WorldStore";

QString trText(const char *text)
{
    return QCoreApplication::translate(translationContext, text);
}

void setError(QString *error, const QString &message)
{
    if (error != nullptr)
        *error = message;
}

QString defaultDataDir()
{
    QString location = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!location.isEmpty())
        return location;
    return QDir::homePath() + QStringLiteral("/.desktop_companion");
}

bool isSafeId(const QString &id)
{
    return !id.isEmpty() && sanitizeFileId(id) == id;
}

bool readTextFile(const QString &path, QString *text, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(error, trText("无法读取文件：%1").arg(file.errorString()));
        return false;
    }
    *text = QString::fromUtf8(file.readAll());
    return true;
}

bool writeTextFile(const QString &path, const QString &text, QString *error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(error, trText("无法写入文件：%1").arg(file.errorString()));
        return false;
    }
    file.write(text.toUtf8());
    if (!file.commit()) {
        setError(error, trText("写入文件失败：%1").arg(file.errorString()));
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

WorldStore::WorldStore(const QString &dataDir, QObject *parent)
    : QObject(parent)
    , m_dataDir(dataDir.isEmpty() ? defaultDataDir() : dataDir)
{
}

QString WorldStore::worldsDir() const
{
    return QDir(m_dataDir).filePath(QStringLiteral("worlds"));
}

QString WorldStore::worldDir(const QString &id) const
{
    return QDir(worldsDir()).filePath(id);
}

QString WorldStore::worldPath(const QString &id) const
{
    return QDir(worldDir(id)).filePath(QStringLiteral("world.md"));
}

bool WorldStore::contains(const QString &id) const
{
    return isSafeId(id) && QDir(worldDir(id)).exists();
}

QList<World> WorldStore::list() const
{
    QList<World> worlds;
    const QDir root(worldsDir());
    const QFileInfoList entries = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &entry : entries) {
        const QString id = entry.fileName();
        if (!contains(id))
            continue;
        World world;
        if (!read(id, &world, nullptr))
            continue;
        worlds.append(world);
    }
    return worlds;
}

bool WorldStore::read(const QString &id, World *world, QString *error) const
{
    if (world == nullptr) {
        setError(error, tr("世界观输出指针为空"));
        return false;
    }
    *world = World{};
    if (!contains(id)) {
        setError(error, tr("世界观不存在：%1").arg(id));
        return false;
    }

    QString markdown;
    if (!readTextFile(worldPath(id), &markdown, error))
        return false;

    PersonaData data;
    if (!parsePersonaMarkdown(markdown, id, &data, error))
        return false;
    world->id = id;
    world->name = data.name;
    world->content = data.body;
    return true;
}

bool WorldStore::write(const QString &id, const World &world, QString *error)
{
    if (!writeWorld(id, world, error))
        return false;
    emit changed();
    return true;
}

bool WorldStore::create(const QString &name, QString *newId, QString *error)
{
    const QString displayName = name.trimmed();
    if (displayName.isEmpty()) {
        setError(error, tr("世界观名不能为空"));
        return false;
    }
    const QString id = sanitizeFileId(displayName);
    if (id.isEmpty()) {
        setError(error, tr("世界观名无法用作目录名：%1").arg(displayName));
        return false;
    }
    if (QDir(worldDir(id)).exists()) {
        setError(error, tr("世界观已存在：%1").arg(id));
        return false;
    }

    World world;
    world.id = id;
    world.name = displayName;
    if (!writeWorld(id, world, error)) {
        QDir(worldDir(id)).removeRecursively();
        return false;
    }
    if (newId != nullptr)
        *newId = id;
    emit changed();
    return true;
}

bool WorldStore::importFrom(const QString &filePath, QString *newId, QString *error)
{
    if (!importFromImpl(filePath, newId, error))
        return false;
    emit changed();
    return true;
}

QFuture<WorldImportResult> WorldStore::importFromAsync(const QString &filePath)
{
    const QPointer<WorldStore> self(this);
    return runOnPool<WorldImportResult>([self, filePath] {
        WorldImportResult result;
        if (self.isNull()) {
            result.error = trText("世界观仓库已销毁");
            return result;
        }
        result.ok = self->importFromImpl(filePath, &result.worldId, &result.error);
        if (result.ok) {
            QMetaObject::invokeMethod(
                self.data(),
                [self] {
                    if (!self.isNull())
                        emit self->changed();
                },
                Qt::QueuedConnection);
        }
        return result;
    });
}

bool WorldStore::importFromImpl(const QString &filePath, QString *newId, QString *error)
{
    if (filePath.isEmpty()) {
        setError(error, trText("世界观文件路径为空"));
        return false;
    }
    QString markdown;
    if (!readTextFile(filePath, &markdown, error))
        return false;

    const QString fallbackName = QFileInfo(filePath).completeBaseName();
    PersonaData data;
    if (!parsePersonaMarkdown(markdown, fallbackName, &data, error))
        return false;

    const QString id = sanitizeFileId(fallbackName);
    if (id.isEmpty()) {
        setError(error, trText("世界观文件名无法用作目录名：%1").arg(fallbackName));
        return false;
    }
    if (QDir(worldDir(id)).exists()) {
        setError(error, trText("世界观已存在：%1").arg(id));
        return false;
    }

    World world;
    world.id = id;
    world.name = data.name.trimmed().isEmpty() ? fallbackName : data.name;
    world.content = data.body;
    if (!writeWorld(id, world, error)) {
        QDir(worldDir(id)).removeRecursively();
        return false;
    }
    if (newId != nullptr)
        *newId = id;
    return true;
}

bool WorldStore::rename(const QString &id, const QString &newName, QString *error)
{
    World world;
    if (!read(id, &world, error))
        return false;
    const QString displayName = newName.trimmed();
    if (displayName.isEmpty()) {
        setError(error, tr("世界观名不能为空"));
        return false;
    }
    world.name = displayName;
    return write(id, world, error);
}

bool WorldStore::remove(const QString &id, QString *error)
{
    if (!contains(id)) {
        setError(error, tr("世界观不存在：%1").arg(id));
        return false;
    }
    const QString directory = worldDir(id);
    if (!QDir(directory).removeRecursively()) {
        setError(error, tr("无法删除世界观目录：%1").arg(directory));
        return false;
    }
    emit changed();
    return true;
}

bool WorldStore::writeWorld(const QString &id, const World &world, QString *error) const
{
    if (!isSafeId(id)) {
        setError(error, tr("世界观 id 无效：%1").arg(id));
        return false;
    }
    if (!QDir().mkpath(worldDir(id))) {
        setError(error, tr("无法创建世界观目录：%1").arg(worldDir(id)));
        return false;
    }

    PersonaData data;
    data.id = id;
    data.name = world.name.trimmed().isEmpty() ? id : world.name;
    data.body = world.content;
    return writeTextFile(worldPath(id), renderPersonaMarkdown(data), error);
}
