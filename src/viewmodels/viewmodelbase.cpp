/**
 * \file src/viewmodels/viewmodelbase.cpp
 * \brief ViewModel 基类实现：状态/错误/忙碌反馈的赋值与信号派发。
 */

#include "viewmodelbase.h"

#include "services/errorcodes.h"

ViewModelBase::ViewModelBase(QObject *parent)
    : QObject(parent)
{
}

ViewModelBase::~ViewModelBase() = default;

QString ViewModelBase::statusText() const
{
    return m_statusText;
}

QString ViewModelBase::errorText() const
{
    return m_errorText;
}

bool ViewModelBase::busy() const
{
    return m_busy;
}

void ViewModelBase::clearStatus()
{
    setStatus(QString());
    setError(QString());
}

void ViewModelBase::setStatus(const QString &text)
{
    if (m_statusText == text)
        return;
    m_statusText = text;
    emit statusTextChanged();
}

void ViewModelBase::setError(const QString &text)
{
    if (m_errorText == text)
        return;
    m_errorText = text;
    emit errorTextChanged();
}

void ViewModelBase::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void ViewModelBase::reportError(quint32 code, const QString &detail)
{
    const AppError error = errors::make(static_cast<ErrorCode>(code), detail);
    setError(error.toString());
}
