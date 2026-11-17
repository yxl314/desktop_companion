#include "promptbuilder.h"

#include <QJsonObject>
#include <QJsonValue>

namespace
{
void appendSection(QStringList *sections, const QString &header, const QString &body)
{
    const QString text = body.trimmed();
    if (text.isEmpty())
        return;
    sections->append(header + QLatin1Char('\n') + text);
}
} // namespace

QString PromptBuilder::build(const PromptContext &context)
{
    // 顺序即契约：身份 → 世界观 → 人设 → 回复约束 → 记忆 → 情境。
    QStringList sections;

    const QString personaName = context.personaName.trimmed();
    const QString characterId = context.characterId.trimmed();
    QString identity;
    if (!personaName.isEmpty())
        identity = QStringLiteral("你是「%1」，请始终保持这个身份与语气。").arg(personaName);
    else if (!characterId.isEmpty())
        identity = QStringLiteral("你是角色 %1，请始终保持这个角色的身份与语气。").arg(characterId);
    appendSection(&sections, QStringLiteral("【身份】"), identity);

    appendSection(&sections, QStringLiteral("【世界观】"), context.worldBody);
    appendSection(&sections, QStringLiteral("【人设】"), context.personaBody);
    appendSection(&sections, QStringLiteral("【回复约束】"), context.constraints);

    QStringList memoryLines;
    for (const QString &memory : context.memories) {
        const QString text = memory.trimmed();
        if (!text.isEmpty())
            memoryLines.append(QStringLiteral("- ") + text);
    }
    appendSection(&sections, QStringLiteral("【记忆】"), memoryLines.join(QLatin1Char('\n')));

    appendSection(&sections, QStringLiteral("【情境】"), context.situation);

    return sections.join(QStringLiteral("\n\n"));
}

QJsonArray PromptBuilder::buildMessages(const QString &systemPrompt, const QJsonArray &history,
                                        const QString &userText)
{
    QJsonArray messages;

    const QString system = systemPrompt.trimmed();
    if (!system.isEmpty()) {
        messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("system")},
                                    {QStringLiteral("content"), system}});
    }
    for (const QJsonValue message : history)
        messages.append(message);

    if (!userText.trimmed().isEmpty()) {
        messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                    {QStringLiteral("content"), userText}});
    }
    return messages;
}

QJsonArray PromptBuilder::buildMessages(const PromptContext &context, const QJsonArray &history,
                                        const QString &userText)
{
    return buildMessages(build(context), history, userText);
}

QString PromptBuilder::defaultConstraints()
{
    return QStringLiteral("用简体中文自然回复，像聊天而不是写作文；一次回复不超过三句话，除非用户明确要求展开；"
                          "不知道的事不要编造。");
}
