#ifndef TEST_CHATSTORE_H
#define TEST_CHATSTORE_H

#include <QObject>

class TestChatStore : public QObject
{
    Q_OBJECT

private slots:
    void persistsMessages();
    void separatesConversationsByPersona();
    void clearRemovesMessages();
};

#endif // TEST_CHATSTORE_H
