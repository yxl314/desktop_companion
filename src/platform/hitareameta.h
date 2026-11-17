/**
 * \file src/platform/hitareameta.h
 * \brief 角色包 meta.json 的命中区、缩放与锚点解析（PET-H-03）。
 */

#ifndef HITAREAMETA_H
#define HITAREAMETA_H

#include <QByteArray>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVariantMap>

/**
 * \brief 角色包 meta.json 中的一个命中区（PET-H-03）。
 *
 * rect 为相对桌宠可见区域的归一化矩形（0~1），命中判断按左闭右开处理。
 */
struct HitArea
{
    QString id;        ///< 命中区 id（必填且唯一）。
    QString name;      ///< 显示名。
    QRectF rect;       ///< 归一化矩形（0~1）。
    QStringList lines; ///< 命中后显示的台词。
};

/**
 * \brief 解析角色包 meta.json 的命中区、缩放与锚点（PET-H-03）。
 *
 * 支持的字段：
 * - hitAreas：对象数组，每项 { id, name?, rect, lines? }；id 必填且唯一；
 *   rect 可写 [x, y, w, h] 或 { "x":.., "y":.., "width":.., "height":.. }，均为 0~1 归一化值。
 * - scale：大于 0 的数字，默认 1.0。
 * - anchor：[x, y] 或 { "x":.., "y":.. }，原样读取，默认 (0.5, 0.5)。
 * - 其余字段保存在 meta() 中，原样返回。
 *
 * 解析失败时保留原有数据不变，error 写入原因；缺少 hitAreas 视为空表（合法）。
 */
class HitAreaMeta
{
public:
    /**
     * \brief 从 meta.json 文件加载。
     * @param filePath meta.json 路径。
     * @param[out] error 非空时写入失败原因。
     * @return 是否加载成功。
     */
    bool loadFromFile(const QString &filePath, QString *error = nullptr);
    /**
     * \brief 从 JSON 内容加载（供测试与内存数据使用）。
     * @param json JSON 字节内容。
     * @param[out] error 非空时写入失败原因。
     * @return 是否加载成功。
     */
    bool loadFromJson(const QByteArray &json, QString *error = nullptr);
    /**
     * \brief 恢复为空表与默认 scale/anchor。
     */
    void clear();

    /// 是否没有任何命中区。
    bool isEmpty() const { return m_areas.isEmpty(); }
    /// 命中区数量。
    int areaCount() const { return int(m_areas.size()); }
    /// 全部命中区（声明顺序）。
    const QList<HitArea> &areas() const { return m_areas; }

    /**
     * \brief 按 id 查找命中区。
     * @param id 命中区 id。
     * @return 命中区指针；未找到返回 nullptr，指针在下次加载/清空后失效。
     */
    const HitArea *find(const QString &id) const;

    /**
     * \brief 判断归一化坐标命中的命中区。
     *
     * 多个命中区重叠时返回声明顺序靠前者。
     * @param normalizedPoint 归一化坐标（0~1）。
     * @return 命中区 id；未命中返回空字符串。
     */
    QString hitAreaAt(const QPointF &normalizedPoint) const;

    /// 缩放系数。
    qreal scale() const { return m_scale; }
    /// 归一化锚点。
    QPointF anchor() const { return m_anchor; }
    /// meta.json 根对象，原样返回（含 scale/anchor/hitAreas 之外的扩展字段）。
    QVariantMap meta() const { return m_meta; }

private:
    QList<HitArea> m_areas; ///< 命中区列表。
    qreal m_scale = 1.0;    ///< 缩放系数。
    QPointF m_anchor{0.5, 0.5}; ///< 归一化锚点。
    QVariantMap m_meta;     ///< 原始 meta 根对象。
};

#endif // HITAREAMETA_H
