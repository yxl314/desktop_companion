#ifndef CHATENGINE_H
#define CHATENGINE_H

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

    /// 中止正在进行的回复；没有进行中的回复时为空操作。
    void cancel();

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
    bool m_active = false;
};

#endif // CHATENGINE_H
