/**
 * \file src/platform/foregroundwatcher.h
 * \brief 前台应用轮询与分类（自动模式切换用）。
 */

#ifndef FOREGROUNDWATCHER_H
#define FOREGROUNDWATCHER_H

#include <QObject>
#include <QString>

class QTimer;

/**
 * \brief 前台应用分类：用于 SYS-M-03 的自动模式切换。
 *
 * 类别：0 休闲（含游戏/其他）、1 工作（IDE/Office）、2 游戏。
 */
class ForegroundWatcher : public QObject
{
    Q_OBJECT

public:
    /**
     * \brief 构造前台应用监视器。
     * @param parent 父对象。
     */
    explicit ForegroundWatcher(QObject *parent = nullptr);

    /**
     * \brief 开始轮询前台进程。
     * @param intervalMs 轮询间隔（毫秒）。
     */
    void start(int intervalMs = 5000);
    /**
     * \brief 停止轮询。
     */
    void stop();
    /// 当前前台进程名（可执行文件名）。
    QString foregroundProcess() const { return m_process; }

    /**
     * \brief 把进程名分类为模式类别。
     * @param processName 进程名。
     * @return 0 休闲、1 工作、2 游戏。
     */
    static int classify(const QString &processName);

signals:
    /// 分类结果变化。
    void categoryChanged(int category);

private:
    void poll();

    QTimer *m_timer = nullptr; ///< 轮询定时器。
    QString m_process;         ///< 当前前台进程名。
    int m_category = 0;        ///< 当前分类。
};

#endif // FOREGROUNDWATCHER_H
