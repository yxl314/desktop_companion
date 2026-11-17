#ifndef TEST_CHATENGINE_H
#define TEST_CHATENGINE_H

#include <QObject>

/// mock 对话引擎的最小回归测试：发送后先进入 thinking，再流式输出并结束。
class TestChatEngine : public QObject
{
    Q_OBJECT

private slots:
    void replyContainsExpectedLine();
    void thinkingThenStreamingThenDone();
};

#endif // TEST_CHATENGINE_H
