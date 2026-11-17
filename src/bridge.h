#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <QPointF>
#include <QString>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class CharacterStore;
class IntimacyService;
class MemoryStore;
class ChatService;
class ChatStore;
class ConfigStore;
class PersonaStore;
class WorldStore;

/// QML 与 C++ 的唯一桥接面：命令走 Q_INVOKABLE，事件走信号。
class Bridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool petVisible READ petVisible WRITE setPetVisible NOTIFY petVisibleChanged)
    Q_PROPERTY(bool chatVisible READ chatVisible WRITE setChatVisible NOTIFY chatVisibleChanged)
    Q_PROPERTY(bool managerVisible READ managerVisible WRITE setManagerVisible NOTIFY managerVisibleChanged)
    Q_PROPERTY(bool bubbleVisible READ bubbleVisible WRITE setBubbleVisible NOTIFY bubbleVisibleChanged)
    Q_PROPERTY(QString bubbleText READ bubbleText NOTIFY bubbleTextChanged)
    Q_PROPERTY(double bubbleX READ bubbleX NOTIFY bubblePosChanged)
    Q_PROPERTY(double bubbleY READ bubbleY NOTIFY bubblePosChanged)
    Q_PROPERTY(double petX READ petX NOTIFY petPosChanged)
    Q_PROPERTY(double petY READ petY NOTIFY petPosChanged)
    Q_PROPERTY(QVariantList pets READ pets NOTIFY petLayoutChanged)
    Q_PROPERTY(int petCount READ petCount WRITE setPetCount NOTIFY petLayoutChanged)
    Q_PROPERTY(bool hasSavedPos READ hasSavedPos CONSTANT)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY chatStateChanged)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
    Q_PROPERTY(QString personaName READ personaName NOTIFY settingsChanged)
    Q_PROPERTY(QString modelSummary READ modelSummary NOTIFY settingsChanged)
    Q_PROPERTY(bool llmReady READ llmReady NOTIFY settingsChanged)
    Q_PROPERTY(QVariantList personas READ personas NOTIFY personasChanged)
    Q_PROPERTY(QStringList presets READ presets CONSTANT)
    Q_PROPERTY(QString presetName READ presetName NOTIFY settingsChanged)
    Q_PROPERTY(QString baseUrl READ baseUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString apiKey READ apiKey NOTIFY settingsChanged)
    Q_PROPERTY(QString modelName READ modelName NOTIFY settingsChanged)
    Q_PROPERTY(QString testResult READ testResult NOTIFY testResultChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString dataDir READ dataDir CONSTANT)
    Q_PROPERTY(QString logDir READ logDir CONSTANT)
    Q_PROPERTY(QVariantList characters READ characters NOTIFY charactersChanged)
    Q_PROPERTY(QVariantList worlds READ worlds NOTIFY worldsChanged)
    Q_PROPERTY(QString currentCharacterId READ currentCharacterId NOTIFY settingsChanged)
    Q_PROPERTY(QString currentCharacterName READ currentCharacterName NOTIFY settingsChanged)
    Q_PROPERTY(QString avatarPath READ currentAvatarPath NOTIFY avatarPathChanged)
    Q_PROPERTY(QString currentPersonaId READ currentPersonaId NOTIFY settingsChanged)
    Q_PROPERTY(int managerPage READ managerPage WRITE setManagerPage NOTIFY managerPageChanged)
    Q_PROPERTY(QVariantList memories READ memories NOTIFY memoriesChanged)
    Q_PROPERTY(int intimacyScore READ intimacyScore NOTIFY intimacyChanged)
    Q_PROPERTY(QString intimacyLevel READ intimacyLevel NOTIFY intimacyChanged)
    Q_PROPERTY(bool autoExtractMemories READ autoExtractMemories WRITE setAutoExtractMemories NOTIFY settingsChanged)
    Q_PROPERTY(int proactiveFrequency READ proactiveFrequency WRITE setProactiveFrequency NOTIFY settingsChanged)
    Q_PROPERTY(bool idleMotion READ idleMotion WRITE setIdleMotion NOTIFY settingsChanged)
    Q_PROPERTY(double idleAmplitude READ idleAmplitude WRITE setIdleAmplitude NOTIFY settingsChanged)
    Q_PROPERTY(bool clickThrough READ clickThrough WRITE setClickThrough NOTIFY settingsChanged)
    Q_PROPERTY(bool debugHitAreas READ debugHitAreas WRITE setDebugHitAreas NOTIFY settingsChanged)
    Q_PROPERTY(bool edgeHide READ edgeHide WRITE setEdgeHide NOTIFY settingsChanged)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY settingsChanged)
    Q_PROPERTY(bool ballVisible READ ballVisible NOTIFY settingsChanged)
    Q_PROPERTY(double ballX READ ballX NOTIFY ballPosChanged)
    Q_PROPERTY(double ballY READ ballY NOTIFY ballPosChanged)
    Q_PROPERTY(bool onboardingDone READ onboardingDone WRITE setOnboardingDone NOTIFY settingsChanged)
    Q_PROPERTY(QString updateResult READ updateResult NOTIFY updateResultChanged)
    Q_PROPERTY(int theme READ theme WRITE setTheme NOTIFY settingsChanged)
    Q_PROPERTY(bool darkTheme READ darkTheme NOTIFY settingsChanged)
    Q_PROPERTY(QString windowColor READ windowColor NOTIFY settingsChanged)
    Q_PROPERTY(QString cardColor READ cardColor NOTIFY settingsChanged)
    Q_PROPERTY(QString borderColor READ borderColor NOTIFY settingsChanged)
    Q_PROPERTY(QString textColor READ textColor NOTIFY settingsChanged)
    Q_PROPERTY(QString secondaryTextColor READ secondaryTextColor NOTIFY settingsChanged)
    Q_PROPERTY(int language READ language WRITE setLanguage NOTIFY settingsChanged)
    Q_PROPERTY(double temperature READ temperature WRITE setTemperature NOTIFY settingsChanged)
    Q_PROPERTY(int maxTokens READ maxTokens WRITE setMaxTokens NOTIFY settingsChanged)

