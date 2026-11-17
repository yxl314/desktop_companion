#include "personafile.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

namespace
{
const char *const translationContext = "PersonaFile";

QString trText(const char *text)
{
    return QCoreApplication::translate(translationContext, text);
}

void setError(QString *error, const QString &message)
{
    if (error != nullptr)
        *error = message;
}

/// 去掉值两侧成对的引号，并还原双引号内的 \n、\"、\\ 转义。
QString stripQuotes(const QString &raw)
{
    if (raw.size() < 2)
        return raw;
    const QChar quote = raw.front();
    if ((quote != QLatin1Char('"') && quote != QLatin1Char('\'')) || raw.back() != quote)
        return raw;

    QString inner = raw.mid(1, raw.size() - 2);
    if (quote == QLatin1Char('\''))
        return inner;

    QString result;
    result.reserve(inner.size());
    for (qsizetype index = 0; index < inner.size(); ++index) {
        const QChar current = inner.at(index);
        if (current == QLatin1Char('\\') && index + 1 < inner.size()) {
            const QChar escaped = inner.at(++index);
            if (escaped == QLatin1Char('n'))
                result.append(QLatin1Char('\n'));
            else
                result.append(escaped);
        } else {
            result.append(current);
        }
    }
    return result;
}

/// 把值渲染成安全的一行：需要时加双引号并转义。
QString formatValue(const QString &value)
{
    QString text = value.trimmed();
    const bool needsQuotes =
        text.contains(QLatin1Char('\n')) || text.startsWith(QLatin1Char('"')) || text.startsWith(QLatin1Char('\''));
    if (!needsQuotes)
        return text;
    text.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    text.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    text.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    return QLatin1Char('"') + text + QLatin1Char('"');
}

/// 归一化 key：去空白、转小写、空格变下划线，便于中英文混用。
QString normalizeKey(const QString &raw)
{
    QString key = raw.trimmed().toLower();
    key.replace(QLatin1Char(' '), QLatin1Char('_'));
    key.replace(QChar(0x3000), QLatin1Char('_')); // 全角空格
    return key;
}

/// 把 frontmatter 的 key/value 写入对应字段；未知 key 忽略。
void applyField(const QString &rawKey, const QString &value, PersonaData *data)
{
    const QString key = normalizeKey(rawKey);
    if (key == QLatin1String("name") || key == QStringLiteral("称呼") || key == QStringLiteral("名字") ||
        key == QStringLiteral("名称")) {
        data->name = value;
    } else if (key == QLatin1String("birthday") || key == QStringLiteral("生日")) {
        data->birthday = value;
    } else if (key == QLatin1String("zodiac") || key == QStringLiteral("星座")) {
        data->zodiac = value;
    } else if (key == QLatin1String("hobbies") || key == QLatin1String("hobby") || key == QStringLiteral("爱好") ||
               key == QStringLiteral("兴趣")) {
        data->hobbies = value;
    } else if (key == QLatin1String("favorite_food") || key == QLatin1String("favourite_food") ||
               key == QLatin1String("favoritefood") || key == QLatin1String("favouritefood") ||
               key == QStringLiteral("最爱的食物") || key == QStringLiteral("喜欢的食物") ||
               key == QStringLiteral("食物")) {
        data->favoriteFood = value;
    } else if (key == QLatin1String("background") || key == QLatin1String("story") ||
               key == QStringLiteral("背景故事") || key == QStringLiteral("背景")) {
        data->background = value;
    }
}

int findColon(const QString &line)
{
    for (qsizetype index = 0; index < line.size(); ++index) {
        const QChar current = line.at(index);
        if (current == QLatin1Char(':') || current == QChar(0xFF1A)) // 半角 / 全角冒号
            return static_cast<int>(index);
    }
    return -1;
}

void appendField(QStringList *lines, const char *key, const QString &value)
{
    if (value.trimmed().isEmpty())
        return;
    lines->append(QString::fromLatin1(key) + QStringLiteral(": ") + formatValue(value));
}
} // namespace

