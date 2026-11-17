#include "platform/hitareameta.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSet>

#include <cmath>
#include <utility>

namespace
{
constexpr qreal rectTolerance = 1e-6;

bool fail(QString *error, const QString &message)
{
    if (error != nullptr)
        *error = message;
    return false;
}

bool readNumber(const QJsonValue &value, qreal *number)
{
    if (!value.isDouble())
        return false;
    const qreal result = value.toDouble();
    if (!std::isfinite(result))
        return false;
    *number = result;
    return true;
}

bool parseRect(const QJsonValue &value, QRectF *rect, QString *error, const QString &context)
{
    qreal x = 0.0;
    qreal y = 0.0;
    qreal width = 0.0;
    qreal height = 0.0;
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        if (array.size() != 4 || !readNumber(array.at(0), &x) || !readNumber(array.at(1), &y)
            || !readNumber(array.at(2), &width) || !readNumber(array.at(3), &height))
            return fail(error, QStringLiteral("%1：rect 必须是 4 个数字的数组").arg(context));
    } else if (value.isObject()) {
        const QJsonObject object = value.toObject();
        if (!readNumber(object.value(QStringLiteral("x")), &x) || !readNumber(object.value(QStringLiteral("y")), &y)
            || !readNumber(object.value(QStringLiteral("width")), &width)
            || !readNumber(object.value(QStringLiteral("height")), &height))
            return fail(error, QStringLiteral("%1：rect 对象必须包含 x/y/width/height 数字").arg(context));
    } else {
        return fail(error, QStringLiteral("%1：rect 必须是数组或对象").arg(context));
    }

    if (width <= 0.0 || height <= 0.0)
        return fail(error, QStringLiteral("%1：rect 宽高必须大于 0").arg(context));
    if (x < -rectTolerance || y < -rectTolerance || x + width > 1.0 + rectTolerance
        || y + height > 1.0 + rectTolerance)
        return fail(error, QStringLiteral("%1：rect 必须归一化到 0~1 范围").arg(context));

    *rect = QRectF(x, y, width, height);
    return true;
}

bool parseLines(const QJsonValue &value, QStringList *lines, QString *error, const QString &context)
{
    if (value.isUndefined() || value.isNull())
        return true;
    if (!value.isArray())
        return fail(error, QStringLiteral("%1：lines 必须是字符串数组").arg(context));
    const QJsonArray array = value.toArray();
    for (const auto &entry : array) {
        if (!entry.isString())
            return fail(error, QStringLiteral("%1：lines 必须是字符串数组").arg(context));
        lines->append(entry.toString());
    }
    return true;
}

bool parseArea(const QJsonValue &value, int index, HitArea *area, QString *error)
{
    const QString base = QStringLiteral("hitAreas[%1]").arg(index);
    if (!value.isObject())
        return fail(error, QStringLiteral("%1 必须是对象").arg(base));

    const QJsonObject object = value.toObject();
    HitArea result;
    result.id = object.value(QStringLiteral("id")).toString().trimmed();
    if (result.id.isEmpty())
        return fail(error, QStringLiteral("%1 缺少非空 id").arg(base));

    const QString context = QStringLiteral("%1(%2)").arg(base, result.id);
    result.name = object.value(QStringLiteral("name")).toString(result.id);
    if (!parseRect(object.value(QStringLiteral("rect")), &result.rect, error, context))
        return false;
    if (!parseLines(object.value(QStringLiteral("lines")), &result.lines, error, context))
        return false;

    *area = std::move(result);
    return true;
}

bool parseAnchor(const QJsonValue &value, QPointF *anchor, QString *error)
{
    qreal x = 0.0;
    qreal y = 0.0;
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        if (array.size() != 2 || !readNumber(array.at(0), &x) || !readNumber(array.at(1), &y))
            return fail(error, QStringLiteral("meta.json 的 anchor 必须是 [x, y] 数组或 {x, y} 对象"));
    } else if (value.isObject()) {
        const QJsonObject object = value.toObject();
        if (!readNumber(object.value(QStringLiteral("x")), &x) || !readNumber(object.value(QStringLiteral("y")), &y))
            return fail(error, QStringLiteral("meta.json 的 anchor 必须是 [x, y] 数组或 {x, y} 对象"));
    } else {
        return fail(error, QStringLiteral("meta.json 的 anchor 必须是 [x, y] 数组或 {x, y} 对象"));
    }
    *anchor = QPointF(x, y);
    return true;
}
} // namespace

