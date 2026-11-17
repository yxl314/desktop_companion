/**
 * \file src/viewmodels/memoriesviewmodel.h
 * \brief 记忆页 ViewModel：记忆列表增删改查、置顶与手动抽取。
 */

#ifndef MEMORIESVIEWMODEL_H
#define MEMORIESVIEWMODEL_H

#include <QObject>
#include <QVariantList>

#include "viewmodelbase.h"

class ChatService;
class MemoryStore;

/**
 * \brief 记忆页 ViewModel。
 *
 * 记忆按 (characterId, personaId) 分区，分区键取自当前对话上下文
 * （ChatService::characterId / personaId）。\c memories 的每一行是 QVariantMap，
 * 字段为 id / content / pinned / createdAt。
 */
class MemoriesViewModel : public ViewModelBase
{
    Q_OBJECT
    /** \brief 当前分区的记忆列表（行字段见类注释）。 */
    Q_PROPERTY(QVariantList memories READ memories NOTIFY memoriesChanged)
    /** \brief 当前分区的记忆条数。 */
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    /**
     * \brief 构造记忆页 ViewModel。
     * \param memory 记忆仓库，必须非空。
     * \param chat 对话服务，必须非空（提供分区键与抽取完成信号）。
     * \param parent 父对象。
     */
    explicit MemoriesViewModel(MemoryStore *memory, ChatService *chat, QObject *parent = nullptr);

    /// 析构函数。
    ~MemoriesViewModel() override;

    /// \brief 记忆列表。
    QVariantList memories() const;
    /// \brief 记忆条数。
    int count() const;

    /**
     * \brief 新增一条记忆。
     * \param content 记忆正文；空白会被拒绝。
     */
    Q_INVOKABLE void add(const QString &content);
    /**
     * \brief 更新一条记忆的正文。
     * \param id 记忆 id（字符串形式）。
     * \param content 新正文。
     */
    Q_INVOKABLE void update(const QString &id, const QString &content);
    /**
     * \brief 删除一条记忆。
     * \param id 记忆 id（字符串形式）。
     */
    Q_INVOKABLE void remove(const QString &id);
    /**
     * \brief 切换记忆置顶状态。
     * \param id 记忆 id（字符串形式）。
     */
    Q_INVOKABLE void togglePin(const QString &id);
    /// 刷新当前分区列表。
    Q_INVOKABLE void refresh();
    /// 请求异步抽取记忆（附加信号 extractRequested，由 Application 接线）。
    Q_INVOKABLE void extractNow();

signals:
    /// memories 属性变化信号。
    void memoriesChanged();
    /// count 属性变化信号。
    void countChanged();
    /// 请求 Application 触发 LLM 记忆抽取（附加信号，契约外集成项）。
    void extractRequested();

private:
    /// 当前角色 id（分区键）。
    QString characterId() const;
    /// 当前人设 id（分区键）。
    QString personaId() const;

    MemoryStore *m_memory = nullptr; ///< 记忆仓库。
    ChatService *m_chat = nullptr;   ///< 对话服务。
    QVariantList m_memories;         ///< 记忆行缓存。
    int m_count = 0;                 ///< 条数缓存。
};

#endif // MEMORIESVIEWMODEL_H
