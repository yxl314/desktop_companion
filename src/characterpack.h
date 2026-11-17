#ifndef CHARACTERPACK_H
#define CHARACTERPACK_H

#include <QString>

/// 角色包（zip）：角色身份 + 人设 + 形象 + meta。
/// 包内结构：<characterId>/{card.md, meta.json, avatars/**, personas/*.md}
namespace characterpack
{
/// 把角色目录打成 zip；失败通过 error 返回可读原因。
bool exportPack(const QString &characterDir, const QString &zipPath, QString *error);

/// 从 zip 导入到 charactersDir；成功时返回新角色 id。
/// 缺少人设/形象、路径越界、同名冲突都会失败。
bool importPack(const QString &zipPath, const QString &charactersDir,
                QString *newCharacterId, QString *error);
} // namespace characterpack

#endif // CHARACTERPACK_H
