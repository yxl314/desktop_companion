/**
 * \file src/viewmodels/chatviewmodel.h
 * \brief 聊天 ViewModel：消息增量合并、卡片解析、点赞点踩与对话生命周期。
 *
 * ChatViewModel 持有 ChatService / ChatStore / MemoryStore / IntimacyService 等模型层
 * 引用，负责把流式增量合并进 \c messages 列表后节流发出 messagesChanged，
 * 并在此层完成卡片解析、评价持久化、回溯/重启/清空/导出等交互。
 * 本类不依赖任何 QML 或窗口类，可脱离 QML 实例化测试。
 */

#ifndef CHATVIEWMODEL_H
#define CHATVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

#include "viewmodelbase.h"

class CharacterStore;
class ChatService;
class ChatStore;
class ConfigStore;
class IntimacyService;
class MemoryStore;
class WorldStore;

/**
 * \brief 聊天页 ViewModel。
 *
 * \c messages 的每一行是 QVariantMap，字段固定为
 * id / role / text / feedback / card / cardTitle / cardBody / expanded / time。
 * 流式回复期间增量文本先合并到内存列表，再由 30ms 节流器统一发出 messagesChanged，
 * 避免逐 token 触发整表重绘。
 */
class ChatViewModel : public ViewModelBase
{
    Q_OBJECT
    /** \brief 当前会话消息列表（行字段见类注释）。 */
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    /** \brief 是否正在等待/接收模型回复。 */
    Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)
    /** \brief 聊天标题，如「小柚 · 亲近」。 */
    Q_PROPERTY(QString chatTitle READ chatTitle NOTIFY chatTitleChanged)
    /** \brief 模型摘要，如「DeepSeek · deepseek-chat」或「本地 mock（未配置模型）」。 */
    Q_PROPERTY(QString modelSummary READ modelSummary NOTIFY modelSummaryChanged)
    /** \brief 模型是否已配置（baseUrl 与 model 均非空）。 */
    Q_PROPERTY(bool llmReady READ llmReady NOTIFY llmReadyChanged)
    /** \brief 空会话欢迎语。 */
    Q_PROPERTY(QString welcomeText READ welcomeText NOTIFY welcomeTextChanged)
    /** \brief 空会话推荐问句。 */
    Q_PROPERTY(QStringList quickPrompts READ quickPrompts CONSTANT)
    /** \brief 当前角色好感度分数（0~100）。 */
    Q_PROPERTY(int intimacyScore READ intimacyScore NOTIFY intimacyScoreChanged)
    /** \brief 当前角色好感度等级名。 */
    Q_PROPERTY(QString intimacyLevel READ intimacyLevel NOTIFY intimacyLevelChanged)

public:
    /**
     * \brief 构造聊天 ViewModel 并接管对话上下文。
     * \param chat 对话服务，必须非空。
     * \param store 聊天记录仓库，必须非空。
     * \param memory 记忆仓库，必须非空。
     * \param intimacy 好感度仓库，必须非空。
     * \param config 应用配置仓库，必须非空。
     * \param characters 角色仓库，必须非空。
     * \param worlds 世界观仓库，必须非空。
     * \param parent 父对象。
     */
    explicit ChatViewModel(ChatService *chat, ChatStore *store, MemoryStore *memory,
                           IntimacyService *intimacy, ConfigStore *config,
                           CharacterStore *characters, WorldStore *worlds,
                           QObject *parent = nullptr);

    /// 析构函数。
    ~ChatViewModel() override;

    /// \brief 当前会话消息列表。
    QVariantList messages() const;
    /// \brief 是否正在流式接收回复。
    bool streaming() const;
    /// \brief 聊天标题。
    QString chatTitle() const;
    /// \brief 模型摘要。
    QString modelSummary() const;
    /// \brief 模型是否已配置。
    bool llmReady() const;
    /// \brief 空会话欢迎语。
    QString welcomeText() const;
    /// \brief 空会话推荐问句。
    QStringList quickPrompts() const;
    /// \brief 好感度分数。
    int intimacyScore() const;
    /// \brief 好感度等级名。
    QString intimacyLevel() const;

    /**
     * \brief 发送一条消息。
     * \param text 用户输入；空白输入会被拒绝并写入 errorText。
     */
    Q_INVOKABLE void send(const QString &text);
    /// 重试上一条用户消息。
    Q_INVOKABLE void retry();
    /// 停止当前回复。
    Q_INVOKABLE void stop();
    /// 重启对话：保留聊天记录，重置上下文与记忆。
    Q_INVOKABLE void restart();
    /// 清空当前会话消息。
    Q_INVOKABLE void clear();
    /**
     * \brief 回溯到指定消息，删除其后的消息与记忆。
     * \param index messages 列表下标。
     */
    Q_INVOKABLE void backtrack(int index);
    /**
     * \brief 为指定消息设置评价。
     * \param index messages 列表下标。
     * \param value 1 点赞、-1 点踩、0 取消。
     */
    Q_INVOKABLE void setFeedback(int index, int value);
    /**
     * \brief 展开/收起指定消息的卡片。
     * \param index messages 列表下标。
     */
    Q_INVOKABLE void toggleExpanded(int index);
    /// 导出当前聊天记录到 <AppData>/exports/。
    Q_INVOKABLE void exportChat();
    /// 请求 shell 打开模型设置页（附加信号 modelSettingsRequested）。
    Q_INVOKABLE void openModelSettings();

