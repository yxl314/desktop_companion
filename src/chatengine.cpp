#include "chatengine.h"

#include <QRandomGenerator>

ChatEngine::ChatEngine(QObject *parent)
    : QObject(parent)
{
    m_thinkTimer.setSingleShot(true);
    connect(&m_thinkTimer, &QTimer::timeout, this, &ChatEngine::beginReply);

    m_tickTimer.setInterval(18);
    connect(&m_tickTimer, &QTimer::timeout, this, &ChatEngine::emitNextChunk);
}

void ChatEngine::send(const QString &text)
{
    m_active = true;
    m_pending = text;
    m_reply = pickReply(text);
    m_index = 0;
    emit stateChanged(QStringLiteral("thinking"));
    m_thinkTimer.start(350);
}

void ChatEngine::cancel()
{
    if (!m_active)
        return;
    m_thinkTimer.stop();
    m_tickTimer.stop();
    m_active = false;
    emit stateChanged(QStringLiteral("idle"));
    emit done();
}

void ChatEngine::beginReply()
{
    emit stateChanged(QStringLiteral("streaming"));
    m_tickTimer.start();
}

void ChatEngine::emitNextChunk()
{
    if (m_index >= m_reply.size()) {
        m_tickTimer.stop();
        m_active = false;
        emit stateChanged(QStringLiteral("idle"));
        emit done();
        return;
    }
    const int n = qMin(2, m_reply.size() - m_index);
    const QString chunk = m_reply.mid(m_index, n);
    m_index += n;
    emit delta(chunk);
}

QString ChatEngine::pickReply(const QString &text) const
{
    const QString t = text.trimmed();
    if (t.contains("你好") || t.contains("hi", Qt::CaseInsensitive) || t.contains("hello", Qt::CaseInsensitive))
        return QStringLiteral("你好呀，我是桌面上的小柚，今天想玩点什么？");
    if (t.contains("名字") || t.contains("你是谁"))
        return QStringLiteral("我叫小柚，是一只住在你桌面上的小精灵。");
    if (t.contains("心情") || t.contains("开心") || t.contains("累"))
        return QStringLiteral("累了就休息一下吧，我会一直在这里陪着你。");
    if (t.contains("游戏"))
        return QStringLiteral("游戏的事我懂一点，想聊哪个游戏？");
    if (t.contains("吃") || t.contains("饿"))
        return QStringLiteral("我不饿，不过你要是吃东西，记得让我看着。");
    if (t.contains("再见") || t.contains("拜拜"))
        return QStringLiteral("别走嘛，再陪我聊两句。");

    static const QList<QString> defaults = {
        QStringLiteral("嗯嗯，我听着呢。"),
        QStringLiteral("这个我不太懂，但你说得很有意思。"),
        QStringLiteral("然后呢？"),
        QStringLiteral("我觉得你说得对。"),
        QStringLiteral("有我陪着你呢，慢慢说。"),
    };
    return defaults.at(QRandomGenerator::global()->bounded(defaults.size()));
}
