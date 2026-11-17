/**
 * \file src/viewmodels/personasviewmodel.h
 * \brief 人设页 ViewModel：当前角色的人设列表、编辑、导入与切换。
 */

#ifndef PERSONASVIEWMODEL_H
#define PERSONASVIEWMODEL_H

#include <QObject>
#include <QVariantList>

#include "viewmodelbase.h"

class CharacterStore;
class ChatService;
class ConfigStore;
class DialogService;
class IntimacyService;
class PersonaStore;

/**
 * \brief 人设页 ViewModel。
 *
 * 列表语义与旧版一致：当前角色存在时 = 角色卡默认人设（id 为空）+ 该角色
 * personas/ 下的多套人设；没有角色时回退到全局 PersonaStore。
 * save() 通过 CharacterStore::saveCard / savePersona 落盘；select() 只切换
 * ConfigStore::personaId，对话上下文由 ChatViewModel 监听配置变化后重建。
 */
class PersonasViewModel : public ViewModelBase
{
    Q_OBJECT
    /** \brief 人设列表（行字段：id / name / current）。 */
    Q_PROPERTY(QVariantList personas READ personas NOTIFY personasChanged)
    /** \brief 当前人设显示名。 */
    Q_PROPERTY(QString currentName READ currentName NOTIFY currentNameChanged)
    /** \brief 编辑中的名字。 */
    Q_PROPERTY(QString editName READ editName WRITE setEditName NOTIFY editNameChanged)
    /** \brief 编辑中的正文。 */
    Q_PROPERTY(QString editBody READ editBody WRITE setEditBody NOTIFY editBodyChanged)
    /** \brief 是否处于编辑态。 */
    Q_PROPERTY(bool editing READ editing NOTIFY editingChanged)
    /** \brief 是否预览渲染结果。 */
    Q_PROPERTY(bool preview READ preview NOTIFY previewChanged)
    /** \brief 编辑中的人设 id；空串表示角色卡默认人设。 */
    Q_PROPERTY(QString editId READ editId NOTIFY editIdChanged)
    /** \brief 当前角色好感度等级名。 */
    Q_PROPERTY(QString intimacyLevel READ intimacyLevel NOTIFY intimacyLevelChanged)

public:
    /**
     * \brief 构造人设页 ViewModel。
     * \param personas 全局人设仓库，必须非空。
     * \param chat 对话服务，必须非空。
     * \param config 配置仓库，必须非空。
     * \param dialogs 对话框服务，必须非空。
     * \param characters 角色仓库（可选；为空时只编辑全局人设）。
     * \param intimacy 好感度仓库（可选；用于 intimacyLevel 展示）。
     * \param parent 父对象。
     */
    explicit PersonasViewModel(PersonaStore *personas, ChatService *chat, ConfigStore *config,
                               DialogService *dialogs, CharacterStore *characters = nullptr,
                               IntimacyService *intimacy = nullptr, QObject *parent = nullptr);

    /// 析构函数。
    ~PersonasViewModel() override;

    /// \brief 人设列表。
    QVariantList personas() const;
    /// \brief 当前人设显示名。
    QString currentName() const;
    /// \brief 编辑中的名字。
    QString editName() const;
    /// \brief 编辑中的正文。
    QString editBody() const;
    /// \brief 是否处于编辑态。
    bool editing() const;
    /// \brief 是否预览。
    bool preview() const;
    /// \brief 编辑中的人设 id。
    QString editId() const;
    /// \brief 当前角色好感度等级名。
    QString intimacyLevel() const;

    /// 新建一套空白人设并进入编辑态。
    Q_INVOKABLE void createNew();
    /**
     * \brief 载入指定人设到编辑区。
     * \param id 人设 id；空串表示角色卡默认人设。
     */
    Q_INVOKABLE void loadFor(const QString &id);
    /// 保存当前编辑内容。
    Q_INVOKABLE void save();
    /// 从 Markdown 文件导入人设。
    Q_INVOKABLE void importMd();
    /**
     * \brief 切换当前人设。
     * \param id 人设 id；空串表示角色卡默认人设。
     */
    Q_INVOKABLE void select(const QString &id);
    /**
     * \brief 删除一套人设。
     * \param id 人设 id。
     */
    Q_INVOKABLE void remove(const QString &id);
    /**
     * \brief 设置编辑中的名字。
     * \param name 新名字。
     */
    Q_INVOKABLE void setEditName(const QString &name);
    /**
     * \brief 设置编辑中的正文。
     * \param body 新正文。
     */
    Q_INVOKABLE void setEditBody(const QString &body);
    /// 切换预览态。
    Q_INVOKABLE void togglePreview();

signals:
    /// personas 属性变化信号。
    void personasChanged();
    /// currentName 属性变化信号。
    void currentNameChanged();
    /// editName 属性变化信号。
    void editNameChanged();
    /// editBody 属性变化信号。
    void editBodyChanged();
    /// editing 属性变化信号。
    void editingChanged();
    /// preview 属性变化信号。
    void previewChanged();
    /// editId 属性变化信号。
    void editIdChanged();
    /// intimacyLevel 属性变化信号。
    void intimacyLevelChanged();

private:
    /// 重新构建人设列表与当前名。
    void rebuild();
    /// 把指定人设载入编辑区（内部实现）。
    void loadPersona(const QString &id);
    /// 当前角色 id；没有角色时返回空串。
    QString characterId() const;
    /// 当前好感度等级名。
    QString currentIntimacyLevel() const;

    PersonaStore *m_personas = nullptr;     ///< 全局人设仓库。
    ChatService *m_chat = nullptr;          ///< 对话服务。
    ConfigStore *m_config = nullptr;        ///< 配置仓库。
    DialogService *m_dialogs = nullptr;     ///< 对话框服务。
    CharacterStore *m_characters = nullptr; ///< 角色仓库（可选）。
    IntimacyService *m_intimacy = nullptr;  ///< 好感度仓库（可选）。

    QVariantList m_personasCache; ///< 人设行缓存。
    QString m_currentName;        ///< 当前人设名缓存。
    QString m_editName;           ///< 编辑名字。
    QString m_editBody;           ///< 编辑正文。
    bool m_editing = false;       ///< 编辑态。
    bool m_preview = false;       ///< 预览态。
    QString m_editId;             ///< 编辑人设 id。
    bool m_creatingNew = false;   ///< 是否是「新建人设」流程（保存时生成新 id）。
    QString m_intimacyLevel;      ///< 好感度等级缓存。
};

#endif // PERSONASVIEWMODEL_H