bool HitAreaMeta::loadFromFile(const QString &filePath, QString *error)
{
    if (filePath.isEmpty())
        return fail(error, QStringLiteral("meta.json 路径为空"));

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return fail(error, QStringLiteral("无法打开 meta.json：%1").arg(file.errorString()));
    const QByteArray data = file.readAll();
    if (file.error() != QFileDevice::NoError)
        return fail(error, QStringLiteral("读取 meta.json 失败：%1").arg(file.errorString()));
    return loadFromJson(data, error);
}

bool HitAreaMeta::loadFromJson(const QByteArray &json, QString *error)
{
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return fail(error,
                    QStringLiteral("meta.json 解析失败：%1（偏移 %2）")
                        .arg(parseError.errorString())
                        .arg(parseError.offset));
    if (!document.isObject())
        return fail(error, QStringLiteral("meta.json 根节点必须是对象"));

    const QJsonObject root = document.object();

    QList<HitArea> areas;
    const QJsonValue areasValue = root.value(QStringLiteral("hitAreas"));
    if (!areasValue.isUndefined() && !areasValue.isNull()) {
        if (!areasValue.isArray())
            return fail(error, QStringLiteral("meta.json 的 hitAreas 必须是数组"));
        const QJsonArray array = areasValue.toArray();
        const int areaCount = int(array.size());
        areas.reserve(areaCount);
        QSet<QString> ids;
        for (int i = 0; i < areaCount; ++i) {
            HitArea area;
            if (!parseArea(array.at(i), i, &area, error))
                return false;
            if (ids.contains(area.id))
                return fail(error, QStringLiteral("meta.json 的 hitAreas id 重复：%1").arg(area.id));
            ids.insert(area.id);
            areas.append(std::move(area));
        }
    }

    qreal scale = 1.0;
    const QJsonValue scaleValue = root.value(QStringLiteral("scale"));
    if (!scaleValue.isUndefined() && !scaleValue.isNull()) {
        if (!readNumber(scaleValue, &scale) || scale <= 0.0)
            return fail(error, QStringLiteral("meta.json 的 scale 必须是大于 0 的数字"));
    }

    QPointF anchor(0.5, 0.5);
    const QJsonValue anchorValue = root.value(QStringLiteral("anchor"));
    if (!anchorValue.isUndefined() && !anchorValue.isNull()) {
        if (!parseAnchor(anchorValue, &anchor, error))
            return false;
    }

    m_areas = std::move(areas);
    m_scale = scale;
    m_anchor = anchor;
    m_meta = root.toVariantMap();
    return true;
}

void HitAreaMeta::clear()
{
    m_areas.clear();
    m_scale = 1.0;
    m_anchor = QPointF(0.5, 0.5);
    m_meta.clear();
}

const HitArea *HitAreaMeta::find(const QString &id) const
{
    for (const HitArea &area : m_areas) {
        if (area.id == id)
            return &area;
    }
    return nullptr;
}

QString HitAreaMeta::hitAreaAt(const QPointF &normalizedPoint) const
{
    if (normalizedPoint.x() < 0.0 || normalizedPoint.x() > 1.0 || normalizedPoint.y() < 0.0
        || normalizedPoint.y() > 1.0)
        return {};

    for (const HitArea &area : m_areas) {
        const QRectF &rect = area.rect;
        if (normalizedPoint.x() >= rect.x() && normalizedPoint.x() < rect.x() + rect.width()
            && normalizedPoint.y() >= rect.y() && normalizedPoint.y() < rect.y() + rect.height())
            return area.id;
    }
    return {};
}