QString sanitizeFileId(const QString &raw)
{
    QString result;
    result.reserve(raw.size());
    for (const QChar character : raw) {
        if (character.isLetterOrNumber() || character == QLatin1Char('-') || character == QLatin1Char('_'))
            result.append(character);
        else
            result.append(QLatin1Char('_'));
    }

    // 去掉首尾的 '_' 与 '.'（Windows 不允许目录名以点或空格结尾）。
    while (!result.isEmpty() && (result.front() == QLatin1Char('_') || result.front() == QLatin1Char('.')))
        result.remove(0, 1);
    while (!result.isEmpty() && (result.back() == QLatin1Char('_') || result.back() == QLatin1Char('.')))
        result.chop(1);

    // 目录 id 不宜过长，保留前 64 个字符。
    if (result.size() > 64) {
        result.truncate(64);
        while (!result.isEmpty() && (result.back() == QLatin1Char('_') || result.back() == QLatin1Char('.')))
            result.chop(1);
    }
    if (result.isEmpty())
        return {};

    // Windows 保留设备名不能直接做目录名。
    static const QStringList reservedNames = {
        QStringLiteral("CON"),  QStringLiteral("PRN"),  QStringLiteral("AUX"),  QStringLiteral("NUL"),
        QStringLiteral("COM1"), QStringLiteral("COM2"), QStringLiteral("COM3"), QStringLiteral("COM4"),
        QStringLiteral("COM5"), QStringLiteral("COM6"), QStringLiteral("COM7"), QStringLiteral("COM8"),
        QStringLiteral("COM9"), QStringLiteral("LPT1"), QStringLiteral("LPT2"), QStringLiteral("LPT3"),
        QStringLiteral("LPT4"), QStringLiteral("LPT5"), QStringLiteral("LPT6"), QStringLiteral("LPT7"),
        QStringLiteral("LPT8"), QStringLiteral("LPT9")};
    if (reservedNames.contains(result.toUpper()))
        return QLatin1Char('_') + result;
    return result;
}

bool parsePersonaFile(const QString &filePath, PersonaData *data, QString *error)
{
    if (data == nullptr) {
        setError(error, trText("人设输出指针为空"));
        return false;
    }
    *data = PersonaData{};
    if (filePath.isEmpty()) {
        setError(error, trText("人设文件路径为空"));
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(error, trText("无法读取人设文件：%1").arg(file.errorString()));
        return false;
    }
    const QString markdown = QString::fromUtf8(file.readAll());

    const QString fallbackName = QFileInfo(filePath).completeBaseName();
    if (!parsePersonaMarkdown(markdown, fallbackName, data, error))
        return false;
    data->id = fallbackName;
    return true;
}

bool parsePersonaMarkdown(const QString &markdown, const QString &fallbackName, PersonaData *data, QString *error)
{
    if (data == nullptr) {
        setError(error, trText("人设输出指针为空"));
        return false;
    }
    *data = PersonaData{};

    QString text = markdown;
    if (!text.isEmpty() && text.front() == QChar(0xFEFF)) // UTF-8 BOM
        text.remove(0, 1);
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));

    const QStringList lines = text.split(QLatin1Char('\n'));
    qsizetype firstContent = 0;
    while (firstContent < lines.size() && lines.at(firstContent).trimmed().isEmpty())
        ++firstContent;

    // 没有 frontmatter：整文件为正文，文件名作名字。
    if (firstContent >= lines.size() || lines.at(firstContent).trimmed() != QLatin1String("---")) {
        data->body = text.trimmed();
        data->name = fallbackName.trimmed();
        if (data->name.isEmpty()) {
            setError(error, trText("人设名称为空：缺少 frontmatter 且文件名为空"));
            return false;
        }
        return true;
    }

    qsizetype closing = -1;
    for (qsizetype index = firstContent + 1; index < lines.size(); ++index) {
        if (lines.at(index).trimmed() == QLatin1String("---")) {
            closing = index;
            break;
        }
    }
    if (closing < 0) {
        setError(error, trText("frontmatter 缺少结束的 --- 分隔符"));
        return false;
    }

    for (qsizetype index = firstContent + 1; index < closing; ++index) {
        const QString line = lines.at(index).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const int colon = findColon(line);
        if (colon < 0) {
            setError(error, trText("frontmatter 第 %1 行格式非法：缺少冒号").arg(index + 1));
            return false;
        }
        const QString key = line.left(colon).trimmed();
        if (key.isEmpty()) {
            setError(error, trText("frontmatter 第 %1 行格式非法：key 为空").arg(index + 1));
            return false;
        }
        applyField(key, stripQuotes(line.mid(colon + 1).trimmed()), data);
    }

    data->body = lines.mid(closing + 1).join(QLatin1Char('\n')).trimmed();
    if (data->name.isEmpty())
        data->name = fallbackName.trimmed();
    if (data->name.isEmpty()) {
        setError(error, trText("人设名称为空：frontmatter 缺少 name 且文件名为空"));
        return false;
    }
    return true;
}

QString renderPersonaMarkdown(const PersonaData &data)
{
    QStringList lines;
    lines.append(QStringLiteral("---"));
    appendField(&lines, "name", data.name);
    appendField(&lines, "birthday", data.birthday);
    appendField(&lines, "zodiac", data.zodiac);
    appendField(&lines, "hobbies", data.hobbies);
    appendField(&lines, "favorite_food", data.favoriteFood);
    appendField(&lines, "background", data.background);
    lines.append(QStringLiteral("---"));

    const QString body = data.body.trimmed();
    if (!body.isEmpty()) {
        lines.append(QString());
        lines.append(body);
    }
    return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}
