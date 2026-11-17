/**
 * \file src/viewmodels/modelsettingsviewmodel.cpp
 * \brief ModelSettingsViewModel 实现：参数编辑、保存、多配置与连通性测试。
 */

#include "modelsettingsviewmodel.h"

#include "data/configstore.h"
#include "services/chatservice.h"
#include "services/errorcodes.h"

namespace
{
/// 预设名称 → 默认地址/模型；与旧 Manager.qml 的下拉顺序保持一致。
struct PresetDefaults
{
    const char *name;
    const char *baseUrl;
    const char *model;
};
const PresetDefaults presetTable[] = {
    {"Ollama", "http://127.0.0.1:11434/v1", "qwen2.5:7b"},
    {"LM Studio", "http://127.0.0.1:1234/v1", "local-model"},
    {"DeepSeek", "https://api.deepseek.com/v1", "deepseek-chat"},
    {"OpenAI", "https://api.openai.com/v1", "gpt-4o-mini"},
    {"OpenCode Go", "https://opencode.ai/zen/go/v1", "deepseek-v4.1-flash"},
    {"自定义", "", ""},
};
constexpr int presetCount = int(sizeof(presetTable) / sizeof(presetTable[0]));
} // namespace

ModelSettingsViewModel::ModelSettingsViewModel(ConfigStore *config, ChatService *chat,
                                               QObject *parent)
    : ViewModelBase(parent)
    , m_config(config)
    , m_chat(chat)
{
    Q_ASSERT(m_config != nullptr);
    Q_ASSERT(m_chat != nullptr);

    connect(m_config, &ConfigStore::changed, this, &ModelSettingsViewModel::reloadFromConfig);
    connect(m_chat, &ChatService::testFinished, this, [this](bool ok, const QString &message) {
        m_testing = false;
        emit testingChanged();
        m_testResult = (ok ? QStringLiteral("✅ ") : QStringLiteral("⚠️ ")) + message;
        emit testResultChanged();
    });

    reloadFromConfig();
}

ModelSettingsViewModel::~ModelSettingsViewModel() = default;

int ModelSettingsViewModel::presetIndex() const
{
    return m_presetIndex;
}

QString ModelSettingsViewModel::baseUrl() const
{
    return m_baseUrl;
}

QString ModelSettingsViewModel::apiKey() const
{
    return m_apiKey;
}

QString ModelSettingsViewModel::model() const
{
    return m_model;
}

double ModelSettingsViewModel::temperature() const
{
    return m_temperature;
}

int ModelSettingsViewModel::maxTokens() const
{
    return m_maxTokens;
}

QVariantList ModelSettingsViewModel::profiles() const
{
    return m_profiles;
}

bool ModelSettingsViewModel::llmReady() const
{
    return m_llmReady;
}

QString ModelSettingsViewModel::testResult() const
{
    return m_testResult;
}

bool ModelSettingsViewModel::testing() const
{
    return m_testing;
}

void ModelSettingsViewModel::setPresetIndex(int index)
{
    const int bounded = qBound(0, index, presetCount - 1);
    if (m_presetIndex == bounded)
        return;
    m_presetIndex = bounded;
    emit presetIndexChanged();
    applyPresetDefaults(bounded);
}

void ModelSettingsViewModel::setBaseUrl(const QString &url)
{
    if (m_baseUrl == url)
        return;
    m_baseUrl = url;
    emit baseUrlChanged();
    const bool ready = !m_baseUrl.isEmpty() && !m_model.isEmpty();
    if (m_llmReady != ready) {
        m_llmReady = ready;
        emit llmReadyChanged();
    }
}

void ModelSettingsViewModel::setApiKey(const QString &key)
{
    if (m_apiKey == key)
        return;
    m_apiKey = key;
    emit apiKeyChanged();
}

void ModelSettingsViewModel::setModel(const QString &name)
{
    if (m_model == name)
        return;
    m_model = name;
    emit modelChanged();
    const bool ready = !m_baseUrl.isEmpty() && !m_model.isEmpty();
    if (m_llmReady != ready) {
        m_llmReady = ready;
        emit llmReadyChanged();
    }
}

void ModelSettingsViewModel::setTemperature(double value)
{
    if (qFuzzyCompare(m_temperature, value))
        return;
    m_temperature = value;
    emit temperatureChanged();
}

void ModelSettingsViewModel::setMaxTokens(int value)
{
    if (m_maxTokens == value)
        return;
    m_maxTokens = value;
    emit maxTokensChanged();
}

void ModelSettingsViewModel::testConnection()
{
    m_testing = true;
    emit testingChanged();
    m_testResult = tr("测试中…");
    emit testResultChanged();
    m_chat->testConnection(m_baseUrl.trimmed(), m_apiKey.trimmed(), m_model.trimmed());
}

