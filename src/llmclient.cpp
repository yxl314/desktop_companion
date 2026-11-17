#include "llmclient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUuid>

namespace
{
QString trimBaseUrl(const QString &baseUrl)
{
    QString value = baseUrl.trimmed();
    while (value.endsWith(QLatin1Char('/')))
        value.chop(1);
    return value;
}

QString httpErrorMessage(int status, const QString &body)
{
    QString detail = body.trimmed();
    if (detail.size() > 200)
        detail = detail.left(200) + QStringLiteral("…");
    if (detail.isEmpty())
        return QStringLiteral("HTTP %1").arg(status);
    return QStringLiteral("HTTP %1：%2").arg(status).arg(detail);
}
} // namespace

LlmClient::LlmClient(QObject *parent)
    : QObject(parent)
{
}

void LlmClient::configure(const QString &baseUrl, const QString &apiKey, const QString &model)
{
    m_baseUrl = trimBaseUrl(baseUrl);
    m_apiKey = apiKey.trimmed();
    m_model = model.trimmed();
}

bool LlmClient::ready() const
{
    return !m_baseUrl.isEmpty() && !m_model.isEmpty();
}

QNetworkRequest LlmClient::makeRequest(const QString &path) const
{
    QNetworkRequest request(QUrl(m_baseUrl + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_apiKey.isEmpty())
        request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_apiKey.toUtf8());
    // OpenCode Go 网关按 session 路由；普通 OpenAI 兼容服务会忽略未知头。
    if (m_baseUrl.contains(QLatin1String("opencode.ai")))
        request.setRawHeader("x-opencode-session",
                             QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8());
    request.setTransferTimeout(60000);
    return request;
}

void LlmClient::start(const QString &systemPrompt, const QJsonArray &history, const QString &userText)
{
    if (m_reply != nullptr || !ready())
        return;

    QJsonArray messages;
    if (!systemPrompt.trimmed().isEmpty()) {
        messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("system")},
                                    {QStringLiteral("content"), systemPrompt}});
    }
    for (const QJsonValue value : history)
        messages.append(value);
    messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                {QStringLiteral("content"), userText}});

    const QJsonObject body{{QStringLiteral("model"), m_model},
                           {QStringLiteral("stream"), true},
                           {QStringLiteral("messages"), messages}};

    m_parser.reset();
    m_aborted = false;
    m_reply = m_network.post(makeRequest(QStringLiteral("/chat/completions")),
                             QJsonDocument(body).toJson(QJsonDocument::Compact));

    connect(m_reply, &QNetworkReply::readyRead, this, &LlmClient::handleReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &LlmClient::finalizeReply);
}

void LlmClient::handleReadyRead()
{
    if (m_reply == nullptr)
        return;
    const QList<QString> deltas = m_parser.feed(m_reply->readAll());
    for (const QString &delta : deltas)
        emit this->delta(delta);

    // 有些兼容服务发完 [DONE] 仍保持连接（HTTP keep-alive），
    // 不等 socket 关闭，收到 [DONE] 就按完成处理。
    if (m_parser.done()) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
        m_aborted = false;
        emit finished();
    }
}

void LlmClient::finalizeReply()
{
    if (m_reply == nullptr)
        return;

    const QByteArray rest = m_reply->readAll();
    if (!rest.isEmpty()) {
        const QList<QString> deltas = m_parser.feed(rest);
        for (const QString &delta : deltas)
            emit this->delta(delta);
    }

    const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError error = m_reply->error();
    const QString errorString = m_reply->errorString();
    const QString body = QString::fromUtf8(rest);
    const bool aborted = m_aborted;

    m_reply->deleteLater();
    m_reply = nullptr;
    m_aborted = false;

    if (aborted)
        return;
    if (error != QNetworkReply::NoError) {
        emit failed(QStringLiteral("网络错误：%1").arg(errorString));
        return;
    }
    if (status >= 400) {
        emit failed(httpErrorMessage(status, body));
        return;
    }
    emit finished();
}

void LlmClient::abort()
{
    if (m_reply == nullptr)
        return;
    m_aborted = true;
    m_reply->abort();
}

void LlmClient::testConnection()
{
    if (m_reply != nullptr)
        return;
    if (m_baseUrl.isEmpty()) {
        emit testFinished(false, QStringLiteral("请先填写 Base URL"));
        return;
    }
    m_reply = m_network.get(makeRequest(QStringLiteral("/models")));
    connect(m_reply, &QNetworkReply::finished, this, &LlmClient::finishTest);
}

void LlmClient::finishTest()
{
    if (m_reply == nullptr)
        return;

    const QByteArray body = m_reply->readAll();
    const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError error = m_reply->error();
    const QString errorString = m_reply->errorString();

    m_reply->deleteLater();
    m_reply = nullptr;

    if (error != QNetworkReply::NoError) {
        emit testFinished(false, QStringLiteral("连接失败：%1").arg(errorString));
        return;
    }
    if (status >= 400) {
        emit testFinished(false, httpErrorMessage(status, QString::fromUtf8(body)));
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(body);
    const QJsonArray data = document.object().value(QStringLiteral("data")).toArray();
    if (!data.isEmpty())
        emit testFinished(true, QStringLiteral("连接成功，可用模型 %1 个").arg(data.size()));
    else
        emit testFinished(true, QStringLiteral("连接成功"));
}
