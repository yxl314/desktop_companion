/**
 * \file src/services/sseparser.h
 * \brief OpenAI 兼容接口的 SSE 流增量解析。
 */

#ifndef SSEPARSER_H
#define SSEPARSER_H

#include <QByteArray>
#include <QList>
#include <QString>

/**
 * \brief 增量解析 OpenAI 兼容接口的 SSE 流，提取 choices[0].delta.content。
 *
 * 不依赖网络与界面，可独立单测。
 */
class SseParser
{
public:
    /**
     * \brief 追加收到的原始字节。
     * @param data 新收到的原始 SSE 字节。
     * @return 本次新增的文本片段列表。
     */
    QList<QString> feed(const QByteArray &data);

    /// 是否已经收到 data: [DONE]。
    bool done() const { return m_done; }

    /**
     * \brief 重置解析状态（缓冲区与 [DONE] 标记）。
     */
    void reset();

private:
    QByteArray m_buffer;        ///< 未消费的字节缓冲。
    bool m_done = false;        ///< 是否已收到 [DONE]。
    QList<QString> m_pending;   ///< 尚未取走的解析结果。
};

#endif // SSEPARSER_H
