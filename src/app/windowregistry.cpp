/**
 * \file src/app/windowregistry.cpp
 * \brief 窗口注册表实现：从 qrc 加载 QML 窗口并管理显隐。
 */

#include "app/windowregistry.h"

#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickWindow>
#include <QUrl>

#include <stacklog/stacklog.hpp>

WindowRegistry::WindowRegistry(QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
}

QQuickWindow *WindowRegistry::createWindow(const QString &qmlPath, const QVariantMap &properties)
{
    if (m_engine == nullptr) {
        STACKLOG_ERROR() << "window registry: engine is null";
        return nullptr;
    }
    QQmlComponent component(m_engine, QUrl(qmlPath));
    if (component.isError()) {
        STACKLOG_ERROR() << "load qml failed: " << qmlPath.toStdString() << " "
                         << component.errorString().toStdString();
        return nullptr;
    }
    QObject *object = component.create();
    if (object == nullptr) {
        STACKLOG_ERROR() << "create qml window failed: " << qmlPath.toStdString();
        return nullptr;
    }
    auto *window = qobject_cast<QQuickWindow *>(object);
    if (window == nullptr) {
        STACKLOG_ERROR() << "root object is not a Window: " << qmlPath.toStdString();
        delete object;
        return nullptr;
    }
    for (auto it = properties.cbegin(); it != properties.cend(); ++it)
        window->setProperty(it.key().toUtf8().constData(), it.value());
    return window;
}

bool WindowRegistry::loadMainWindow()
{
    if (m_mainWindow != nullptr)
        return true;
    m_mainWindow = createWindow(QStringLiteral("qrc:/qml/MainWindow.qml"));
    if (m_mainWindow == nullptr)
        return false;
    m_mainWindow->setObjectName(QStringLiteral("mainWindow"));
    STACKLOG_INFO() << "main window loaded";
    emit windowsChanged();
    return true;
}

void WindowRegistry::showMainWindow()
{
    if (m_mainWindow == nullptr)
        return;
    m_mainWindow->show();
    m_mainWindow->raise();
    m_mainWindow->requestActivate();
}

void WindowRegistry::hideMainWindow()
{
    if (m_mainWindow != nullptr)
        m_mainWindow->hide();
}

void WindowRegistry::toggleMainWindow()
{
    if (m_mainWindow == nullptr)
        return;
    if (m_mainWindow->isVisible())
        hideMainWindow();
    else
        showMainWindow();
}

void WindowRegistry::raiseMainWindow()
{
    showMainWindow();
}

bool WindowRegistry::setPetCount(int count)
{
    count = qBound(0, count, 3);
    while (m_petWindows.size() > count) {
        QQuickWindow *window = m_petWindows.takeLast();
        if (window != nullptr) {
            window->hide();
            window->deleteLater();
        }
    }
    bool ok = true;
    while (m_petWindows.size() < count) {
        const int index = m_petWindows.size();
        QVariantMap properties;
        properties.insert(QStringLiteral("petIndex"), index);
        QQuickWindow *window = createWindow(QStringLiteral("qrc:/qml/pet/PetWindow.qml"), properties);
        if (window == nullptr) {
            ok = false;
            break;
        }
        window->setObjectName(QStringLiteral("petWindow%1").arg(index));
        m_petWindows.append(window);
    }
    emit windowsChanged();
    return ok;
}

void WindowRegistry::setPetVisible(int index, bool visible)
{
    if (index < 0 || index >= m_petWindows.size())
        return;
    if (m_petWindows.at(index) == nullptr)
        return;
    m_petWindows.at(index)->setVisible(visible);
}

bool WindowRegistry::loadBubbleWindow()
{
    if (m_bubbleWindow != nullptr)
        return true;
    m_bubbleWindow = createWindow(QStringLiteral("qrc:/qml/pet/BubbleWindow.qml"));
    if (m_bubbleWindow == nullptr)
        return false;
    m_bubbleWindow->setObjectName(QStringLiteral("bubbleWindow"));
    emit windowsChanged();
    return true;
}

bool WindowRegistry::loadBallWindow()
{
    if (m_ballWindow != nullptr)
        return true;
    m_ballWindow = createWindow(QStringLiteral("qrc:/qml/pet/BallWindow.qml"));
    if (m_ballWindow == nullptr)
        return false;
    m_ballWindow->setObjectName(QStringLiteral("ballWindow"));
    emit windowsChanged();
    return true;
}

QList<QQuickWindow *> WindowRegistry::allWindows() const
{
    QList<QQuickWindow *> windows;
    if (m_mainWindow != nullptr)
        windows.append(m_mainWindow);
    for (QQuickWindow *pet : m_petWindows) {
        if (pet != nullptr)
            windows.append(pet);
    }
    if (m_bubbleWindow != nullptr)
        windows.append(m_bubbleWindow);
    if (m_ballWindow != nullptr)
        windows.append(m_ballWindow);
    return windows;
}
