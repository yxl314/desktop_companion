#include "bridge.h"

#include <QApplication>
#include <QCursor>
#include <QMenu>
#include <QRandomGenerator>
#include <QSettings>

namespace {
const QList<QString> kIdleLines = {
    QStringLiteral("嗯？"),
    QStringLiteral("摸摸头～"),
    QStringLiteral("今天也要加油呀！"),
    QStringLiteral("要不要聊聊天？"),
    QStringLiteral("别戳啦，痒～"),
    QStringLiteral("我在呢。"),
};
}

Bridge::Bridge(QObject *parent)
    : QObject(parent)
{
    connect(&m_engine, &ChatEngine::delta, this, &Bridge::chatDelta);
    connect(&m_engine, &ChatEngine::done, this, [this] {
        setStatusText(QString());
        emit chatDone();
    });
    connect(&m_engine, &ChatEngine::stateChanged, this, [this](const QString &state) {
        if (state == QLatin1String("thinking"))
            setStatusText(QStringLiteral("思考中…"));
        else if (state == QLatin1String("streaming"))
            setStatusText(QStringLiteral("回复中…"));
        emit chatStateChanged(state);
    });

    m_bubbleTimer.setSingleShot(true);
    connect(&m_bubbleTimer, &QTimer::timeout, this, [this] { setBubbleVisible(false); });

    QSettings settings;
    const double x = settings.value(QStringLiteral("pet/x"), -1.0).toDouble();
    const double y = settings.value(QStringLiteral("pet/y"), -1.0).toDouble();
    if (x >= 0 && y >= 0) {
        m_petX = x;
        m_petY = y;
        m_hasSavedPos = true;
    }
}

void Bridge::setPetVisible(bool visible)
{
    if (m_petVisible == visible)
        return;
    m_petVisible = visible;
    emit petVisibleChanged();
}

void Bridge::setChatVisible(bool visible)
{
    if (m_chatVisible == visible)
        return;
    m_chatVisible = visible;
    emit chatVisibleChanged();
}

void Bridge::setBubbleVisible(bool visible)
{
    if (m_bubbleVisible == visible)
        return;
    m_bubbleVisible = visible;
    emit bubbleVisibleChanged();
}

void Bridge::setStatusText(const QString &text)
{
    if (m_statusText == text)
        return;
    m_statusText = text;
    emit statusTextChanged();
}

void Bridge::petClicked(double x, double y)
{
    m_bubbleText = kIdleLines.at(QRandomGenerator::global()->bounded(kIdleLines.size()));
    emit bubbleTextChanged();
    positionBubble(x, y);
    setBubbleVisible(true);
    m_bubbleTimer.start(4200);
    setPetPosition(x, y);
}

void Bridge::positionBubble(double x, double y)
{
    // 气泡默认在桌宠上方，贴顶时翻到下方。
    m_bubbleX = x + 40;
    m_bubbleY = (y - 110 < 10) ? y + 330 : y - 110;
    emit bubblePosChanged();
}

void Bridge::setPetPosition(double x, double y)
{
    m_petX = x;
    m_petY = y;
    m_hasSavedPos = true;
    emit petPosChanged();
    QSettings settings;
    settings.setValue(QStringLiteral("pet/x"), x);
    settings.setValue(QStringLiteral("pet/y"), y);
}

void Bridge::openChat()
{
    setChatVisible(true);
    setBubbleVisible(false);
}

void Bridge::closeChat()
{
    setChatVisible(false);
}

void Bridge::sendChat(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return;
    emit chatMessage(QStringLiteral("user"), trimmed);
    m_engine.send(trimmed);
}

void Bridge::togglePet()
{
    setPetVisible(!m_petVisible);
}

void Bridge::showPetMenu()
{
    QMenu menu;
    QAction *chatAction = menu.addAction(QStringLiteral("打开聊天"));
    QAction *hideAction = menu.addAction(QStringLiteral("隐藏桌宠"));
    menu.addSeparator();
    QAction *quitAction = menu.addAction(QStringLiteral("退出"));

    QAction *chosen = menu.exec(QCursor::pos());
    if (chosen == chatAction)
        openChat();
    else if (chosen == hideAction)
        setPetVisible(false);
    else if (chosen == quitAction)
        quitApp();
}

void Bridge::quitApp()
{
    QApplication::quit();
}
