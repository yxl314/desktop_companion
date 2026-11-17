/**
 * \file src/viewmodels/dialogservice.h
 * \brief 文件/目录选择与消息对话框的抽象：供 ViewModel 注入，便于单元测试替换。
 *
 * 默认实现基于 QFileDialog / QMessageBox；测试可派生本类并覆写虚函数，
 * 从而避免在无界面环境弹出真实对话框。ViewModel 只依赖本抽象，
 * 不直接 include 任何窗口类。
 */

#ifndef DIALOGSERVICE_H
#define DIALOGSERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>

/**
 * \brief 对话框服务抽象：打开文件、保存文件、选择目录、确认与提示。
 *
 * 全部接口均为 Q_INVOKABLE 虚函数，QML 可直接调用；派生类可覆写返回值，
 * 用于测试时注入固定路径或固定确认结果。
 */
class DialogService : public QObject
{
    Q_OBJECT

public:
    /**
     * \brief 构造对话框服务。
     * \param parent 父对象，所有权交给 Qt 对象树。
     */
    explicit DialogService(QObject *parent = nullptr);

    /// 析构函数。
    ~DialogService() override;

    /**
     * \brief 选择单个已有文件。
     * \param title 对话框标题。
     * \param filter 文件过滤器，Qt 格式，如 "Markdown (*.md)"。
     * \return 选中的绝对路径；用户取消时返回空串。
     */
    Q_INVOKABLE virtual QString openFile(const QString &title, const QString &filter);

    /**
     * \brief 选择多个已有文件。
     * \param title 对话框标题。
     * \param filter 文件过滤器，Qt 格式。
     * \return 选中的绝对路径列表；用户取消时返回空列表。
     */
    Q_INVOKABLE virtual QStringList openFiles(const QString &title, const QString &filter);

    /**
     * \brief 选择目录。
     * \param title 对话框标题。
     * \return 选中的目录绝对路径；用户取消时返回空串。
     */
    Q_INVOKABLE virtual QString openDirectory(const QString &title);

    /**
     * \brief 选择保存文件路径。
     * \param title 对话框标题。
     * \param filter 文件过滤器，Qt 格式。
     * \param defaultPath 预填充的默认路径，可为空。
     * \return 选中的绝对路径；用户取消时返回空串。
     */
    Q_INVOKABLE virtual QString saveFile(const QString &title, const QString &filter,
                                         const QString &defaultPath = QString());

    /**
     * \brief 弹出确认对话框（是/否）。
     * \param title 对话框标题。
     * \param text 提示正文。
     * \return 用户选择「是」时返回 true，否则返回 false。
     */
    Q_INVOKABLE virtual bool confirm(const QString &title, const QString &text);

    /**
     * \brief 弹出信息提示对话框。
     * \param title 对话框标题。
     * \param text 提示正文。
     */
    Q_INVOKABLE virtual void info(const QString &title, const QString &text);
};

#endif // DIALOGSERVICE_H
