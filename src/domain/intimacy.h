/**
 * \file src/domain/intimacy.h
 * \brief 好感度领域的值类型：等级分档与一次好感度变化事件。
 */

#ifndef DOMAIN_INTIMACY_H
#define DOMAIN_INTIMACY_H

#include <QString>

/**
 * \brief 好感度等级，由分数按阈值分档得到。
 *
 * 分档阈值见 data/IntimacyService 的 familiarThreshold / closeThreshold /
 * trustedThreshold；对外展示名由 IntimacyService::levelName 提供。
 */
enum class IntimacyLevel
{
    Stranger, ///< 陌生。
    Familiar, ///< 熟悉。
    Close,    ///< 亲近。
    Trusted   ///< 信赖。
};

/**
 * \brief 一条好感度变化事件，用于控制中心展示与跨天衰减追溯。
 */
struct IntimacyEvent
{
    QString at;     ///< 发生时间（ISO 8601）。
    int delta = 0;  ///< 本次变化量，可为负。
    int score = 0;  ///< 变化后的分数（0~100）。
    QString reason; ///< 可读原因。
};

#endif // DOMAIN_INTIMACY_H
