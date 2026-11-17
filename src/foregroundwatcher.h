#ifndef FOREGROUNDWATCHER_H
#define FOREGROUNDWATCHER_H

#include <QObject>
#include <QString>

class QTimer;

/// 前台应用分类：用于 SYS-M-03 的自动模式切换。
/// 类别：0 休闲（含游戏/其他）、1 工作（IDE/Office）、2 游戏。
class ForegroundWatcher : public QObject
{
    Q_OBJECT

public:
    explicit ForegroundWatcher(QObject *parent = nullptr);

    void start(int intervalMs = 5000);
    void stop();
    QString foregroundProcess() const { return m_process; }

    static int classify(const QString &processName);

signals:
    void categoryChanged(int category);

private:
    void poll();

    QTimer *m_timer = nullptr;
    QString m_process;
    int m_category = 0;
};

#endif // FOREGROUNDWATCHER_H
