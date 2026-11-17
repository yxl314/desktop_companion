/**
 * \file src/viewmodels/modelsettingsviewmodel.h
 * \brief 模型设置 ViewModel：连接参数、生成参数、多配置与连通性测试。
 */

#ifndef MODELSETTINGSVIEWMODEL_H
#define MODELSETTINGSVIEWMODEL_H

#include <QObject>
#include <QVariantList>

#include "viewmodelbase.h"

class ChatService;
class ConfigStore;

/**
 * \brief 模型设置页 ViewModel。
 *
 * 编辑区属性（baseUrl/apiKey/model/temperature/maxTokens）在 setter 中只改内存，
 * 由 save() 统一写入 ConfigStore；presetIndex 对应内置预设列表（Ollama / LM Studio /
 * DeepSeek / OpenAI / OpenCode Go / 自定义）。testConnection() 委托 ChatService 做
 * /models 连通性检查，结果通过 testResult / testing 反馈。
 */
class ModelSettingsViewModel : public ViewModelBase
{
    Q_OBJECT
    /** \brief 预设下标：0 Ollama … 5 自定义。 */
    Q_PROPERTY(int presetIndex READ presetIndex WRITE setPresetIndex NOTIFY presetIndexChanged)
    /** \brief 接口根地址。 */
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    /** \brief 接口密钥。 */
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    /** \brief 模型名。 */
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY modelChanged)
    /** \brief 采样温度。 */
    Q_PROPERTY(double temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged)
    /** \brief 单次回复最大 token 数。 */
    Q_PROPERTY(int maxTokens READ maxTokens WRITE setMaxTokens NOTIFY maxTokensChanged)
    /** \brief 多配置列表（行字段：index / name / model / active）。 */
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY profilesChanged)
    /** \brief 模型是否已配置。 */
    Q_PROPERTY(bool llmReady READ llmReady NOTIFY llmReadyChanged)
    /** \brief 连通性测试结果文本。 */
    Q_PROPERTY(QString testResult READ testResult NOTIFY testResultChanged)
    /** \brief 是否正在测试连通性。 */
    Q_PROPERTY(bool testing READ testing NOTIFY testingChanged)

public:
    /**
     * \brief 构造模型设置 ViewModel。
     * \param config 配置仓库，必须非空。
     * \param chat 对话服务，必须非空（连通性测试）。
     * \param parent 父对象。
     */
    explicit ModelSettingsViewModel(ConfigStore *config, ChatService *chat,
                                    QObject *parent = nullptr);

    /// 析构函数。
    ~ModelSettingsViewModel() override;

    /// \brief 预设下标。
    int presetIndex() const;
    /// \brief 接口根地址。
    QString baseUrl() const;
    /// \brief 接口密钥。
    QString apiKey() const;
    /// \brief 模型名。
    QString model() const;
    /// \brief 采样温度。
    double temperature() const;
    /// \brief 最大 token 数。
    int maxTokens() const;
    /// \brief 多配置列表。
    QVariantList profiles() const;
    /// \brief 模型是否已配置。
    bool llmReady() const;
    /// \brief 连通性测试结果。
    QString testResult() const;
    /// \brief 是否正在测试。
    bool testing() const;

    /**
     * \brief 切换预设并填充该预设的默认地址/模型。
     * \param index 预设下标，越界时夹到 0~5。
     */
    Q_INVOKABLE void setPresetIndex(int index);
    /**
     * \brief 设置接口根地址。
     * \param url 新地址。
     */
    Q_INVOKABLE void setBaseUrl(const QString &url);
    /**
     * \brief 设置接口密钥。
     * \param key 新密钥。
     */
    Q_INVOKABLE void setApiKey(const QString &key);
    /**
     * \brief 设置模型名。
     * \param name 新模型名。
     */
    Q_INVOKABLE void setModel(const QString &name);
    /**
     * \brief 设置采样温度。
     * \param value 新温度。
     */
    Q_INVOKABLE void setTemperature(double value);
    /**
     * \brief 设置最大 token 数。
     * \param value 新上限。
     */
    Q_INVOKABLE void setMaxTokens(int value);
    /// 测试当前连接参数（异步）。
    Q_INVOKABLE void testConnection();
    /// 保存当前连接与生成参数。
    Q_INVOKABLE void save();
    /**
     * \brief 把当前参数另存为命名配置。
     * \param name 配置名称；同名时覆盖。
     */
    Q_INVOKABLE void saveAsProfile(const QString &name);
    /**
     * \brief 应用一套命名配置。
     * \param index 配置下标。
     */
    Q_INVOKABLE void applyProfile(int index);
    /**
     * \brief 删除一套命名配置。
     * \param index 配置下标。
     */
    Q_INVOKABLE void deleteProfile(int index);
    /// 从配置仓库重新载入。
    Q_INVOKABLE void refresh();

signals:
    /// presetIndex 属性变化信号。
    void presetIndexChanged();
    /// baseUrl 属性变化信号。
    void baseUrlChanged();
    /// apiKey 属性变化信号。
    void apiKeyChanged();
    /// model 属性变化信号。
    void modelChanged();
    /// temperature 属性变化信号。
    void temperatureChanged();
    /// maxTokens 属性变化信号。
    void maxTokensChanged();
    /// profiles 属性变化信号。
    void profilesChanged();
    /// llmReady 属性变化信号。
    void llmReadyChanged();
    /// testResult 属性变化信号。
    void testResultChanged();
    /// testing 属性变化信号。
    void testingChanged();

private:
    /// 从 ConfigStore 重新载入编辑区与列表。
    void reloadFromConfig();
    /// 重建多配置列表。
    void rebuildProfiles();
    /**
     * \brief 根据预设下标填充默认连接参数。
     * \param index 预设下标。
     */
    void applyPresetDefaults(int index);
    /// 预设名称列表。
    static QStringList presetNames();
    /// 预设名对应的下标。
    static int presetIndexForName(const QString &name);

    ConfigStore *m_config = nullptr; ///< 配置仓库。
    ChatService *m_chat = nullptr;   ///< 对话服务。

    int m_presetIndex = 0;       ///< 预设下标。
    QString m_baseUrl;           ///< 编辑中的地址。
    QString m_apiKey;            ///< 编辑中的密钥。
    QString m_model;             ///< 编辑中的模型。
    double m_temperature = 0.8;  ///< 编辑中的温度。
    int m_maxTokens = 0;         ///< 编辑中的 token 上限。
    bool m_llmReady = false;     ///< 就绪缓存。
    QVariantList m_profiles;     ///< 多配置行缓存。
    QString m_testResult;        ///< 测试结果。
    bool m_testing = false;      ///< 测试中标记。
};

#endif // MODELSETTINGSVIEWMODEL_H
