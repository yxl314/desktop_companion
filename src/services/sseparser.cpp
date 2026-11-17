#include "services/sseparser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
QString extractDelta(const QByteArray &payload)
{
    const QJsonDocument document = QJsonDocument::fromJson(payload);
    if (!document.isObject())
        return {};
    const QJsonObject root = document.object();
    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty())
        return {};
    const QJsonObject choice = choices.at(0).toObject();
    const QJsonObject delta = choice.value(QStringLiteral("delta")).toObject();
    return delta.value(QStringLiteral("content")).toString();
}
} // namespace

QList<QString> SseParser::feed(const QByteArray &data)
{
    QList<QString> deltas;
    m_buffer.append(data);

    for (;;) {
        const int newline = m_buffer.indexOf('\n');
        if (newline < 0)
            break;

        QByteArray line = m_buffer.left(newline).trimmed();
        m_buffer.remove(0, newline + 1);

        if (!line.startsWith("data:"))
            continue;
        line = line.mid(5).trimmed();
        if (line.isEmpty())
            continue;
        if (line == "[DONE]") {
            m_done = true;
            continue;
        }

        const QString text = extractDelta(line);
        if (!text.isEmpty())
            deltas.append(text);
    }
    return deltas;
}

void SseParser::reset()
{
    m_buffer.clear();
    m_done = false;
}
