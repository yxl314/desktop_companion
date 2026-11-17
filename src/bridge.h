#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include "chatengine.h"

// QML 与 C++ 的唯一桥接面：命令走 Q_INVOKABLE，事件走信号。
class Bridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool petVisible READ petVisible WRITE setPetVisible NOTIFY petVisibleChanged)
    Q_PROPERTY(bool chatVisible READ chatVisible WRITE setChatVisible NOTIFY chatVisibleChanged)
    Q_PROPERTY(bool bubbleVisible READ bubbleVisible WRITE setBubbleVisible NOTIFY bubbleVisibleChanged)
    Q_PROPERTY(QString bubbleText READ bubbleText NOTIFY bubbleTextChanged)
    Q_PROPERTY(double bubbleX READ bubbleX NOTIFY bubblePosChanged)
    Q_PROPERTY(double bubbleY READ bubbleY NOTIFY bubblePosChanged)
    Q_PROPERTY(double petX READ petX NOTIFY petPosChanged)
    Q_PROPERTY(double petY READ petY NOTIFY petPosChanged)
    Q_PROPERTY(bool hasSavedPos READ hasSavedPos CONSTANT)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit Bridge(QObject *parent = nullptr);

    bool petVisible() const { return m_petVisible; }
    void setPetVisible(bool visible);
    bool chatVisible() const { return m_chatVisible; }
    void setChatVisible(bool visible);
    bool bubbleVisible() const { return m_bubbleVisible; }
    void setBubbleVisible(bool visible);
    QString bubbleText() const { return m_bubbleText; }
    double bubbleX() const { return m_bubbleX; }
    double bubbleY() const { return m_bubbleY; }
    double petX() const { return m_petX; }
    double petY() const { return m_petY; }
    bool hasSavedPos() const { return m_hasSavedPos; }
    QString statusText() const { return m_statusText; }

    Q_INVOKABLE void petClicked(double x, double y);
    Q_INVOKABLE void setPetPosition(double x, double y);
    Q_INVOKABLE void openChat();
    Q_INVOKABLE void closeChat();
    Q_INVOKABLE void sendChat(const QString &text);
    Q_INVOKABLE void togglePet();
    Q_INVOKABLE void showPetMenu();
    Q_INVOKABLE void quitApp();

signals:
    void petVisibleChanged();
    void chatVisibleChanged();
    void bubbleVisibleChanged();
    void bubbleTextChanged();
    void bubblePosChanged();
    void petPosChanged();
    void statusTextChanged();

    void chatMessage(const QString &role, const QString &text);
    void chatDelta(const QString &text);
    void chatDone();
    void chatStateChanged(const QString &state);

private:
    void setStatusText(const QString &text);
    void positionBubble(double petX, double petY);

    ChatEngine m_engine;
    QTimer m_bubbleTimer;
    bool m_petVisible = true;
    bool m_chatVisible = false;
    bool m_bubbleVisible = false;
    QString m_bubbleText;
    double m_bubbleX = 0;
    double m_bubbleY = 0;
    double m_petX = 0;
    double m_petY = 0;
    bool m_hasSavedPos = false;
    QString m_statusText;
};
