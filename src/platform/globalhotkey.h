/**
 * \file src/platform/globalhotkey.h
 * \brief Windows 全局快捷键注册与分发。
 */

#ifndef GLOBALHOTKEY_H
#define GLOBALHOTKEY_H

#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QString>

/**
 * \brief Windows 全局快捷键。
 *
 * 默认快捷键：Ctrl+Alt+P 呼出/隐藏桌宠、Ctrl+Alt+C 打开聊天、
 * Ctrl+Alt+M 切换勿扰模式。非 Windows 平台为空实现。
 */
class GlobalHotkey : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    /**
     * \brief 构造全局快捷键分发器。
     * @param parent 父对象。
     */
    explicit GlobalHotkey(QObject *parent = nullptr);
    ~GlobalHotkey() override;

    /**
     * \brief 注册默认全局快捷键。
     * @param[out] error 非空时写入失败原因。
     * @return 是否全部注册成功。
     */
    bool registerDefaultHotkeys(QString *error = nullptr);
    /**
     * \brief 注销已注册的全部全局快捷键。
     */
    void unregisterAll();

signals:
    /// 请求显示/隐藏桌宠。
    void togglePetRequested();
    /// 请求打开聊天。
    void openChatRequested();
    /// 请求切换勿扰模式。
    void toggleMuteRequested();

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    QList<int> m_ids; ///< 已注册的快捷键 id 列表。
};

#endif // GLOBALHOTKEY_H
