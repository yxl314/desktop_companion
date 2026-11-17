/**
 * \file src/viewmodels/dataviewmodel.h
 * \brief 数据页 ViewModel：数据/日志目录、占用大小、导出与更新检查。
 */

#ifndef DATAVIEWMODEL_H
#define DATAVIEWMODEL_H

#include <QObject>
#include <QString>

#include "viewmodelbase.h"

class ChatService;
class ChatStore;
class ConfigStore;
class DialogService;
class QNetworkAccessManager;

/**
 * \brief 数据页 ViewModel。
 *
 * 负责展示数据目录/日志目录/占用大小，导出聊天记录与日志，打开目录，
 * 重置新手引导以及检查更新。目录来自 QStandardPaths，不在本层做路径规则。
 */
class DataViewModel : public ViewModelBase
{
    Q_OBJECT
    /** \brief 应用数据目录绝对路径（只读）。 */
    Q_PROPERTY(QString dataPath READ dataPath CONSTANT)
    /** \brief 日志目录绝对路径（只读）。 */
    Q_PROPERTY(QString logPath READ logPath CONSTANT)
    /** \brief 数据目录占用大小的可读文本。 */
    Q_PROPERTY(QString dataSize READ dataSize NOTIFY dataSizeChanged)
    /** \brief 最近一次导出的目标路径。 */
    Q_PROPERTY(QString lastExportPath READ lastExportPath NOTIFY lastExportPathChanged)

public:
    /**
     * \brief 构造数据页 ViewModel。
     * \param config 配置仓库，必须非空。
     * \param store 聊天记录仓库，必须非空。
     * \param chat 对话服务，必须非空。
     * \param dialogs 对话框服务，必须非空。
     * \param parent 父对象。
     */
    explicit DataViewModel(ConfigStore *config, ChatStore *store, ChatService *chat,
                           DialogService *dialogs, QObject *parent = nullptr);

    /// 析构函数。
    ~DataViewModel() override;

    /// \brief 数据目录。
    QString dataPath() const;
    /// \brief 日志目录。
    QString logPath() const;
    /// \brief 数据占用大小文本。
    QString dataSize() const;
    /// \brief 最近导出路径。
    QString lastExportPath() const;

    /// 导出聊天记录为 Markdown。
    Q_INVOKABLE void exportChats();
    /// 导出日志目录中的文件。
    Q_INVOKABLE void exportLogs();
    /// 打开数据目录。
    Q_INVOKABLE void openDataDir();
    /// 打开日志目录。
    Q_INVOKABLE void openLogsDir();
    /// 重置新手引导（下次启动重新展示）。
    Q_INVOKABLE void resetOnboarding();
    /// 检查更新（读取 config.json 的 updateUrl）。
    Q_INVOKABLE void checkUpdate();

signals:
    /// dataSize 属性变化信号。
    void dataSizeChanged();
    /// lastExportPath 属性变化信号。
    void lastExportPathChanged();

private:
    /// 重新计算数据目录占用大小。
    void refreshSize();
    /**
     * \brief 设置最近导出路径。
     * \param path 导出目标路径。
     */
    void setLastExportPath(const QString &path);

    ConfigStore *m_config = nullptr;             ///< 配置仓库。
    ChatStore *m_store = nullptr;                ///< 聊天记录仓库。
    ChatService *m_chat = nullptr;               ///< 对话服务。
    DialogService *m_dialogs = nullptr;          ///< 对话框服务。
    QNetworkAccessManager *m_network = nullptr;  ///< 更新检查网络访问（惰性创建）。

    QString m_dataSize;      ///< 占用大小文本。
    QString m_lastExportPath; ///< 最近导出路径。
};

#endif // DATAVIEWMODEL_H
