/**
 * \file src/services/chatservice.h
 * \brief 对话服务：LLM 优先、未配置时回退 mock，并负责历史、记忆注入与好感度。
 */

#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVariantList>

#include "services/chatengine.h"
#include "services/llmclient.h"
#include "services/promptbuilder.h"

class CharacterStore;
class ChatStore;
class ConfigStore;
class IntimacyService;
class MemoryStore;
class PersonaStore;
class WorldStore;

/**
 * \brief 对话服务：LLM 优先、未配置回退 mock。
 *
 * 负责历史读取、提示词组装、记忆注入、好感度更新与自动记忆抽取；
 * 通过信号把流式结果回传给上层，不感知界面。
 */
class ChatService : public QObject
{
    Q_OBJECT

public:
    /**
     * \brief 构造对话服务。
     * @param config 配置仓库（模型参数来源）。
     * @param personas 全局人设仓库。
     * @param store 聊天记录仓库。
     * @param memory 记忆仓库。
     * @param intimacy 好感度仓库。
     * @param parent 父对象。
     */
    ChatService(ConfigStore *config, PersonaStore *personas, ChatStore *store,
                MemoryStore *memory, IntimacyService *intimacy, QObject *parent = nullptr);

    /// 真实模型是否已就绪。
    bool llmReady() const { return m_llm.ready(); }
    /// 是否有进行中的对话。
    bool busy() const { return m_active; }

    /**
     * \brief 发送一条用户消息。
     * @param text 用户输入文本。
     */
    void send(const QString &text);
    /**
     * \brief 停止当前生成。
     */
    void stop();
    /**
     * \brief 清空当前会话（消息与记忆分区由上层决定是否重置）。
     */
    void clearConversation();
    /**
     * \brief 重启对话：清空当前会话并重置分区。
     */
    void restartConversation();
    /**
     * \brief 重试上一条用户输入。
     */
    void retryLast();
    /// 立即对最近一轮对话触发记忆抽取（异步，完成后发 memoriesChanged）。
    void extractMemoriesNow();
    /**
     * \brief 回溯到指定消息（删除其后的消息）。
     * @param messageId 保留的最后一条消息 id。
     */
    void backtrackToMessage(qint64 messageId);
    /**
     * \brief 按消息下标回溯（删除该下标之后的消息）。
     * @param index 保留的最后一条消息下标。
     */
    void backtrackToIndex(int index);

    /**
     * \brief 设置对话上下文：分区键、角色/人设（记忆分区）与 system prompt 基础上下文。
     * @param partitionKey 分区键。
     * @param characterId 角色 id。
     * @param personaId 人设 id。
     * @param baseContext 提示词基础上下文。
     */
    void setContext(const QString &partitionKey, const QString &characterId, const QString &personaId,
                    const PromptContext &baseContext);
    /// 当前分区键。
    QString partitionKey() const { return m_partitionKey; }
    /// 当前角色 id。
    QString characterId() const { return m_characterId; }
    /// 当前人设 id。
    QString personaId() const { return m_personaId; }

    /**
     * \brief 设置同世界观角色摘要来源（WLD-L-01 同世界观角色互知）。
     *
     * 接线后每次组装 system prompt 时，按当前角色 meta.json 的 worldId
     * 收集同世界观下其他已启用角色的摘要（「名字（一句话人设）」），
     * 由 PromptBuilder 注入「同世界观的伙伴」段；未接线或为空时保持 1.0 行为。
     * @param characters 角色仓库；传 nullptr 关闭注入。
     * @param worlds 世界观仓库，用于确认世界观目录仍存在；可为 nullptr。
     */
    void setWorldPeerSources(CharacterStore *characters, WorldStore *worlds = nullptr);

    /**
     * \brief 当前会话的初始消息（供界面重建历史）。
     * @return 消息行列表（role/content 等字段）。
     */
    QVariantList initialMessages() const;

    /**
     * \brief 用给定连接参数做一次 /models 连通性检查。
     * @param baseUrl 接口根地址。
     * @param apiKey 接口密钥。
     * @param model 模型名。
     */
    void testConnection(const QString &baseUrl, const QString &apiKey, const QString &model);

signals:
    /// 用户消息已记录。
    void userMessage(const QString &text);
    /// 收到一段增量回复文本。
    void delta(const QString &text);
    /// 本次回复正常结束。
    void done();
    /// 对话失败，携带可读原因。
    void failed(const QString &message);
    /// 对话状态变化：thinking / streaming / idle。
    void stateChanged(const QString &state);
    /// 当前会话被清空。
    void conversationCleared();
    /// 记忆发生变化。
    void memoriesChanged();
    /// 好感度发生变化。
    void intimacyChanged();
    /// 助手消息已落盘。
    void assistantStored(qint64 messageId);
    /// 连通性检查结束。
    void testFinished(bool ok, const QString &message);

private:
    enum class Source
    {
        None, ///< 无进行中的回复。
        Mock, ///< 本地 mock 引擎。
        Llm   ///< 真实模型。
    };

    void appendHistory(const QString &role, const QString &content);
    qint64 appendStored(const QString &role, const QString &content);
    void startLlm(const QString &userText);
    void finishExchange();
    QString buildPrompt(const QString &userText) const;
    void resetPartition(bool resetMemory);
    void startExtraction(const QString &userText, const QString &assistantText);
    QStringList collectWorldPeers() const;

    ConfigStore *m_config = nullptr;   ///< 配置仓库。
    PersonaStore *m_personas = nullptr; ///< 人设仓库。
    ChatStore *m_store = nullptr;      ///< 聊天记录仓库。
    MemoryStore *m_memory = nullptr;   ///< 记忆仓库。
    IntimacyService *m_intimacy = nullptr; ///< 好感度仓库。
    CharacterStore *m_characters = nullptr; ///< 同世界观伙伴来源（可选）。
    WorldStore *m_worlds = nullptr;    ///< 世界观仓库（可选，用于校验世界观存在）。
    ChatEngine m_mock;                 ///< 本地 mock 引擎。
    LlmClient m_llm;                   ///< 对话用模型客户端。
    LlmClient m_extractor;             ///< 记忆抽取用模型客户端。
    QList<QPair<QString, QString>> m_history; ///< 本轮上下文历史（role, content）。
    QString m_partial;                 ///< 流式回复的已累积文本。
    QString m_lastUserText;            ///< 上一条用户输入（重试用）。
    QString m_extractBuffer;           ///< 记忆抽取的累积输入。
    QString m_partitionKey;            ///< 当前记忆分区键。
    QString m_characterId;             ///< 当前角色 id。
    QString m_personaId;               ///< 当前人设 id。
    PromptContext m_baseContext;       ///< 提示词基础上下文。
    Source m_source = Source::None;    ///< 当前回复来源。
    bool m_active = false;             ///< 是否有进行中的回复。
    bool m_contextInitialized = false; ///< 上下文是否已初始化。
    bool m_extracting = false;         ///< 是否正在抽取记忆。
};

#endif // CHATSERVICE_H
