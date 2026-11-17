/**
 * \file src/viewmodels/charactersviewmodel.h
 * \brief 角色页 ViewModel：角色列表、形象/角色包导入导出与桌宠启动。
 */

#ifndef CHARACTERSVIEWMODEL_H
#define CHARACTERSVIEWMODEL_H

#include <QObject>
#include <QVariantList>

#include "viewmodelbase.h"

class CharacterStore;
class ConfigStore;
class DialogService;
class WorldStore;

/**
 * \brief 角色页 ViewModel。
 *
 * \c characters 的每一行是 QVariantMap，字段为
 * id / name / avatar / worldId / worldName / enabled / current / isPet。
 * 写操作（新建/重命名/删除/启用/切换）直接调用 CharacterStore / ConfigStore，
 * 完成后刷新列表并通过信号回到 QML。
 */
class CharactersViewModel : public ViewModelBase
{
    Q_OBJECT
    /** \brief 角色列表（行字段见类注释）。 */
    Q_PROPERTY(QVariantList characters READ characters NOTIFY charactersChanged)
    /** \brief 空列表提示文案。 */
    Q_PROPERTY(QString emptyHint READ emptyHint NOTIFY emptyHintChanged)

public:
    /**
     * \brief 构造角色页 ViewModel。
     * \param characters 角色仓库，必须非空。
     * \param worlds 世界观仓库，必须非空（用于补齐 worldName）。
     * \param config 配置仓库，必须非空。
     * \param dialogs 对话框服务，必须非空（导入/导出选文件）。
     * \param parent 父对象。
     */
    explicit CharactersViewModel(CharacterStore *characters, WorldStore *worlds,
                                 ConfigStore *config, DialogService *dialogs,
                                 QObject *parent = nullptr);

    /// 析构函数。
    ~CharactersViewModel() override;

    /// \brief 角色列表。
    QVariantList characters() const;
    /// \brief 空列表提示文案。
    QString emptyHint() const;

    /**
     * \brief 新建角色并设为当前角色。
     * \param name 角色显示名。
     */
    Q_INVOKABLE void create(const QString &name);
    /// 为当前角色导入形象图片。
    Q_INVOKABLE void importAvatar();
    /// 从 zip 导入角色包。
    Q_INVOKABLE void importPack();
    /**
     * \brief 导出角色包到 zip。
     * \param id 角色 id；为空时使用当前角色。
     */
    Q_INVOKABLE void exportPack(const QString &id);
    /**
     * \brief 重命名角色。
     * \param id 角色 id。
     * \param name 新显示名。
     */
    Q_INVOKABLE void rename(const QString &id, const QString &name);
    /**
     * \brief 删除角色（当前角色被删除时清空选择）。
     * \param id 角色 id。
     */
    Q_INVOKABLE void remove(const QString &id);
    /**
     * \brief 切换当前角色。
     * \param id 角色 id。
     */
    Q_INVOKABLE void select(const QString &id);
    /**
     * \brief 启用/停用角色。
     * \param id 角色 id。
     * \param enabled 是否启用。
     */
    Q_INVOKABLE void setEnabled(const QString &id, bool enabled);
    /**
     * \brief 把角色启动为桌宠（设置为 0 号桌宠角色）。
     * \param id 角色 id。
     */
    Q_INVOKABLE void launchAsPet(const QString &id);
    /// 重新扫描角色目录并刷新列表。
    Q_INVOKABLE void refresh();

signals:
    /// characters 属性变化信号。
    void charactersChanged();
    /// emptyHint 属性变化信号。
    void emptyHintChanged();
    /// 请求 shell 用指定角色启动桌宠（附加信号，契约外集成项）。
    void launchPetRequested(const QString &id);

private:
    /// 重新构建角色列表与空提示。
    void rebuild();

    CharacterStore *m_characters = nullptr; ///< 角色仓库。
    WorldStore *m_worlds = nullptr;         ///< 世界观仓库。
    ConfigStore *m_config = nullptr;        ///< 配置仓库。
    DialogService *m_dialogs = nullptr;     ///< 对话框服务。
    QVariantList m_charactersCache;         ///< 角色行缓存。
    QString m_emptyHint;                    ///< 空提示缓存。
};

#endif // CHARACTERSVIEWMODEL_H
