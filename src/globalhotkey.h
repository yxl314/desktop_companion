#ifndef GLOBALHOTKEY_H
#define GLOBALHOTKEY_H

#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QString>

/// Windows 全局快捷键：Ctrl+Alt+P 呼出/隐藏桌宠、Ctrl+Alt+C 打开聊天、
/// Ctrl+Alt+M 切换勿扰模式。非 Windows 平台为空实现。
class GlobalHotkey : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit GlobalHotkey(QObject *parent = nullptr);
    ~GlobalHotkey() override;

    bool registerDefaultHotkeys(QString *error = nullptr);
    void unregisterAll();

signals:
    void togglePetRequested();
    void openChatRequested();
    void toggleMuteRequested();

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    QList<int> m_ids;
};

#endif // GLOBALHOTKEY_H
