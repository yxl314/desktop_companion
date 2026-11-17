#include "test_promptbuilder.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QtTest>

#include "services/promptbuilder.h"

void TestPromptBuilder::sectionsInFixedOrder()
{
    PromptContext context;
    context.characterId = QStringLiteral("builtin-default");
    context.personaName = QStringLiteral("小柚");
    context.personaBody = QStringLiteral("你是一只住在桌面上的小精灵。");
    context.worldBody = QStringLiteral("世界是一座漂浮在云上的小镇。");
    context.constraints = QStringLiteral("只说中文，回答不超过三句话。");
    context.memories = QStringList{QStringLiteral("用户喜欢猫"), QStringLiteral("用户生日是 5 月 1 日")};
    context.situation = QStringLiteral("现在是晚上十点，用户刚下班。");

    const QString prompt = PromptBuilder::build(context);
    QVERIFY(!prompt.isEmpty());

    const QStringList headers = {
        QStringLiteral("【身份】"),  QStringLiteral("【世界观】"), QStringLiteral("【人设】"),
        QStringLiteral("【回复约束】"), QStringLiteral("【记忆】"),   QStringLiteral("【情境】"),
    };
    int previous = -1;
    for (const QString &header : headers) {
        const int position = prompt.indexOf(header);
        QVERIFY2(position >= 0, qPrintable(header));
        QVERIFY2(position > previous, qPrintable(header));
        previous = position;
    }

    // 每段正文都应落在对应标题之后。
    const QStringList bodies = {
        QStringLiteral("小柚"),          QStringLiteral("漂浮在云上"),   QStringLiteral("小精灵"),
        QStringLiteral("回答不超过三句话"), QStringLiteral("用户喜欢猫"), QStringLiteral("晚上十点"),
    };
    for (qsizetype index = 0; index < bodies.size(); ++index) {
        const int headerPosition = prompt.indexOf(headers.at(index));
        const int bodyPosition = prompt.indexOf(bodies.at(index));
        QVERIFY2(bodyPosition > headerPosition, qPrintable(bodies.at(index)));
        if (index + 1 < bodies.size())
            QVERIFY2(bodyPosition < prompt.indexOf(headers.at(index + 1)), qPrintable(bodies.at(index)));
    }

    QVERIFY(prompt.contains(QStringLiteral("- 用户喜欢猫")));
    QVERIFY(!prompt.startsWith(QLatin1Char('\n')));
    QVERIFY(!prompt.endsWith(QLatin1Char('\n')));
}

void TestPromptBuilder::worldPeersRenderedAfterWorld()
{
    PromptContext context;
    context.personaName = QStringLiteral("小柚");
    context.personaBody = QStringLiteral("你是一只住在桌面上的小精灵。");
    context.worldBody = QStringLiteral("世界是一座漂浮在云上的小镇。");
    context.worldPeers = QStringList{QStringLiteral("阿桃（喜欢在钟楼打盹的守钟人）"),
                                     QStringLiteral("   "),
                                     QStringLiteral(" 阿岚（修伞铺的学徒） ")};

    const QString prompt = PromptBuilder::build(context);
    QVERIFY(prompt.contains(QStringLiteral("【世界观】")));
    QVERIFY(prompt.contains(QStringLiteral("【同世界观的伙伴】")));

    // WLD-L-01：伙伴段必须紧跟世界观段，位于人设段之前。
    const int worldPosition = prompt.indexOf(QStringLiteral("【世界观】"));
    const int peerPosition = prompt.indexOf(QStringLiteral("【同世界观的伙伴】"));
    const int personaPosition = prompt.indexOf(QStringLiteral("【人设】"));
    QVERIFY(worldPosition >= 0);
    QVERIFY(peerPosition > worldPosition);
    QVERIFY(personaPosition > peerPosition);
    QVERIFY2(prompt.indexOf(QStringLiteral("漂浮在云上")) < peerPosition, "世界观正文应在伙伴段之前");

    // 每条伙伴渲染成列表项，空白项被丢弃。
    QVERIFY(prompt.contains(QStringLiteral("- 阿桃（喜欢在钟楼打盹的守钟人）")));
    QVERIFY(prompt.contains(QStringLiteral("- 阿岚（修伞铺的学徒）")));
    QCOMPARE(prompt.count(QStringLiteral("- ")), 2);

    // 没有伙伴时整段省略，保持原有提示词结构。
    PromptContext withoutPeers;
    withoutPeers.worldBody = QStringLiteral("云上小镇");
    QVERIFY(!PromptBuilder::build(withoutPeers).contains(QStringLiteral("【同世界观的伙伴】")));
}

