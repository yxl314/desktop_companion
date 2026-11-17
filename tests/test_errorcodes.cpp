#include "test_errorcodes.h"

#include <QtTest>

#include "services/errorcodes.h"

void TestErrorCodes::formatsCode()
{
    QCOMPARE(errors::codeText(ErrorCode::Success), QStringLiteral("0x0000_0000"));
    QCOMPARE(errors::codeText(ErrorCode::AuthFailed), QStringLiteral("0x0002_0003"));
    QCOMPARE(errors::codeText(ErrorCode::DataCorrupted), QStringLiteral("0x0003_0005"));
}

void TestErrorCodes::mapsHttpStatus()
{
    QCOMPARE(errors::fromHttpStatus(200), ErrorCode::Success);
    QCOMPARE(errors::fromHttpStatus(401), ErrorCode::AuthFailed);
    QCOMPARE(errors::fromHttpStatus(403), ErrorCode::AuthFailed);
    QCOMPARE(errors::fromHttpStatus(404), ErrorCode::ModelNotFound);
    QCOMPARE(errors::fromHttpStatus(429), ErrorCode::RateLimited);
    QCOMPARE(errors::fromHttpStatus(500), ErrorCode::ServerError);
    QCOMPARE(errors::fromHttpStatus(503), ErrorCode::ServerError);
}

void TestErrorCodes::mapsNetworkError()
{
    QCOMPARE(errors::fromNetworkError(4), ErrorCode::RequestTimeout);
    QCOMPARE(errors::fromNetworkError(1), ErrorCode::ConnectionFailed);
}

void TestErrorCodes::buildsErrorObject()
{
    const AppError error = errors::make(ErrorCode::FileWriteFailed, QStringLiteral("/tmp/x"), QString());
    QVERIFY(!error.ok());
    QCOMPARE(error.codeText(), QStringLiteral("0x0003_0002"));
    QVERIFY(error.toString().contains(QStringLiteral("写入文件失败")));
    QVERIFY(error.toString().contains(QStringLiteral("/tmp/x")));

    const AppError custom = errors::make(ErrorCode::Busy, QString(), QStringLiteral("稍等"));
    QCOMPARE(custom.message, QStringLiteral("稍等"));
    QVERIFY(errors::make(ErrorCode::Success).ok());
}

QTEST_GUILESS_MAIN(TestErrorCodes)
