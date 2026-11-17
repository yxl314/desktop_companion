/**
 * \file src/viewmodels/dialogservice.cpp
 * \brief DialogService 的 QFileDialog / QMessageBox 默认实现。
 */

#include "dialogservice.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

namespace
{
/// 文件选择类对话框的默认起始目录：文档目录。
QString documentsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}
} // namespace

DialogService::DialogService(QObject *parent)
    : QObject(parent)
{
}

DialogService::~DialogService() = default;

QString DialogService::openFile(const QString &title, const QString &filter)
{
    return QFileDialog::getOpenFileName(nullptr, title, documentsDir(), filter);
}

QStringList DialogService::openFiles(const QString &title, const QString &filter)
{
    return QFileDialog::getOpenFileNames(nullptr, title, documentsDir(), filter);
}

QString DialogService::openDirectory(const QString &title)
{
    return QFileDialog::getExistingDirectory(nullptr, title, documentsDir());
}

QString DialogService::saveFile(const QString &title, const QString &filter,
                                const QString &defaultPath)
{
    const QString start = defaultPath.isEmpty() ? documentsDir() : defaultPath;
    return QFileDialog::getSaveFileName(nullptr, title, start, filter);
}

bool DialogService::confirm(const QString &title, const QString &text)
{
    const QMessageBox::StandardButton answer =
        QMessageBox::question(nullptr, title, text, QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No);
    return answer == QMessageBox::Yes;
}

void DialogService::info(const QString &title, const QString &text)
{
    QMessageBox::information(nullptr, title, text);
}
