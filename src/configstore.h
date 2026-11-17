#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <QObject>
#include <QString>

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
    bool autoExtractMemories = true;
    int proactiveFrequency = 0; // 0 关 1 低 2 中 3 高
    double petX = -1.0;
    double petY = -1.0;
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
    void setPetPosition(double x, double y);

    bool save(QString *error = nullptr);
    void load();

signals:
    void changed();

private:
    QString m_filePath;
    AppConfig m_config;
};

#endif // CONFIGSTORE_H
