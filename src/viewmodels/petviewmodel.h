/**
 * \file src/viewmodels/petviewmodel.h
 * \brief 桌宠 ViewModel：多桌宠布局、点击意图、气泡与命中区台词。
 */

#ifndef PETVIEWMODEL_H
#define PETVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>

#include "viewmodelbase.h"

class CharacterStore;
class ConfigStore;
class IntimacyService;

/**
 * \brief 桌宠窗口 ViewModel。
 *
 * \c pets 的每一行是 QVariantMap，字段为
 * index / characterId / name / avatar / x / y / visible / scale。
 * 点击坐标约定为相对桌宠可见区域的归一化坐标（0~1），
 * 由 PetWindow 在 QML 侧换算后传入；命中区台词与好感度累加在本 VM 内完成。
 */
class PetViewModel : public ViewModelBase
{
    Q_OBJECT
    /** \brief 多桌宠布局列表（行字段见类注释）。 */
    Q_PROPERTY(QVariantList pets READ pets NOTIFY petsChanged)
    /** \brief 桌宠数量（1~3）。 */
    Q_PROPERTY(int petCount READ petCount NOTIFY petCountChanged)
    /** \brief 当前交互中的桌宠下标。 */
    Q_PROPERTY(int activePetIndex READ activePetIndex NOTIFY activePetIndexChanged)
    /** \brief 是否启用待机动作。 */
    Q_PROPERTY(bool idleMotion READ idleMotion NOTIFY idleMotionChanged)
    /** \brief 待机动作幅度（0.2~2.0）。 */
    Q_PROPERTY(double idleAmplitude READ idleAmplitude NOTIFY idleAmplitudeChanged)
    /** \brief 是否开启点击穿透。 */
    Q_PROPERTY(bool clickThrough READ clickThrough NOTIFY clickThroughChanged)
    /** \brief 是否贴边隐藏。 */
    Q_PROPERTY(bool edgeHide READ edgeHide NOTIFY edgeHideChanged)
    /** \brief 是否显示命中区调试。 */
    Q_PROPERTY(bool debugHitAreas READ debugHitAreas NOTIFY debugHitAreasChanged)
    /** \brief 当前气泡文本。 */
    Q_PROPERTY(QString bubbleText READ bubbleText NOTIFY bubbleTextChanged)
    /** \brief 气泡是否可见。 */
    Q_PROPERTY(bool bubbleVisible READ bubbleVisible NOTIFY bubbleVisibleChanged)

public:
    /**
     * \brief 构造桌宠 ViewModel。
     * \param config 配置仓库，必须非空。
     * \param characters 角色仓库，必须非空。
     * \param intimacy 好感度仓库，必须非空。
     * \param parent 父对象。
     */
    explicit PetViewModel(ConfigStore *config, CharacterStore *characters,
                          IntimacyService *intimacy, QObject *parent = nullptr);

    /// 析构函数。
    ~PetViewModel() override;

    /// \brief 桌宠布局列表。
    QVariantList pets() const;
    /// \brief 桌宠数量。
    int petCount() const;
    /// \brief 当前交互桌宠下标。
    int activePetIndex() const;
    /// \brief 待机动作开关。
    bool idleMotion() const;
    /// \brief 待机动作幅度。
    double idleAmplitude() const;
    /// \brief 点击穿透开关。
    bool clickThrough() const;
    /// \brief 贴边隐藏开关。
    bool edgeHide() const;
    /// \brief 命中区调试开关。
    bool debugHitAreas() const;
    /// \brief 当前气泡文本。
    QString bubbleText() const;
    /// \brief 气泡是否可见。
    bool bubbleVisible() const;

