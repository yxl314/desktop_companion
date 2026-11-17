/**
 * \file src/domain/appconfig.h
 * \brief 应用配置的领域值类型：模型连接参数与应用级偏好。
 *
 * 本文件只包含纯数据定义，不依赖 Qt GUI、不做任何 IO；
 * 持久化由 data/configstore 负责，界面通过 ViewModel 读写。
 */

#ifndef DOMAIN_APPCONFIG_H
#define DOMAIN_APPCONFIG_H

#include <QPointF>
#include <QString>
#include <QVector>

/**
 * \brief 一套命名的模型连接配置（多配置档）。
 *
 * 用于设置页的「多配置」功能：每套配置保存名称、预设、接口地址、
 * 密钥、模型名与生成参数；AppConfig::activeProfile 指向当前生效的配置。
 */
struct ModelProfile
{
    QString name;          ///< 配置名称（用户可读）。
    QString preset;        ///< 预设标识（如 ollama / openai）。
    QString baseUrl;       ///< OpenAI 兼容接口根地址。
    QString apiKey;        ///< 接口密钥（落盘前经 services/secretstore 保护）。
    QString model;         ///< 模型名。
    double temperature = 0.8; ///< 采样温度，越大越发散。
    int maxTokens = 0;     ///< 单次回复最大 token 数，0 表示由服务端默认。
};

/**
 * \brief 应用全局配置快照。
 *
 * 对应数据目录下的 config.json，由 data/ConfigStore 负责原子读写。
 * 字段按「模型 / 角色 / 桌宠行为 / 模式 / 外观 / 引导与更新」分组，
 * 所有默认值即首次启动时的出厂配置。
 */
struct AppConfig
{
    // ---- 模型连接 ----
    QString preset = QStringLiteral("ollama");      ///< 模型预设标识。
    QString baseUrl;                                ///< 接口根地址。
    QString apiKey;                                 ///< 接口密钥。
    QString model = QStringLiteral("qwen2.5:7b");   ///< 模型名。
    double temperature = 0.8;                       ///< 采样温度。
    int maxTokens = 0;                              ///< 最大回复 token 数，0 表示默认。
    QVector<ModelProfile> modelProfiles;            ///< 全部命名的模型配置档。
    int activeProfile = -1;                         ///< 当前配置档下标，-1 表示未使用配置档。

    // ---- 角色与人设 ----
    QString personaId;             ///< 当前人设 id。
    QString characterId;           ///< 当前角色 id。

    // ---- 桌宠行为 ----
    bool idleMotion = true;        ///< 是否启用待机动作。
    double idleAmplitude = 1.0;    ///< 待机动作幅度（0.2~2.0）。
    bool clickThrough = true;      ///< 是否开启点击穿透。
    bool debugHitAreas = false;    ///< 是否显示命中区调试。
    bool edgeHide = true;          ///< 是否贴边隐藏。
    bool autoExtractMemories = true; ///< 是否自动抽取记忆。
    int proactiveFrequency = 0;    ///< 主动对话频率：0 关 1 低 2 中 3 高。
    int mode = 0;                  ///< 当前模式：0 休闲 1 工作 2 勿扰。
    int theme = 0;                 ///< 主题：0 浅色 1 深色 2 跟随系统。
    int language = 2;              ///< 语言：0 中文 1 English 2 跟随系统。
    bool autoMode = false;         ///< 是否按前台应用自动切换模式。
    bool intimacyDecay = false;    ///< 好感度跨天衰减，默认关闭。
    bool ballVisible = true;       ///< 是否显示悬浮球。
    double ballX = -1.0;           ///< 悬浮球横坐标，-1 表示未设置。
    double ballY = -1.0;           ///< 悬浮球纵坐标，-1 表示未设置。
    bool onboardingDone = false;   ///< 引导是否已完成。
    QString updateUrl;             ///< 更新检查地址。
    int petCount = 1;              ///< 桌宠数量（1~3）。
    QVector<QPointF> petPositions; ///< 每个桌宠的位置，-1 表示未设置。
    QVector<QString> petCharacters; ///< 每个桌宠绑定的角色 id（空 = 当前角色）。
};

#endif // DOMAIN_APPCONFIG_H
