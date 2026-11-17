#ifndef INTIMACYSERVICE_H
#define INTIMACYSERVICE_H

#include <QDate>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

/// 一条好感度变化事件，用于控制中心展示与跨天衰减追溯。
struct IntimacyEvent
{
    QString at;
    int delta = 0;
    int score = 0;
    QString reason;
};

/// 单个角色的好感度：数据落在 intimacy/<characterId>.json（QSaveFile 原子写）。
///
/// 使用方式：load(characterId) → add(...)/applyDailyDecay(...) → 自动保存；
/// 文件损坏时回退默认值并把原文件备份为 <文件名>.bak。
class IntimacyService : public QObject
{
    Q_OBJECT

public:
    enum class Level
    {
        Stranger,
        Familiar,
        Close,
        Trusted
    };

    static constexpr int minScore = 0;
    static constexpr int maxScore = 100;
    static constexpr int maxEvents = 50;
    static constexpr int familiarThreshold = 20;
    static constexpr int closeThreshold = 50;
    static constexpr int trustedThreshold = 80;

    /// dirPath 为好感度 JSON 所在目录；为空时使用数据目录下的 intimacy/。
    explicit IntimacyService(const QString &dirPath = QString(), QObject *parent = nullptr);

    QString dirPath() const { return m_dirPath; }
    QString characterId() const { return m_characterId; }
    bool loaded() const { return !m_characterId.isEmpty(); }
    QString filePath(const QString &characterId) const;
    QString lastError() const { return m_lastError; }

    int score() const { return m_score; }
    Level level() const { return levelForScore(m_score); }
    QString levelName() const { return levelNameForScore(m_score); }
    bool decayEnabled() const { return m_decayEnabled; }
    QVector<IntimacyEvent> events() const { return m_events; }

    static Level levelForScore(int score);
    static QString levelNameForScore(int score);
    static QString levelName(Level level);

    /// 读取角色好感度；文件不存在时使用默认值，损坏时备份 .bak 并返回 false（状态为默认值）。
    bool load(const QString &characterId, QString *error = nullptr);

    /// 原子写入当前角色的好感度文件；未 load 任何角色时失败。
    bool save(QString *error = nullptr);

    /// 增减好感度（负数可扣分），自动夹到 [0, 100] 并追加事件，成功后立即落盘。
    bool add(int delta, const QString &reason, QString *error = nullptr);

    /// 跨天衰减开关，默认关闭；开启后由 applyDailyDecay 按自然日扣分。
    bool setDecayEnabled(bool enabled, QString *error = nullptr);

    /// 按自然日衰减：与 lastDecayDate 相差几天就扣几次分，每天扣 pointsPerDay；未开启时为空操作。
    bool applyDailyDecay(int pointsPerDay = 1, const QDate &today = QDate::currentDate(),
                         QString *error = nullptr);

    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &object, QString *error = nullptr);

signals:
    void changed();

private:
    void resetToDefault();
    void appendEvent(int appliedDelta, const QString &reason);
    QString absoluteFilePath(const QString &characterId) const;
    bool backupCorruptFile(const QString &path);
    bool fail(const QString &message, QString *error);
    void setErrorText(const QString &text);

    QString m_dirPath;
    QString m_characterId;
    QString m_lastError;
    int m_score = 0;
    bool m_decayEnabled = false;
    QDate m_lastDecayDate;
    QString m_updatedAt;
    QVector<IntimacyEvent> m_events;
};

#endif // INTIMACYSERVICE_H
