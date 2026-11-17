/**
 * \file src/app/windowregistry.h
 * \brief 窗口注册表：集中创建/显示主窗口、桌宠窗口、气泡与悬浮球。
 */

#ifndef WINDOWREGISTRY_H
#define WINDOWREGISTRY_H

#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVector>

class QQmlApplicationEngine;
class QQuickWindow;

/**
 * \brief 视图层的窗口工厂与句柄表（经典 MVVM 的 View 层基础设施）。
 *
 * 只负责从 qrc 加载 QML、创建窗口、记录句柄与显隐控制；
 * 不含业务逻辑，所有状态来自 ViewModel 的属性绑定。
 */
class WindowRegistry : public QObject
{
    Q_OBJECT

public:
    /**
     * \brief 构造窗口注册表。
     * \param engine QML 引擎（由 Application 持有）。
     * \param parent 父对象。
     */
    explicit WindowRegistry(QQmlApplicationEngine *engine, QObject *parent = nullptr);

    /**
     * \brief 加载主窗口（qml/MainWindow.qml）。
     * \return 是否加载成功。
     */
    bool loadMainWindow();

    /// 主窗口句柄；未加载时为空。
    QQuickWindow *mainWindow() const { return m_mainWindow; }

    /// 显示并激活主窗口。
    void showMainWindow();
    /// 隐藏主窗口（关闭按钮等价于隐藏到托盘）。
    void hideMainWindow();
    /// 切换主窗口显隐。
    void toggleMainWindow();
    /// 把主窗口带到前台。
    void raiseMainWindow();

    /**
     * \brief 按数量创建/销毁桌宠窗口（qml/pet/PetWindow.qml）。
     * \param count 目标桌宠数量（1~3）。
     * \return 是否全部创建成功。
     */
    bool setPetCount(int count);

    /// 当前桌宠窗口列表。
    QVector<QQuickWindow *> petWindows() const { return m_petWindows; }

    /**
     * \brief 显示/隐藏指定桌宠窗口。
     * \param index 桌宠下标。
     * \param visible 是否显示。
     */
    void setPetVisible(int index, bool visible);

    /// 加载气泡窗口（qml/pet/BubbleWindow.qml）；可重复调用。
    bool loadBubbleWindow();
    /// 加载悬浮球窗口（qml/pet/BallWindow.qml）。
    bool loadBallWindow();

    /// 气泡窗口句柄。
    QQuickWindow *bubbleWindow() const { return m_bubbleWindow; }
    /// 悬浮球窗口句柄。
    QQuickWindow *ballWindow() const { return m_ballWindow; }

    /// 全部已创建窗口（用于截图/验证）。
    QList<QQuickWindow *> allWindows() const;

signals:
    /// 窗口集合发生变化（用于验证/调试）。
    void windowsChanged();

private:
    /**
     * \brief 从 qrc 路径创建一个 QML Window 实例。
     * \param qmlPath qrc 路径，例如 :/qml/pet/PetWindow.qml。
     * \param properties 创建后立即写入的初始属性。
     * \return 窗口句柄；失败返回空。
     */
    QQuickWindow *createWindow(const QString &qmlPath, const QVariantMap &properties = {});

    QQmlApplicationEngine *m_engine = nullptr; ///< QML 引擎。
    QQuickWindow *m_mainWindow = nullptr;      ///< 主窗口。
    QQuickWindow *m_bubbleWindow = nullptr;    ///< 气泡窗口。
    QQuickWindow *m_ballWindow = nullptr;      ///< 悬浮球窗口。
    QVector<QQuickWindow *> m_petWindows;      ///< 桌宠窗口列表。
};

#endif // WINDOWREGISTRY_H
