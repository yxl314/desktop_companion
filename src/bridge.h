#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QTimer>

class ChatService;
class ChatStore;
class ConfigStore;
class PersonaStore;

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
    QStringList presets() const;
    QString presetName() const;
    QString baseUrl() const;
    QString apiKey() const;
    QString modelName() const;
    QString testResult() const { return m_testResult; }
    QString appVersion() const;
    QString dataDir() const;
    QString logDir() const;

    Q_INVOKABLE void petClicked(double x, double y, double height);
    Q_INVOKABLE void setPetPosition(double x, double y);
    Q_INVOKABLE void openChat();
    Q_INVOKABLE void closeChat();
    Q_INVOKABLE void sendChat(const QString &text);
    Q_INVOKABLE void togglePet();
    Q_INVOKABLE void showPetMenu();
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

signals:
    void petVisibleChanged();
    void chatVisibleChanged();
    void managerVisibleChanged();
    void bubbleVisibleChanged();
    void bubbleTextChanged();
    void bubblePosChanged();
    void petPosChanged();
    void statusTextChanged();
    void settingsChanged();
    void personasChanged();
    void testResultChanged();
    void autoStartChanged();

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

    ConfigStore *m_config = nullptr;
    PersonaStore *m_personas = nullptr;
    ChatStore *m_store = nullptr;
    ChatService *m_chat = nullptr;
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
};

#endif // BRIDGE_H
