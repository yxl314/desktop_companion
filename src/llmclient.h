#ifndef LLMCLIENT_H
#define LLMCLIENT_H

#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

#include "sseparser.h"

class QNetworkReply;

/// OpenAI 兼容接口的最小客户端：流式 chat/completions 与 /models 连通性检查。
class LlmClient : public QObject
{
    Q_OBJECT

public:
    explicit LlmClient(QObject *parent = nullptr);

    void configure(const QString &baseUrl, const QString &apiKey, const QString &model);
    bool ready() const;

    /// 发起一次流式对话；systemPrompt 与人设对应，history 为历史消息数组。
    void start(const QString &systemPrompt, const QJsonArray &history, const QString &userText);
    void abort();

    /// 访问 GET {baseUrl}/models，用于设置页的「测试连接」。
    void testConnection();

signals:
    void delta(const QString &text);
    void finished();
    void failed(const QString &message);
    void testFinished(bool ok, const QString &message);

private:
    QNetworkRequest makeRequest(const QString &path) const;
    void handleReadyRead();
    void finalizeReply();
    void finishTest();

    QNetworkAccessManager m_network;
    QNetworkReply *m_reply = nullptr;
    QString m_baseUrl;
    QString m_apiKey;
    QString m_model;
    SseParser m_parser;
    bool m_aborted = false;
};

#endif // LLMCLIENT_H
