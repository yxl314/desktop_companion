/**
 * \file src/viewmodels/viewmodelbase.h
 * \brief ViewModel 基类：为全部页面 ViewModel 提供统一的状态、错误与忙碌反馈。
 *
 * 所有 ViewModel 均继承本类，QML 只需绑定 \c statusText / \c errorText / \c busy
 * 三个属性即可获得一致的反馈体验；子类通过受保护的 setStatus / setError /
 * setBusy 更新状态，通过 clearStatus() 命令复位。
 */

#ifndef VIEWMODELBASE_H
#define VIEWMODELBASE_H

#include <QObject>
#include <QString>

/**
 * \brief ViewModel 基类：状态文本、错误文本与忙碌标记。
 *
 * 依赖方向：ViewModel → data/services → domain；本类不依赖任何 QML 或窗口类，
 * 因此可脱离 QML 在单元测试中直接实例化。
 */
class ViewModelBase : public QObject
{
    Q_OBJECT
    /** \brief 当前状态提示文本（如「已保存」「回复中…」），空串表示无提示。 */
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    /** \brief 最近一次错误的可读文本，空串表示无错误。 */
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
    /** \brief 是否正在执行耗时操作，供 View 禁用按钮/显示进度。 */
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    /**
     * \brief 构造基类。
     * \param parent 父对象，所有权交给 Qt 对象树。
     */
    explicit ViewModelBase(QObject *parent = nullptr);

    /// 析构函数。
    ~ViewModelBase() override;

    /**
     * \brief 读取状态提示文本。
     * \return 当前状态文本，可能为空串。
     */
    QString statusText() const;

    /**
     * \brief 读取错误文本。
     * \return 当前错误文本，可能为空串。
     */
    QString errorText() const;

    /**
     * \brief 读取忙碌标记。
     * \return 正在忙碌时返回 true。
     */
    bool busy() const;

    /// 清空状态文本与错误文本（QML 命令）。
    Q_INVOKABLE void clearStatus();

signals:
    /// statusText 属性变化信号。
    void statusTextChanged();
    /// errorText 属性变化信号。
    void errorTextChanged();
    /// busy 属性变化信号。
    void busyChanged();

protected:
    /**
     * \brief 更新状态文本（相同值不重复发信号）。
     * \param text 新的状态文本，空串表示清除。
     */
    void setStatus(const QString &text);

    /**
     * \brief 更新错误文本（相同值不重复发信号）。
     * \param text 新的错误文本，空串表示清除。
     */
    void setError(const QString &text);

    /**
     * \brief 更新忙碌标记（相同值不重复发信号）。
     * \param busy 是否忙碌。
     */
    void setBusy(bool busy);

    /**
     * \brief 用统一错误码格式化错误文本并写入 errorText。
     * \param code services/errorcodes.h 中的 ErrorCode 数值。
     * \param detail 可读细节，可为空。
     */
    void reportError(quint32 code, const QString &detail);

private:
    QString m_statusText; ///< 状态提示文本。
    QString m_errorText;  ///< 错误提示文本。
    bool m_busy = false;  ///< 忙碌标记。
};

#endif // VIEWMODELBASE_H
