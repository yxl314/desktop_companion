#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

// 本地 mock 对话引擎：不联网，按关键词返回台词，逐字流式输出。
class ChatEngine : public QObject
{
    Q_OBJECT
public:
    explicit ChatEngine(QObject *parent = nullptr);

    void send(const QString &text);

signals:
    void delta(const QString &text);
    void done();
    void stateChanged(const QString &state); // thinking / streaming / idle

private:
    void beginReply();
    void emitNextChunk();

    QString pickReply(const QString &text) const;

    QString m_reply;
    QString m_pending;
    int m_index = 0;
    QTimer m_thinkTimer;
    QTimer m_tickTimer;
};
