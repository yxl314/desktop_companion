/**
 * \file src/services/llmclient.h
 * \brief OpenAI 兼容接口客户端：流式 chat/completions 与 /models 连通性检查。
 */

#ifndef LLMCLIENT_H
#define LLMCLIENT_H

#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

#include "services/sseparser.h"

class QNetworkReply;

/**
 * \brief OpenAI 兼容接口的最小客户端。
 *
 * 负责流式对话请求、增量文本转发与 /models 连通性检查；
 * 不处理提示词组装与历史持久化（由 PromptBuilder / ChatStore 负责）。
 */
class LlmClient : public QObject
{
    Q_OBJECT

public:
    /**
     * \brief 构造客户端。
     * @param parent 父对象。
     */
    explicit LlmClient(QObject *parent = nullptr);

    /**
     * \brief 配置接口地址、密钥、模型与生成参数。
     * @param baseUrl 接口根地址。
     * @param apiKey 接口密钥。
     * @param model 模型名。
     * @param temperature 采样温度。
     * @param maxTokens 最大回复 token 数，0 表示默认。
     */
    void configure(const QString &baseUrl, const QString &apiKey, const QString &model,
                   double temperature = 0.8, int maxTokens = 0);
    /**
     * \brief 仅更新生成参数。
     * @param temperature 采样温度。
     * @param maxTokens 最大回复 token 数，0 表示默认。
     */
    void setGenerationOptions(double temperature, int maxTokens);
    /// 是否已配置好可用连接（地址、模型非空）。
    bool ready() const;

    /**
     * \brief 发起一次流式对话。
     * @param systemPrompt system 提示词，与人设对应。
     * @param history 历史消息数组。
     * @param userText 本轮用户输入。
     */
    void start(const QString &systemPrompt, const QJsonArray &history, const QString &userText);
    /**
     * \brief 中止正在进行的请求；没有进行中的请求时为空操作。
     */
    void abort();

    /**
     * \brief 访问 GET {baseUrl}/models，用于设置页的「测试连接」。
     */
    void testConnection();

signals:
    /// 收到一段增量文本。
    void delta(const QString &text);
    /// 本次流式对话正常结束。
    void finished();
    /// 请求失败，携带可读原因。
    void failed(const QString &message);
    /// 连通性检查结束。
    void testFinished(bool ok, const QString &message);

private:
    QNetworkRequest makeRequest(const QString &path) const;
    void handleReadyRead();
    void finalizeReply();
    void finishTest();

    QNetworkAccessManager m_network; ///< 网络访问管理器。
    QNetworkReply *m_reply = nullptr; ///< 当前流式请求。
    QString m_baseUrl;               ///< 接口根地址。
    QString m_apiKey;                ///< 接口密钥。
    QString m_model;                 ///< 模型名。
    double m_temperature = 0.8;      ///< 采样温度。
    int m_maxTokens = 0;             ///< 最大回复 token 数。
    SseParser m_parser;              ///< SSE 增量解析器。
    bool m_aborted = false;          ///< 是否由用户主动中止。
};

#endif // LLMCLIENT_H