public:
    explicit Bridge(QObject *parent = nullptr);
    ~Bridge() override;

    bool petVisible() const { return m_petVisible; }
    void setPetVisible(bool visible);
    bool chatVisible() const { return m_chatVisible; }
    void setChatVisible(bool visible);
    bool managerVisible() const { return m_managerVisible; }
    void setManagerVisible(bool visible);
    bool bubbleVisible() const { return m_bubbleVisible; }
    void setBubbleVisible(bool visible);
    QString bubbleText() const { return m_bubbleText; }
    double bubbleX() const { return m_bubbleX; }
    double bubbleY() const { return m_bubbleY; }
    double petX() const { return m_petX; }
    double petY() const { return m_petY; }
    bool hasSavedPos() const { return m_hasSavedPos; }
    QString statusText() const { return m_statusText; }
    bool busy() const;
    bool autoStart() const;
    void setAutoStart(bool enabled);

    QString personaName() const;
    QString modelSummary() const;
    bool llmReady() const;
    QVariantList personas() const;
    QVariantList characters() const;
    QVariantList worlds() const;
    QString currentCharacterId() const;
    QString currentCharacterName() const;
    QString currentPersonaId() const;
    int managerPage() const { return m_managerPage; }
    QVariantList memories() const;
    int intimacyScore() const;
    QString intimacyLevel() const;
    bool autoExtractMemories() const;
    void setAutoExtractMemories(bool enabled);
    int proactiveFrequency() const;
    void setProactiveFrequency(int frequency);
    bool idleMotion() const;
    void setIdleMotion(bool enabled);
    double idleAmplitude() const;
    void setIdleAmplitude(double amplitude);
    bool clickThrough() const;
    void setClickThrough(bool enabled);
    bool debugHitAreas() const;
    void setDebugHitAreas(bool enabled);
    bool edgeHide() const;
    void setEdgeHide(bool enabled);
    int mode() const;
    void setMode(int mode);
    bool ballVisible() const;
    void setBallVisible(bool visible);
    double ballX() const { return m_ballX; }
    double ballY() const { return m_ballY; }
    bool onboardingDone() const;
    void setOnboardingDone(bool done);
    QString updateResult() const { return m_updateResult; }
    int theme() const;
    void setTheme(int theme);
    bool darkTheme() const;
    QString windowColor() const;
    QString cardColor() const;
    QString borderColor() const;
    QString textColor() const;
    QString secondaryTextColor() const;
    int language() const;
    void setLanguage(int language);
    double temperature() const;
    void setTemperature(double temperature);
    int maxTokens() const;
    void setMaxTokens(int maxTokens);
    void setManagerPage(int page);
    QStringList presets() const;
    QString presetName() const;
    QString baseUrl() const;
    QString apiKey() const;
    QString modelName() const;
    QString testResult() const { return m_testResult; }
    QString appVersion() const;
    QString dataDir() const;
    QString logDir() const;

    Q_INVOKABLE void petClicked(int index, double x, double y, double width, double height);
    Q_INVOKABLE QVariantList pets() const;
    Q_INVOKABLE int petCount() const;
    Q_INVOKABLE void setPetPosition(int index, double x, double y);
    Q_INVOKABLE void setPetCount(int count);
    Q_INVOKABLE void setBallPosition(double x, double y);
    Q_INVOKABLE void toggleMuteMode();
    Q_INVOKABLE void checkUpdate();
    Q_INVOKABLE void restartApp();
    Q_INVOKABLE QString currentAvatarPath() const;
    Q_INVOKABLE QVariantList hitAreaRects() const;
    Q_INVOKABLE void cycleCharacter();
    Q_INVOKABLE void openChat();
    Q_INVOKABLE void closeChat();
    Q_INVOKABLE void sendChat(const QString &text);
    Q_INVOKABLE void togglePet();
    Q_INVOKABLE void showPetMenu(int index = 0);
    Q_INVOKABLE void quitApp();
    Q_INVOKABLE void notifyAlreadyRunning();

    Q_INVOKABLE void openManager();
    Q_INVOKABLE void closeManager();
    Q_INVOKABLE void saveLlmSettings(const QString &preset, const QString &baseUrl, const QString &apiKey,
                                     const QString &model);
    Q_INVOKABLE void testLlmConnection(const QString &baseUrl, const QString &apiKey, const QString &model);
    Q_INVOKABLE void importPersona();
    Q_INVOKABLE void removePersona(const QString &personaId);
    Q_INVOKABLE void selectPersona(const QString &personaId);
    Q_INVOKABLE void clearConversation();
    Q_INVOKABLE void stopReply();
    Q_INVOKABLE QVariantList initialMessages() const;

    /// 预设的默认 Base URL / 模型，供设置页在下拉切换时自动填充。
    Q_INVOKABLE QVariantMap presetDefaults(const QString &preset) const;
    Q_INVOKABLE void openDataFolder() const;
    Q_INVOKABLE void openLogFolder() const;

    // ---- 0.4 角色 / 人设 / 世界观 / 数据导出 ----
    Q_INVOKABLE void createCharacter(const QString &name);
    Q_INVOKABLE void renameCharacter(const QString &id, const QString &newName);
    Q_INVOKABLE void removeCharacter(const QString &id);
    Q_INVOKABLE void selectCharacter(const QString &id);
    Q_INVOKABLE void importAvatarFile();
    Q_INVOKABLE QVariantMap personaFields(const QString &personaId) const;
    Q_INVOKABLE void savePersonaFields(const QVariantMap &fields);
    Q_INVOKABLE void createPersona(const QString &name);
    Q_INVOKABLE void importPersonaFile();
    Q_INVOKABLE void createWorld(const QString &name);
    Q_INVOKABLE void renameWorld(const QString &id, const QString &newName);
    Q_INVOKABLE void removeWorld(const QString &id);
    Q_INVOKABLE void saveWorldBody(const QString &id, const QString &body);
    Q_INVOKABLE void importWorldFile();
    Q_INVOKABLE void assignCharacterWorld(const QString &worldId);
    Q_INVOKABLE void exportCharacterPack();
    Q_INVOKABLE void importCharacterPack();
    Q_INVOKABLE void exportChatHistory();
    Q_INVOKABLE void exportLogs();
    Q_INVOKABLE void addMemory(const QString &content);
    Q_INVOKABLE void updateMemory(qint64 id, const QString &content);
    Q_INVOKABLE void removeMemory(qint64 id);
    Q_INVOKABLE void toggleMemoryPin(qint64 id);
    Q_INVOKABLE void restartConversation();
    Q_INVOKABLE void retryChat();
    Q_INVOKABLE void backtrackToIndex(int index);

