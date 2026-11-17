/**
 * \file src/data/configstore.h
 * \brief 应用配置持久化：把 domain::AppConfig 读写到 config.json（原子写）。
 */

#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <QObject>
#include <QString>
#include <QVector>

#include "domain/appconfig.h"

/**
 * \brief 应用配置仓库：内存快照 + config.json 原子写入。
 *
 * 构造时按文件路径加载已有配置；所有 setter 只修改内存值，
 * 由 save() 统一落盘，保存成功后发出 changed()。
 * 本类只做存储，不包含任何交互规则。
 */
class ConfigStore : public QObject
{
    Q_OBJECT

public:
    /**
     * \brief 构造配置仓库并按需加载配置文件。
     * @param filePath config.json 路径；为空时使用 QStandardPaths 的应用数据目录。
     * @param parent 父对象。
     */
    explicit ConfigStore(const QString &filePath = QString(), QObject *parent = nullptr);

    /// 配置文件的绝对路径。
    QString filePath() const { return m_filePath; }
    /// 当前内存中的配置快照。
    const AppConfig &values() const { return m_config; }

    /**
     * \brief 设置模型连接参数（清空当前配置档选择）。
     * @param preset 预设标识。
     * @param baseUrl 接口根地址。
     * @param apiKey 接口密钥。
     * @param model 模型名。
     */
    void setLlm(const QString &preset, const QString &baseUrl, const QString &apiKey, const QString &model);
    /**
     * \brief 设置当前人设 id。
     * @param personaId 人设 id；空串表示未选择。
     */
    void setPersonaId(const QString &personaId);
    /**
     * \brief 设置当前角色 id。
     * @param characterId 角色 id；空串表示未选择。
     */
    void setCharacterId(const QString &characterId);
    /**
     * \brief 设置记忆相关偏好。
     * @param autoExtract 是否自动抽取记忆。
     * @param proactiveFrequency 主动对话频率（0 关 1 低 2 中 3 高）。
     */
    void setMemoryPrefs(bool autoExtract, int proactiveFrequency);
    /**
     * \brief 设置桌宠行为开关。
     * @param idleMotion 是否启用待机动作。
     * @param idleAmplitude 待机动作幅度（0.2~2.0）。
     * @param clickThrough 是否开启点击穿透。
     * @param debugHitAreas 是否显示命中区调试。
     */
    void setPetBehavior(bool idleMotion, double idleAmplitude, bool clickThrough, bool debugHitAreas);
    /**
     * \brief 设置指定桌宠的位置。
     * @param index 桌宠下标。
     * @param x 横坐标；-1 表示未设置。
     * @param y 纵坐标；-1 表示未设置。
     */
    void setPetPosition(int index, double x, double y);
    /**
     * \brief 设置桌宠数量。
     * @param count 桌宠数量（1~3）。
     */
    void setPetCount(int count);
    /**
     * \brief 设置贴边隐藏开关。
     * @param enabled 是否贴边隐藏。
     */
    void setEdgeHide(bool enabled);
    /**
     * \brief 设置指定桌宠绑定的角色。
     * @param index 桌宠下标。
     * @param characterId 角色 id；空串表示使用当前角色。
     */
    void setPetCharacter(int index, const QString &characterId);
    /**
     * \brief 设置当前模式。
     * @param mode 0 休闲 1 工作 2 勿扰。
     */
    void setMode(int mode);
    /**
     * \brief 设置主题。
     * @param theme 0 浅色 1 深色 2 跟随系统。
     */
    void setTheme(int theme);
    /**
     * \brief 设置界面语言。
     * @param language 0 中文 1 English 2 跟随系统。
     */
    void setLanguage(int language);
    /**
     * \brief 设置生成参数。
     * @param temperature 采样温度。
     * @param maxTokens 最大回复 token 数，0 表示默认。
     */
    void setGeneration(double temperature, int maxTokens);
    /**
     * \brief 覆盖全部命名的模型配置档。
     * @param profiles 配置档列表。
     * @param activeIndex 当前生效的配置档下标，-1 表示未使用。
     */
    void setModelProfiles(const QVector<ModelProfile> &profiles, int activeIndex);
    /**
     * \brief 设置按前台应用自动切换模式。
     * @param enabled 是否启用自动模式。
     */
    void setAutoMode(bool enabled);
    /**
     * \brief 设置好感度跨天衰减。
     * @param enabled 是否启用衰减。
     */
    void setIntimacyDecay(bool enabled);
    /**
     * \brief 设置悬浮球的可见性与位置。
     * @param visible 是否可见。
     * @param x 横坐标；-1 表示未设置。
     * @param y 纵坐标；-1 表示未设置。
     */
    void setBall(bool visible, double x, double y);
    /**
     * \brief 标记引导流程是否完成。
     * @param done 是否已完成。
     */
    void setOnboardingDone(bool done);
    /**
     * \brief 设置更新检查地址。
     * @param url 更新信息地址。
     */
    void setUpdateUrl(const QString &url);

    /**
     * \brief 把当前内存配置原子写入 config.json。
     * @param[out] error 失败时写入可读原因；可为空。
     * @return 写入是否成功。
     */
    bool save(QString *error = nullptr);
    /**
     * \brief 从 config.json 重新加载配置；文件缺失或损坏时保留默认值。
     */
    void load();

signals:
    /// 配置内容发生变化（保存成功后发出）。
    void changed();

private:
    QString m_filePath; ///< 配置文件路径。
    AppConfig m_config; ///< 当前配置快照。
};

#endif // CONFIGSTORE_H