    /**
     * \brief 记录桌宠位置并持久化。
     * \param index 桌宠下标。
     * \param x 横坐标（窗口坐标）。
     * \param y 纵坐标（窗口坐标）。
     */
    Q_INVOKABLE void movePet(int index, double x, double y);
    /**
     * \brief 点击桌宠：命中区台词、气泡、好感度与角色切换请求。
     * \param index 桌宠下标。
     * \param x 归一化横坐标（0~1）。
     * \param y 归一化纵坐标（0~1）。
     */
    Q_INVOKABLE void tapPet(int index, double x, double y);
    /**
     * \brief 双击桌宠。
     * \param index 桌宠下标。
     */
    Q_INVOKABLE void doubleTapPet(int index);
    /**
     * \brief 请求桌宠右键菜单。
     * \param index 桌宠下标。
     */
    Q_INVOKABLE void contextMenu(int index);
    /**
     * \brief 拖拽结束：持久化最终位置。
     * \param index 桌宠下标。
     */
    Q_INVOKABLE void dragFinished(int index);
    /**
     * \brief 读取当前桌宠角色的命中区矩形（归一化，供调试叠加）。
     * \param index 桌宠下标；越界或未绑定角色时回退当前角色。
     * \return 命中区列表，每项为 {id,x,y,w,h}（归一化坐标）。
     */
    Q_INVOKABLE QVariantList hitAreaRects(int index) const;
    /**
     * \brief 显示气泡。
     * \param text 气泡文本；4.2 秒后自动隐藏。
     */
    Q_INVOKABLE void showBubble(const QString &text);
    /// 立即隐藏气泡。
    Q_INVOKABLE void hideBubble();

signals:
    /// pets 属性变化信号。
    void petsChanged();
    /// petCount 属性变化信号。
    void petCountChanged();
    /// activePetIndex 属性变化信号。
    void activePetIndexChanged();
    /// idleMotion 属性变化信号。
    void idleMotionChanged();
    /// idleAmplitude 属性变化信号。
    void idleAmplitudeChanged();
    /// clickThrough 属性变化信号。
    void clickThroughChanged();
    /// edgeHide 属性变化信号。
    void edgeHideChanged();
    /// debugHitAreas 属性变化信号。
    void debugHitAreasChanged();
    /// bubbleText 属性变化信号。
    void bubbleTextChanged();
    /// bubbleVisible 属性变化信号。
    void bubbleVisibleChanged();
    /// 请求 Application 把当前角色切换为指定 id（桌宠绑定角色与当前角色不同）。
    void characterActivated(const QString &id);
    /// 请求 shell 处理桌宠双击（附加信号，契约外集成项）。
    void doubleTapRequested(int index);
    /// 请求 shell 在指定桌宠上弹出右键菜单（附加信号，契约外集成项）。
    void contextMenuRequested(int index);

private:
    /// 重新构建多桌宠布局。
    void rebuild();
    /// 从配置同步桌宠行为缓存并发信号。
    void syncFromConfig();
    /**
     * \brief 指定桌宠绑定的角色 id。
     * \param index 桌宠下标。
     * \return 绑定角色 id；未绑定时回退当前角色。
     */
    QString petCharacterId(int index) const;
    /**
     * \brief 设置当前交互桌宠下标并发信号。
     * \param index 桌宠下标。
     */
    void setActivePetIndex(int index);

    ConfigStore *m_config = nullptr;       ///< 配置仓库。
    CharacterStore *m_characters = nullptr;///< 角色仓库。
    IntimacyService *m_intimacy = nullptr; ///< 好感度仓库。

    QVariantList m_pets;      ///< 桌宠布局缓存。
    int m_petCount = 1;       ///< 数量缓存。
    int m_activePetIndex = 0; ///< 当前交互下标。
    bool m_idleMotion = true; ///< 待机动作缓存。
    double m_idleAmplitude = 1.0; ///< 待机幅度缓存。
    bool m_clickThrough = true;   ///< 点击穿透缓存。
    bool m_edgeHide = true;       ///< 贴边隐藏缓存。
    bool m_debugHitAreas = false; ///< 命中区调试缓存。
    QString m_bubbleText;         ///< 气泡文本。
    bool m_bubbleVisible = false; ///< 气泡显隐。
    QTimer m_bubbleHideTimer;     ///< 气泡自动隐藏定时器。
};

#endif // PETVIEWMODEL_H