void TestPromptBuilder::emptySectionsSkipped()
{
    PromptContext empty;
    QVERIFY(PromptBuilder::build(empty).isEmpty());

    PromptContext situationOnly;
    situationOnly.situation = QStringLiteral("  用户发来一条消息。  ");
    QCOMPARE(PromptBuilder::build(situationOnly), QStringLiteral("【情境】\n用户发来一条消息。"));

    PromptContext whitespace;
    whitespace.personaName = QStringLiteral("   ");
    whitespace.worldBody = QStringLiteral("\n\t");
    QVERIFY(PromptBuilder::build(whitespace).isEmpty());
}

void TestPromptBuilder::identityFallsBackToCharacterId()
{
    PromptContext context;
    context.characterId = QStringLiteral("builtin-default");
    const QString prompt = PromptBuilder::build(context);
    QVERIFY(prompt.contains(QStringLiteral("【身份】")));
    QVERIFY(prompt.contains(QStringLiteral("builtin-default")));
    QVERIFY(!prompt.contains(QStringLiteral("【世界观】")));
    QVERIFY(!prompt.contains(QStringLiteral("【人设】")));
}

void TestPromptBuilder::memoriesRendered()
{
    PromptContext context;
    context.memories = QStringList{QStringLiteral("  用户喜欢猫  "), QStringLiteral("用户生日是 5 月 1 日"),
                                   QStringLiteral("   ")};
    const QString prompt = PromptBuilder::build(context);
    QVERIFY(prompt.contains(QStringLiteral("【记忆】")));
    QVERIFY(prompt.contains(QStringLiteral("- 用户喜欢猫")));
    QVERIFY(prompt.contains(QStringLiteral("- 用户生日是 5 月 1 日")));
    QCOMPARE(prompt.count(QStringLiteral("- ")), 2);

    PromptContext withoutMemories;
    withoutMemories.personaName = QStringLiteral("小柚");
    QVERIFY(!PromptBuilder::build(withoutMemories).contains(QStringLiteral("【记忆】")));
}

void TestPromptBuilder::buildMessagesShape()
{
    QJsonArray history;
    history.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                               {QStringLiteral("content"), QStringLiteral("早")}});
    history.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("assistant")},
                               {QStringLiteral("content"), QStringLiteral("早呀")}});

    const QJsonArray messages =
        PromptBuilder::buildMessages(QStringLiteral("你是我的人设"), history, QStringLiteral("今天吃什么"));
    QCOMPARE(messages.size(), 4);
    QCOMPARE(messages.at(0).toObject().value(QStringLiteral("role")).toString(), QStringLiteral("system"));
    QCOMPARE(messages.at(0).toObject().value(QStringLiteral("content")).toString(), QStringLiteral("你是我的人设"));
    QCOMPARE(messages.at(1).toObject().value(QStringLiteral("content")).toString(), QStringLiteral("早"));
    QCOMPARE(messages.at(2).toObject().value(QStringLiteral("role")).toString(), QStringLiteral("assistant"));
    QCOMPARE(messages.at(3).toObject().value(QStringLiteral("role")).toString(), QStringLiteral("user"));
    QCOMPARE(messages.at(3).toObject().value(QStringLiteral("content")).toString(), QStringLiteral("今天吃什么"));
    // 历史条目必须原样透传。
    QCOMPARE(messages.at(1).toObject().value(QStringLiteral("role")).toString(), QStringLiteral("user"));

    QVERIFY(PromptBuilder::buildMessages(QString(), QJsonArray(), QString()).isEmpty());
    QCOMPARE(PromptBuilder::buildMessages(QString(), history, QString()).size(), 2);

    PromptContext context;
    context.personaName = QStringLiteral("小柚");
    const QJsonArray built = PromptBuilder::buildMessages(context, history, QStringLiteral("你好"));
    QCOMPARE(built.at(0).toObject().value(QStringLiteral("content")).toString(), PromptBuilder::build(context));
}

void TestPromptBuilder::defaultConstraintsAvailable()
{
    const QString constraints = PromptBuilder::defaultConstraints();
    QVERIFY(!constraints.trimmed().isEmpty());
    QVERIFY(constraints.contains(QStringLiteral("回复")));
}

QTEST_GUILESS_MAIN(TestPromptBuilder)
