/**
 * \file src/services/promptbuilder.h
 * \brief 提示词组装：按固定顺序拼接 system prompt 并生成 messages 数组。
 */

#ifndef PROMPTBUILDER_H
#define PROMPTBUILDER_H

#include <QJsonArray>
#include <QString>
#include <QStringList>

/**
 * \brief 一次提示词组装所需的上下文；某个字段为空时对应段落整体省略。
 */
struct PromptContext
{
    QString characterId;  ///< 角色 id。
    QString personaName;  ///< 人设名。
    QString personaBody;  ///< 人设正文。
    QString worldBody;    ///< 世界观正文。
    QStringList worldPeers; ///< 同世界观其他角色摘要（「名字（一句话人设）」），追加在世界观段之后。
    QString constraints;  ///< 回复约束。
    QStringList memories; ///< 注入的记忆条目。
    QString situation;    ///< 当前情境描述。
};

/**
 * \brief 纯函数工具：按「身份 → 世界观 → 同世界观的伙伴 → 人设 → 回复约束 → 记忆 → 情境」
 *        固定顺序拼接 system prompt，并负责组装 OpenAI 兼容的 messages 数组。
 */
class PromptBuilder
{
public:
    PromptBuilder() = delete;

    /**
     * \brief 拼接 system prompt。
     * @param context 提示词上下文。
     * @return system prompt；所有字段都为空时返回空字符串。
     */
    static QString build(const PromptContext &context);

    /**
     * \brief 组装 messages 数组。
     * @param systemPrompt system 提示词；非空时放在最前。
     * @param history 历史消息数组，原样追加。
     * @param userText 本轮用户输入；非空时追加在最后。
     * @return OpenAI 兼容的 messages 数组。
     */
    static QJsonArray buildMessages(const QString &systemPrompt, const QJsonArray &history,
                                   const QString &userText);

    /**
     * \brief 先用 build() 生成 system prompt，再组装 messages。
     * @param context 提示词上下文。
     * @param history 历史消息数组。
     * @param userText 本轮用户输入。
     * @return OpenAI 兼容的 messages 数组。
     */
    static QJsonArray buildMessages(const PromptContext &context, const QJsonArray &history,
                                    const QString &userText);

    /**
     * \brief 默认回复约束，供未单独配置约束时使用。
     * @return 默认约束文本。
     */
    static QString defaultConstraints();
};

#endif // PROMPTBUILDER_H
