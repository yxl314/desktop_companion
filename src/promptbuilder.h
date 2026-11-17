#ifndef PROMPTBUILDER_H
#define PROMPTBUILDER_H

#include <QJsonArray>
#include <QString>
#include <QStringList>

/// 一次提示词组装所需的上下文；某个字段为空时对应段落整体省略。
struct PromptContext
{
    QString characterId;
    QString personaName;
    QString personaBody;
    QString worldBody;
    QString constraints;
    QStringList memories;
    QString situation;
};

/// 纯函数：按「身份 → 世界观 → 人设 → 回复约束 → 记忆 → 情境」固定顺序拼接 system prompt，
/// 并负责把 system、历史与当前输入组装成 OpenAI 兼容的 messages 数组。
class PromptBuilder
{
public:
    PromptBuilder() = delete;

    /// 拼接 system prompt；所有字段都为空时返回空字符串。
    static QString build(const PromptContext &context);

    /// 组装 messages：system（非空时）+ 历史原样 + user（非空时）。
    static QJsonArray buildMessages(const QString &systemPrompt, const QJsonArray &history,
                                   const QString &userText);

    /// 先用 build 生成 system prompt，再组装 messages。
    static QJsonArray buildMessages(const PromptContext &context, const QJsonArray &history,
                                    const QString &userText);

    /// 默认回复约束，供未单独配置约束时使用。
    static QString defaultConstraints();
};

#endif // PROMPTBUILDER_H
