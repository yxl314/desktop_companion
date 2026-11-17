/**
 * \file src/services/characterpack.h
 * \brief 角色包（zip）导入与导出。
 */

#ifndef CHARACTERPACK_H
#define CHARACTERPACK_H

#include <QFuture>
#include <QString>

/**
 * \brief 角色包（zip）：角色身份 + 人设 + 形象 + meta。
 *
 * 包内结构：&lt;characterId&gt;/{card.md, meta.json, avatars/**, personas/*.md}
 */
namespace characterpack
{
/**
 * \brief 同步导出角色包。
 * @param characterDir 角色目录路径。
 * @param zipPath 输出 zip 路径。
 * @param[out] error 非空时写入可读失败原因。
 * @return 是否导出成功。
 */
bool exportPack(const QString &characterDir, const QString &zipPath, QString *error);

/**
 * \brief 同步从 zip 导入角色包到 charactersDir。
 *
 * 缺少人设/形象、路径越界、同名冲突都会失败。
 * @param zipPath 角色包路径。
 * @param charactersDir 角色目录集合路径。
 * @param[out] newCharacterId 非空时写入新角色 id。
 * @param[out] error 非空时写入可读失败原因。
 * @return 是否导入成功。
 */
bool importPack(const QString &zipPath, const QString &charactersDir,
                QString *newCharacterId, QString *error);

/**
 * \brief 异步导出结果。
 */
struct AsyncExportResult
{
    bool ok = false; ///< 是否导出成功。
    QString error;   ///< 失败原因（可读文案，失败时非空）。
};

/**
 * \brief 异步导入结果。
 */
struct AsyncImportResult
{
    bool ok = false;      ///< 是否导入成功。
    QString characterId;  ///< 成功时的角色 id。
    QString error;        ///< 失败原因（可读文案，失败时非空）。
};

/**
 * \brief 异步导出角色包（G-H-04：耗时操作不冻结 UI）。
 *
 * 在全局线程池中执行 exportPack；调用方可配合 QFutureWatcher 或
 * QFuture::then() 回到界面线程处理结果与错误。同步版 exportPack 仍可用。
 * @param characterDir 角色目录路径。
 * @param zipPath 输出 zip 路径。
 * @return 异步结果；fail 信息与同步版一致。
 */
QFuture<AsyncExportResult> exportPackAsync(const QString &characterDir, const QString &zipPath);

/**
 * \brief 异步导入角色包（G-H-04：耗时操作不冻结 UI）。
 *
 * 在全局线程池中执行 importPack，错误码/文案与同步版保持一致。
 * @param zipPath 角色包路径。
 * @param charactersDir 角色目录集合路径。
 * @return 异步结果；成功时包含新角色 id。
 */
QFuture<AsyncImportResult> importPackAsync(const QString &zipPath, const QString &charactersDir);
} // namespace characterpack

#endif // CHARACTERPACK_H
