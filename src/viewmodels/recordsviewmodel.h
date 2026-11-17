/**
 * \file src/viewmodels/recordsviewmodel.h
 * \brief 记录页 ViewModel：聊天记录列表、评价统计、导出与清空。
 */

#ifndef RECORDSVIEWMODEL_H
#define RECORDSVIEWMODEL_H

#include <QObject>
#include <QVariantList>

#include "viewmodelbase.h"

class ChatStore;
class DialogService;

/**
 * \brief 聊天记录页 ViewModel。
 *
 * \c records 的每一行是 QVariantMap，字段为 id / role / text / feedback / time。
 * 记录只来自 ChatStore 当前会话；导出为 Markdown 文件，清空只影响当前会话消息。
 */
class RecordsViewModel : public ViewModelBase
{
    Q_OBJECT
    /** \brief 当前会话记录列表（行字段见类注释）。 */
    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)
    /** \brief 点赞/点踩统计，格式「赞 / 踩」。 */
    Q_PROPERTY(QString feedbackSummary READ feedbackSummary NOTIFY feedbackSummaryChanged)

public:
    /**
     * \brief 构造记录页 ViewModel。
     * \param store 聊天记录仓库，必须非空。
     * \param dialogs 对话框服务，必须非空（导出选路径）。
     * \param parent 父对象。
     */
    explicit RecordsViewModel(ChatStore *store, DialogService *dialogs, QObject *parent = nullptr);

    /// 析构函数。
    ~RecordsViewModel() override;

    /// \brief 记录列表。
    QVariantList records() const;
    /// \brief 评价统计文本。
    QString feedbackSummary() const;

    /// 重新读取当前会话记录与评价统计。
    Q_INVOKABLE void refresh();
    /// 导出当前会话记录为 Markdown。
    Q_INVOKABLE void exportChat();
    /// 清空当前会话记录。
    Q_INVOKABLE void clearRecords();

signals:
    /// records 属性变化信号。
    void recordsChanged();
    /// feedbackSummary 属性变化信号。
    void feedbackSummaryChanged();

private:
    ChatStore *m_store = nullptr;       ///< 聊天记录仓库。
    DialogService *m_dialogs = nullptr; ///< 对话框服务。
    QVariantList m_records;             ///< 记录行缓存。
    QString m_feedbackSummary;          ///< 评价统计缓存。
};

#endif // RECORDSVIEWMODEL_H
