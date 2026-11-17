/**
 * \file src/services/chatengine.h
 * \brief 本地 mock 对话引擎：不联网，按关键词返回台词并逐字流式输出。
 */

#ifndef CHATENGINE_H
#define CHATENGINE_H

#include <QObject>
#include <QString>
#include <QTimer>

/**
 * \brief 本地 mock 对话引擎。
 *
 * 未配置真实模型时的回退实现：按关键词选台词，用定时器逐字输出，
 * 供对话链路在离线状态下保持完整行为。
 */
class ChatEngine : public QObject
{
    Q_OBJECT
public:
    /**
     * \brief 构造 mock 引擎。
     * @param parent 父对象。
     */
    explicit ChatEngine(QObject *parent = nullptr);

    /**
     * \brief 发送一条用户输入并开始生成回复。
     * @param text 用户输入文本。
     */
    void send(const QString &text);

    /**
     * \brief 中止正在进行的回复；没有进行中的回复时为空操作。
     */
    void cancel();

signals:
    /// 收到一段增量文本。
    void delta(const QString &text);
    /// 本次回复结束。
    void done();
    /// 引擎状态变化：thinking / streaming / idle。
    void stateChanged(const QString &state);

private:
    void beginReply();
    void emitNextChunk();

    QString pickReply(const QString &text) const;

    QString m_reply;       ///< 本次待输出的完整台词。
    QString m_pending;     ///< 待发送的用户输入。
    int m_index = 0;       ///< 当前输出到的字符下标。
    QTimer m_thinkTimer;   ///< 「思考中」延时定时器。
    QTimer m_tickTimer;    ///< 逐字输出定时器。
    bool m_active = false; ///< 是否有进行中的回复。
};

#endif // CHATENGINE_H
