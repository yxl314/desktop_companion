/**
 * \file src/data/intimacyrepository.h
 * \brief 好感度持久化：每个角色一个 intimacy/&lt;characterId&gt;.json（原子写）。
 *
 * 原 src/intimacyservice.h；按 docs/架构重构-MVVM.md §1 迁入 data 层，
 * 文件名改为 intimacyrepository 以区别于 services 层的领域服务。
 * 类名保持 IntimacyService（对外 API 不变）。
 */

#ifndef INTIMACYSERVICE_H
#define INTIMACYSERVICE_H

#include <QDate>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

#include "domain/intimacy.h"

/**
 * \brief 单个角色的好感度仓库：数据落在 intimacy/&lt;characterId&gt;.json（QSaveFile 原子写）。
 *
 * 使用方式：load(characterId) → add(...)/applyDailyDecay(...) → 自动保存；
 * 文件损坏时回退默认值并把原文件备份为 &lt;文件名&gt;.bak。
 * 本类只做存储与分数计算，不感知界面。
 */
class IntimacyService : public QObject
{
    Q_OBJECT

public:
    /// 好感度等级的领域名别名，保持 IntimacyService::Level 旧引用可用。
    using Level = IntimacyLevel;

    static constexpr int minScore = 0;            ///< 分数下界。
    static constexpr int maxScore = 100;          ///< 分数上界。
    static constexpr int maxEvents = 50;          ///< 事件记录条数上限。
    static constexpr int familiarThreshold = 20;  ///< 熟悉档阈值。
    static constexpr int closeThreshold = 50;     ///< 亲近档阈值。
    static constexpr int trustedThreshold = 80;   ///< 信赖档阈值。

    /**
     * \brief 构造好感度仓库。
     * @param dirPath 好感度 JSON 所在目录；为空时使用数据目录下的 intimacy/。
     * @param parent 父对象。
     */
    explicit IntimacyService(const QString &dirPath = QString(), QObject *parent = nullptr);

    /// 好感度 JSON 所在目录。
    QString dirPath() const { return m_dirPath; }
    /// 当前已加载的角色 id；未加载时为空。
    QString characterId() const { return m_characterId; }
    /// 是否已加载某个角色。
    bool loaded() const { return !m_characterId.isEmpty(); }
    /**
     * \brief 指定角色的好感度文件路径。
     * @param characterId 角色 id。
     * @return JSON 文件的绝对路径。
     */
    QString filePath(const QString &characterId) const;
    /// 最近一次失败原因。
    QString lastError() const { return m_lastError; }

    /// 当前分数（0~100）。
    int score() const { return m_score; }
    /// 当前分数对应的等级。
    Level level() const { return levelForScore(m_score); }
    /// 当前分数对应的等级中文名。
    QString levelName() const { return levelNameForScore(m_score); }
    /// 跨天衰减是否启用。
    bool decayEnabled() const { return m_decayEnabled; }
    /// 当前角色的事件记录（最多 maxEvents 条）。
    QVector<IntimacyEvent> events() const { return m_events; }

    /**
     * \brief 分数 → 等级。
     * @param score 分数（0~100）。
     * @return 对应等级。
     */
    static Level levelForScore(int score);
    /**
     * \brief 分数 → 等级中文名。
     * @param score 分数（0~100）。
     * @return 等级显示名（如「亲近」）。
     */
    static QString levelNameForScore(int score);
    /**
     * \brief 等级 → 等级中文名。
     * @param level 等级。
     * @return 等级显示名。
     */
    static QString levelName(Level level);

    /**
     * \brief 读取角色好感度。
     *
     * 文件不存在时使用默认值；文件损坏时备份 .bak 并返回 false（状态为默认值）。
     * @param characterId 角色 id。
     * @param[out] error 非空时写入失败原因。
     * @return 是否读取成功。
     */
    bool load(const QString &characterId, QString *error = nullptr);

    /**
     * \brief 原子写入当前角色的好感度文件。
     * @param[out] error 非空时写入失败原因。
     * @return 是否写入成功；未 load 任何角色时失败。
     */
    bool save(QString *error = nullptr);

    /**
     * \brief 增减好感度（负数可扣分）。
     *
     * 自动夹到 [0, 100] 并追加事件，成功后立即落盘。
     * @param delta 变化量，可为负。
     * @param reason 可读原因。
     * @param[out] error 非空时写入失败原因。
     * @return 是否写入成功。
     */
    bool add(int delta, const QString &reason, QString *error = nullptr);

    /**
     * \brief 设置跨天衰减开关，默认关闭。
     * @param enabled 是否启用。
     * @param[out] error 非空时写入失败原因。
     * @return 是否写入成功。
     */
    bool setDecayEnabled(bool enabled, QString *error = nullptr);

    /**
     * \brief 按自然日衰减：与 lastDecayDate 相差几天就扣几次分。
     *
     * 每天扣 pointsPerDay；未启用衰减时为空操作。
     * @param pointsPerDay 每天扣除的分数。
     * @param today 视为今天的日期。
     * @param[out] error 非空时写入失败原因。
     * @return 是否处理成功。
     */
    bool applyDailyDecay(int pointsPerDay = 1, const QDate &today = QDate::currentDate(),
                         QString *error = nullptr);

    /**
     * \brief 序列化为 JSON 对象。
     * @return 当前好感度状态的 JSON 表示。
     */
    QJsonObject toJson() const;
    /**
     * \brief 从 JSON 对象恢复好感度状态。
     * @param object JSON 对象。
     * @param[out] error 非空时写入失败原因。
     * @return 是否解析成功。
     */
    bool fromJson(const QJsonObject &object, QString *error = nullptr);

signals:
    /// 好感度状态发生变化（加载、增减、衰减后发出）。
    void changed();

private:
    void resetToDefault();
    void appendEvent(int appliedDelta, const QString &reason);
    QString absoluteFilePath(const QString &characterId) const;
    bool backupCorruptFile(const QString &path);
    bool fail(const QString &message, QString *error);
    void setErrorText(const QString &text);

    QString m_dirPath;              ///< 好感度 JSON 目录。
    QString m_characterId;          ///< 当前已加载的角色 id。
    QString m_lastError;            ///< 最近一次失败原因。
    int m_score = 0;                ///< 当前分数。
    bool m_decayEnabled = false;    ///< 跨天衰减是否启用。
    QDate m_lastDecayDate;          ///< 最近一次衰减日期。
    QString m_updatedAt;            ///< 最近更新时间（ISO 8601）。
    QVector<IntimacyEvent> m_events; ///< 事件记录。
};

#endif // INTIMACYSERVICE_H