signals:
    /// messages 属性变化信号（流式期间节流 30ms）。
    void messagesChanged();
    /// streaming 属性变化信号。
    void streamingChanged();
    /// chatTitle 属性变化信号。
    void chatTitleChanged();
    /// modelSummary 属性变化信号。
    void modelSummaryChanged();
    /// llmReady 属性变化信号。
    void llmReadyChanged();
    /// welcomeText 属性变化信号。
    void welcomeTextChanged();
    /// intimacyScore 属性变化信号。
    void intimacyScoreChanged();
    /// intimacyLevel 属性变化信号。
    void intimacyLevelChanged();
    /// 请求 shell 切换到模型设置页（附加信号，契约外集成项）。
    void modelSettingsRequested();

private:
    /// 从 ChatService 重新加载当前会话消息（清空/回溯/重启/切换上下文后）。
    void reloadMessages();
    /**
     * \brief 追加一行消息。
     * \param role 角色：user / assistant。
     * \param text 正文。
     * \param id 数据库行 id，未落盘为 -1。
     * \param feedback 本地评价。
     */
    void appendRow(const QString &role, const QString &text, qint64 id, int feedback);
    /**
     * \brief 解析文本中的卡片块并写回指定行。
     * \param row messages 行下标。
     */
    void applyCardToRow(int row);
    /**
     * \brief 构造一行消息的 QVariantMap。
     * \param role 角色。
     * \param text 正文。
     * \param id 数据库行 id。
     * \param feedback 本地评价。
     * \param time 展示时间。
     * \return 固定字段的消息行。
     */
    QVariantMap makeRow(const QString &role, const QString &text, qint64 id, int feedback,
                        const QString &time) const;
    /**
     * \brief 解析 ```card 卡片块。
     * \param text 消息正文。
     * \param[out] title 卡片标题。
     * \param[out] body 卡片正文。
     * \return 是否为卡片消息。
     */
    static bool parseCard(const QString &text, QString *title, QString *body);
    /// 合并消息变更并安排一次节流信号。
    void scheduleMessagesChanged();
    /// 合并短时间内的多次上下文变化，延迟统一重建，避免重建风暴。
    void scheduleContextRefresh();
    /// 立即发出 messagesChanged（节流定时器到点）。
    void flushMessagesChanged();
    /**
     * \brief 更新 streaming 与 busy 标记。
     * \param streaming 是否正在流式回复。
     */
    void setStreaming(bool streaming);
    /// 重建对话上下文（角色/人设/世界观/记忆分区/好感度）。
    void applyCharacterContext();
    /// 刷新标题、模型摘要、欢迎语与好感度属性。
    void updateHeader();
    /// 当前角色/人设的显示名。
    QString personaDisplayName() const;
    /// 应用数据目录。
    QString dataDir() const;

    ChatService *m_chat = nullptr;         ///< 对话服务。
    ChatStore *m_store = nullptr;          ///< 聊天记录仓库。
    MemoryStore *m_memory = nullptr;       ///< 记忆仓库。
    IntimacyService *m_intimacy = nullptr; ///< 好感度仓库。
    ConfigStore *m_config = nullptr;       ///< 配置仓库。
    CharacterStore *m_characters = nullptr;///< 角色仓库。
    WorldStore *m_worlds = nullptr;        ///< 世界观仓库。

    QVariantList m_messages; ///< 合并后的消息行。
    QTimer m_flushTimer;     ///< messagesChanged 节流定时器。
    QTimer m_contextTimer;   ///< 上下文重建防抖定时器。
    bool m_streaming = false; ///< 流式回复标记。

    QString m_chatTitle;          ///< 标题缓存。
    QString m_modelSummary;       ///< 模型摘要缓存。
    QString m_welcomeText;        ///< 欢迎语缓存。
    bool m_llmReady = false;      ///< 模型就绪缓存。
    int m_intimacyScore = 0;      ///< 好感度分数缓存。
    QString m_intimacyLevel;      ///< 好感度等级缓存。
    QString m_lastCharacterId;    ///< 上次应用的角色 id（防止重建风暴）。
    QString m_lastPersonaId;      ///< 上次应用的人设 id。
    bool m_applyingContext = false; ///< 上下文重建防重入标记。
};

#endif // CHATVIEWMODEL_H
