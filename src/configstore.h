#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <QObject>
#include <QPointF>
#include <QString>
#include <QVector>

/// 应用配置；保存到数据目录的 config.json，采用原子写。
struct AppConfig
{
    QString preset = QStringLiteral("ollama");
    QString baseUrl;
    QString apiKey;
    QString model = QStringLiteral("qwen2.5:7b");
    QString personaId;
    QString characterId;
    bool idleMotion = true;
    double idleAmplitude = 1.0;
    bool clickThrough = true;
    bool debugHitAreas = false;
    bool edgeHide = true;
    bool autoExtractMemories = true;
    int proactiveFrequency = 0; // 0 关 1 低 2 中 3 高
    int mode = 0; // 0 休闲 1 工作 2 勿扰
    bool ballVisible = true;
    double ballX = -1.0;
    double ballY = -1.0;
    bool onboardingDone = false;
    QString updateUrl;
    int petCount = 1;
    QVector<QPointF> petPositions; // 每个桌宠的位置，-1 表示未设置
    QVector<QString> petCharacters; // 每个桌宠绑定的角色 id（空 = 当前角色）
};

class ConfigStore : public QObject
{
    Q_OBJECT

public:
    explicit ConfigStore(const QString &filePath = QString(), QObject *parent = nullptr);

    QString filePath() const { return m_filePath; }
    const AppConfig &values() const { return m_config; }

    void setLlm(const QString &preset, const QString &baseUrl, const QString &apiKey, const QString &model);
    void setPersonaId(const QString &personaId);
    void setCharacterId(const QString &characterId);
    void setMemoryPrefs(bool autoExtract, int proactiveFrequency);
    void setPetBehavior(bool idleMotion, double idleAmplitude, bool clickThrough, bool debugHitAreas);
    void setPetPosition(int index, double x, double y);
    void setPetCount(int count);
    void setEdgeHide(bool enabled);
    void setPetCharacter(int index, const QString &characterId);
    void setMode(int mode);
    void setBall(bool visible, double x, double y);
    void setOnboardingDone(bool done);
    void setUpdateUrl(const QString &url);

    bool save(QString *error = nullptr);
    void load();

signals:
    void changed();

private:
    QString m_filePath;
    AppConfig m_config;
};

#endif // CONFIGSTORE_H