void ModelSettingsViewModel::save()
{
    // setLlm() 的 changed() 会同步 reloadFromConfig() 覆盖编辑中的生成参数，先存局部值。
    const double temperature = m_temperature;
    const int maxTokens = m_maxTokens;
    m_config->setLlm(presetNames().value(m_presetIndex), m_baseUrl.trimmed(), m_apiKey.trimmed(),
                     m_model.trimmed());
    m_config->setGeneration(temperature, maxTokens);
    QString error;
    if (!m_config->save(&error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    setStatus(tr("模型设置已保存"));
}

void ModelSettingsViewModel::saveAsProfile(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), tr("配置名称不能为空"));
        return;
    }

    QVector<ModelProfile> profileList = m_config->values().modelProfiles;
    int index = -1;
    for (int i = 0; i < profileList.size(); ++i) {
        if (profileList.at(i).name == trimmed) {
            index = i;
            break;
        }
    }

    ModelProfile profile;
    profile.name = trimmed;
    profile.preset = presetNames().value(m_presetIndex);
    profile.baseUrl = m_baseUrl.trimmed();
    profile.apiKey = m_apiKey.trimmed();
    profile.model = m_model.trimmed();
    profile.temperature = m_temperature;
    profile.maxTokens = m_maxTokens;
    if (index >= 0) {
        profileList[index] = profile;
    } else {
        profileList.append(profile);
        index = profileList.size() - 1;
    }

    m_config->setModelProfiles(profileList, index);
    QString error;
    if (!m_config->save(&error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    setStatus(tr("已保存模型配置：%1").arg(trimmed));
}

void ModelSettingsViewModel::applyProfile(int index)
{
    const QVector<ModelProfile> profileList = m_config->values().modelProfiles;
    if (index < 0 || index >= profileList.size())
        return;
    const ModelProfile &profile = profileList.at(index);

    m_presetIndex = presetIndexForName(profile.preset);
    m_baseUrl = profile.baseUrl;
    m_apiKey = profile.apiKey;
    m_model = profile.model;
    m_temperature = profile.temperature;
    m_maxTokens = profile.maxTokens;

    m_config->setLlm(profile.preset, profile.baseUrl, profile.apiKey, profile.model);
    m_config->setGeneration(profile.temperature, profile.maxTokens);
    m_config->setModelProfiles(profileList, index);
    QString error;
    if (!m_config->save(&error))
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);

    emit presetIndexChanged();
    emit baseUrlChanged();
    emit apiKeyChanged();
    emit modelChanged();
    emit temperatureChanged();
    emit maxTokensChanged();
    setStatus(tr("已应用模型配置：%1").arg(profile.name));
}

void ModelSettingsViewModel::deleteProfile(int index)
{
    QVector<ModelProfile> profileList = m_config->values().modelProfiles;
    if (index < 0 || index >= profileList.size())
        return;

    profileList.removeAt(index);
    int active = m_config->values().activeProfile;
    if (active == index)
        active = -1;
    else if (active > index)
        --active;
    m_config->setModelProfiles(profileList, active);
    QString error;
    if (!m_config->save(&error))
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
    setStatus(tr("已删除模型配置"));
}

void ModelSettingsViewModel::refresh()
{
    reloadFromConfig();
}

void ModelSettingsViewModel::reloadFromConfig()
{
    const AppConfig &values = m_config->values();

    m_presetIndex = presetIndexForName(values.preset);
    m_baseUrl = values.baseUrl;
    m_apiKey = values.apiKey;
    m_model = values.model;
    m_temperature = values.temperature;
    m_maxTokens = values.maxTokens;

    emit presetIndexChanged();
    emit baseUrlChanged();
    emit apiKeyChanged();
    emit modelChanged();
    emit temperatureChanged();
    emit maxTokensChanged();

    const bool ready = !m_baseUrl.isEmpty() && !m_model.isEmpty();
    if (m_llmReady != ready) {
        m_llmReady = ready;
        emit llmReadyChanged();
    }

    rebuildProfiles();
}

void ModelSettingsViewModel::rebuildProfiles()
{
    QVariantList rows;
    const int active = m_config->values().activeProfile;
    const QVector<ModelProfile> profileList = m_config->values().modelProfiles;
    for (int i = 0; i < profileList.size(); ++i) {
        QVariantMap item;
        item.insert(QStringLiteral("index"), i);
        item.insert(QStringLiteral("name"), profileList.at(i).name);
        item.insert(QStringLiteral("model"), profileList.at(i).model);
        item.insert(QStringLiteral("active"), i == active);
        rows.append(item);
    }
    m_profiles = rows;
    emit profilesChanged();
}

void ModelSettingsViewModel::applyPresetDefaults(int index)
{
    if (index < 0 || index >= presetCount)
        return;
    if (index == presetCount - 1)
        return; // 「自定义」保留用户当前输入。

    const PresetDefaults &preset = presetTable[index];
    setBaseUrl(QString::fromLatin1(preset.baseUrl));
    setModel(QString::fromLatin1(preset.model));
    setApiKey(QString());
}

QStringList ModelSettingsViewModel::presetNames()
{
    QStringList names;
    for (const PresetDefaults &preset : presetTable)
        names.append(QString::fromUtf8(preset.name));
    return names;
}

int ModelSettingsViewModel::presetIndexForName(const QString &name)
{
    const QStringList names = presetNames();
    for (int i = 0; i < names.size(); ++i) {
        if (names.at(i).compare(name, Qt::CaseInsensitive) == 0)
            return i;
    }
    // 兼容旧配置里的小写/无空格写法（如 "ollama"、"lmstudio"）。
    QString normalized = name;
    normalized.remove(QLatin1Char(' ')).remove(QLatin1Char('-')).remove(QLatin1Char('_'));
    for (int i = 0; i < names.size(); ++i) {
        QString candidate = names.at(i);
        candidate.remove(QLatin1Char(' ')).remove(QLatin1Char('-')).remove(QLatin1Char('_'));
        if (candidate.compare(normalized, Qt::CaseInsensitive) == 0)
            return i;
    }
    return presetCount - 1; // 未识别的一律按「自定义」。
}
