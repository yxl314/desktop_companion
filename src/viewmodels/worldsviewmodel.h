/**
 * \file src/viewmodels/worldsviewmodel.h
 * \brief 世界观页 ViewModel：世界观列表、编辑、导入与角色归属统计。
 */

#ifndef WORLDSVIEWMODEL_H
#define WORLDSVIEWMODEL_H

#include <QObject>
#include <QVariantList>

#include "viewmodelbase.h"

class CharacterStore;
class ConfigStore;
class DialogService;
class WorldStore;

/**
 * \brief 世界观页 ViewModel。
 *
 * \c worlds 的每一行是 QVariantMap，字段为 id / name / bodyPreview / characterCount。
 * 编辑区由 editId / editName / editBody 驱动，save() 统一落盘；导入走 .md 文件。
 */
class WorldsViewModel : public ViewModelBase
{
    Q_OBJECT
    /** \brief 世界观列表（行字段见类注释）。 */
    Q_PROPERTY(QVariantList worlds READ worlds NOTIFY worldsChanged)
    /** \brief 编辑中的世界观 id；空串表示新建。 */
    Q_PROPERTY(QString editId READ editId NOTIFY editIdChanged)
    /** \brief 编辑中的显示名。 */
    Q_PROPERTY(QString editName READ editName WRITE setEditName NOTIFY editNameChanged)
    /** \brief 编辑中的正文。 */
    Q_PROPERTY(QString editBody READ editBody WRITE setEditBody NOTIFY editBodyChanged)
    /** \brief 是否处于编辑态。 */
    Q_PROPERTY(bool editing READ editing NOTIFY editingChanged)
    /** \brief 是否预览正文。 */
    Q_PROPERTY(bool preview READ preview NOTIFY previewChanged)

public:
    /**
     * \brief 构造世界观页 ViewModel。
     * \param worlds 世界观仓库，必须非空。
     * \param characters 角色仓库，必须非空（用于统计归属角色数）。
     * \param dialogs 对话框服务，必须非空（导入选文件）。
     * \param parent 父对象。
     */
    explicit WorldsViewModel(WorldStore *worlds, CharacterStore *characters,
                             DialogService *dialogs, QObject *parent = nullptr);

    /**
     * \brief 构造世界观页 ViewModel（带配置仓库，供 assignToCurrent 确定当前角色）。
     * \param worlds 世界观仓库，必须非空。
     * \param characters 角色仓库，必须非空。
     * \param dialogs 对话框服务，必须非空。
     * \param config 配置仓库，用于读取当前角色 id。
     * \param parent 父对象。
     */
    WorldsViewModel(WorldStore *worlds, CharacterStore *characters, DialogService *dialogs,
                    ConfigStore *config, QObject *parent = nullptr);

    /// 析构函数。
    ~WorldsViewModel() override;

    /// \brief 世界观列表。
    QVariantList worlds() const;
    /// \brief 编辑中的世界观 id。
    QString editId() const;
    /// \brief 编辑中的显示名。
    QString editName() const;
    /// \brief 编辑中的正文。
    QString editBody() const;
    /// \brief 是否处于编辑态。
    bool editing() const;
    /// \brief 是否预览。
    bool preview() const;

    /// 新建一个空白世界观并进入编辑态。
    Q_INVOKABLE void createNew();
    /**
     * \brief 载入指定世界观到编辑区。
     * \param id 世界观 id。
     */
    Q_INVOKABLE void loadFor(const QString &id);
    /// 保存当前编辑内容（新建或覆盖）。
    Q_INVOKABLE void save();
    /// 从 Markdown 文件导入世界观（异步，完成后自动刷新）。
    Q_INVOKABLE void importFile();
    /**
     * \brief 把当前角色归属到指定世界观（空串表示取消归属）。
     * \param worldId 世界观 id。
     */
    Q_INVOKABLE void assignToCurrent(const QString &worldId);
    /**
     * \brief 重命名世界观。
     * \param id 世界观 id。
     * \param name 新显示名。
     */
    Q_INVOKABLE void rename(const QString &id, const QString &name);
    /**
     * \brief 删除世界观。
     * \param id 世界观 id。
     */
    Q_INVOKABLE void remove(const QString &id);
    /// 切换预览态。
    Q_INVOKABLE void togglePreview();
    /**
     * \brief 设置编辑中的显示名。
     * \param name 新显示名。
     */
    Q_INVOKABLE void setEditName(const QString &name);
    /**
     * \brief 设置编辑中的正文。
     * \param body 新正文。
     */
    Q_INVOKABLE void setEditBody(const QString &body);

signals:
    /// worlds 属性变化信号。
    void worldsChanged();
    /// editId 属性变化信号。
    void editIdChanged();
    /// editName 属性变化信号。
    void editNameChanged();
    /// editBody 属性变化信号。
    void editBodyChanged();
    /// editing 属性变化信号。
    void editingChanged();
    /// preview 属性变化信号。
    void previewChanged();

private:
    /// 重新构建世界观列表。
    void rebuild();

    WorldStore *m_worlds = nullptr;         ///< 世界观仓库。
    CharacterStore *m_characters = nullptr; ///< 角色仓库。
    DialogService *m_dialogs = nullptr;     ///< 对话框服务。
    ConfigStore *m_config = nullptr;        ///< 配置仓库（可空，用于确定当前角色）。

    QVariantList m_worldsCache; ///< 世界观行缓存。
    QString m_editId;           ///< 编辑 id。
    QString m_editName;         ///< 编辑名字。
    QString m_editBody;         ///< 编辑正文。
    bool m_editing = false;     ///< 编辑态。
    bool m_preview = false;     ///< 预览态。
};

#endif // WORLDSVIEWMODEL_H
