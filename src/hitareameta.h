#ifndef HITAREAMETA_H
#define HITAREAMETA_H

#include <QByteArray>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVariantMap>

/// 角色包 meta.json 中的一个命中区（PET-H-03）。
/// rect 为相对桌宠可见区域的归一化矩形（0~1），命中判断按左闭右开处理。
struct HitArea
{
    QString id;
    QString name;
    QRectF rect;
    QStringList lines;
};

/// 解析角色包 meta.json 的命中区、缩放与锚点（PET-H-03）。
///
/// 支持的字段：
/// - hitAreas：对象数组，每项 { id, name?, rect, lines? }；id 必填且唯一；
///   rect 可写 [x, y, w, h] 或 { "x":.., "y":.., "width":.., "height":.. }，均为 0~1 归一化值。
/// - scale：大于 0 的数字，默认 1.0。
/// - anchor：[x, y] 或 { "x":.., "y":.. }，原样读取，默认 (0.5, 0.5)。
/// - 其余字段保存在 meta() 中，原样返回。
///
/// 解析失败时保留原有数据不变，error 写入原因；缺少 hitAreas 视为空表（合法）。
class HitAreaMeta
{
public:
    /// 从 meta.json 文件加载。
    bool loadFromFile(const QString &filePath, QString *error = nullptr);
    /// 从 JSON 内容加载（供测试与内存数据使用）。
    bool loadFromJson(const QByteArray &json, QString *error = nullptr);
    /// 恢复为空表与默认 scale/anchor。
    void clear();

    bool isEmpty() const { return m_areas.isEmpty(); }
    int areaCount() const { return int(m_areas.size()); }
    const QList<HitArea> &areas() const { return m_areas; }

    /// 按 id 查找命中区；未找到返回 nullptr，指针在下次加载/清空后失效。
    const HitArea *find(const QString &id) const;

    /// 归一化坐标（0~1）命中的命中区 id；未命中返回空字符串。
    /// 多个命中区重叠时返回声明顺序靠前者。
    QString hitAreaAt(const QPointF &normalizedPoint) const;

    qreal scale() const { return m_scale; }
    QPointF anchor() const { return m_anchor; }
    /// meta.json 根对象，原样返回（含 scale/anchor/hitAreas 之外的扩展字段）。
    QVariantMap meta() const { return m_meta; }

private:
    QList<HitArea> m_areas;
    qreal m_scale = 1.0;
    QPointF m_anchor{0.5, 0.5};
    QVariantMap m_meta;
};

#endif // HITAREAMETA_H