signals:
    void petVisibleChanged();
    void chatVisibleChanged();
    void managerVisibleChanged();
    void bubbleVisibleChanged();
    void bubbleTextChanged();
    void bubblePosChanged();
    void petPosChanged();
    void petLayoutChanged();
    void ballPosChanged();
    void updateResultChanged();
    void statusTextChanged();
    void settingsChanged();
    void personasChanged();
    void testResultChanged();
    void autoStartChanged();
    void managerPageChanged();
    void charactersChanged();
    void avatarPathChanged();
    void worldsChanged();
    void memoriesChanged();
    void intimacyChanged();

    void chatMessage(const QString &role, const QString &text);
    void chatDelta(const QString &text);
    void chatDone();
    void chatStateChanged(const QString &state);
    void chatError(const QString &message);
    void conversationCleared();

private:
    void setStatusText(const QString &text);
    void setTestResult(const QString &text);
    void positionBubble(double petX, double petY);
    QPointF petPosition(int index) const;
    void reportError(quint32 code, const QString &detail);
    void reportInfo(const QString &text);
    void refreshCharacters();
    void refreshWorlds();
    void applyCharacterContext();
    QString buildCharacterPrompt(const QString &characterId, const QString &personaId) const;
    QString currentCharacterDir() const;
    void ensureDefaultCharacter();
    void scheduleProactive();

    ConfigStore *m_config = nullptr;
    PersonaStore *m_personas = nullptr;
    ChatStore *m_store = nullptr;
    MemoryStore *m_memory = nullptr;
    IntimacyService *m_intimacy = nullptr;
    ChatService *m_chat = nullptr;
    CharacterStore *m_characters = nullptr;
    WorldStore *m_worlds = nullptr;
    QTimer m_proactiveTimer;
    QTimer m_bubbleTimer;
    bool m_petVisible = true;
    bool m_chatVisible = false;
    bool m_managerVisible = false;
    bool m_bubbleVisible = false;
    QString m_bubbleText;
    double m_bubbleX = 0;
    double m_bubbleY = 0;
    double m_petX = 0;
    double m_petY = 0;
    bool m_hasSavedPos = false;
    QString m_statusText;
    QString m_testResult;
    bool m_hasSavedPosition = false;
    int m_managerPage = 0;
    int m_activePetIndex = 0;
    double m_ballX = 0;
    double m_ballY = 0;
    QString m_updateResult;
    class QNetworkAccessManager *m_network = nullptr;
};

#endif // BRIDGE_H
