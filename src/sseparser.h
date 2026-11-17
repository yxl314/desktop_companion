#ifndef SSEPARSER_H
#define SSEPARSER_H

#include <QByteArray>
#include <QList>
#include <QString>

/// 增量解析 OpenAI 兼容接口的 SSE 流，提取 choices[0].delta.content。
class SseParser
{
public:
    /// 追加收到的原始字节；返回本次新增的文本片段。
    QList<QString> feed(const QByteArray &data);

    /// 是否已经收到 data: [DONE]。
    bool done() const { return m_done; }

    void reset();

private:
    QByteArray m_buffer;
    bool m_done = false;
    QList<QString> m_pending;
};

#endif // SSEPARSER_H
