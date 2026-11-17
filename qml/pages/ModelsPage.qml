//! \file ModelsPage.qml
//! \brief 模型页：预设 / Base URL / API Key / 模型 / 温度 / maxTokens / 多配置 / 测试连接。
//!
//! 数据绑定（§7.8）：
//!   modelsVM.presetIndex / baseUrl / apiKey / model / temperature / maxTokens /
//!   profiles（index,name,model,active）、llmReady、testResult、testing
//! 命令：setPresetIndex / setBaseUrl / setApiKey / setModel / setTemperature /
//!       setMaxTokens / testConnection / save / saveAsProfile / applyProfile /
//!       deleteProfile / refresh
//!
//! \note 契约未暴露预设名称列表，presetLabels 与旧版 ConfigStore 预设顺序一致；
//!       若 ViewModel 后续提供 presets，可替换为绑定（见交付报告「契约缺项」）。


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: page

    //! 预设名称（顺序需与 ConfigStore 预设顺序一致）
    property var presetLabels: ["Ollama", "LM Studio", "DeepSeek",
                                "OpenAI", "OpenCode Go", "自定义"]

    function syncFields() {
        baseUrlCard.text = modelsVM.baseUrl
        apiKeyCard.text = modelsVM.apiKey
        modelCard.text = modelsVM.model
    }

    Component.onCompleted: syncFields()

    InputDialog {
        id: profileDialog
        onAcceptedText: function(text) {
            modelsVM.saveAsProfile(text)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("模型")
            description: modelsVM.llmReady
                         ? qsTr("模型已就绪") + (modelsVM.model.length > 0
                                                 ? " · " + modelsVM.model : "")
                         : qsTr("未配置模型，当前回退为本地 mock 回复")

            IconButton {
                iconName: "refresh-cw"
                tooltip: qsTr("刷新配置")
                enabled: !modelsVM.testing
                onClicked: modelsVM.refresh()
            }
            SecondaryButton {
                text: modelsVM.testing ? qsTr("测试中…") : qsTr("测试连接")
                iconName: "zap"
                enabled: !modelsVM.testing
                onClicked: modelsVM.testConnection()
            }
            PrimaryButton {
                text: qsTr("保存")
                iconName: "check"
                onClicked: modelsVM.save()
            }
        }

        PageScroll {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ==============================================================
            // 连接配置
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("连接配置")
                description: qsTr("兼容 OpenAI 接口：Ollama / LM Studio / DeepSeek / OpenAI / 自定义")

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("预设")

                    SelectBox {
                        Layout.preferredWidth: 280
                        options: page.presetLabels
                        currentIndex: modelsVM.presetIndex
                        onActivated: function(index) {
                            modelsVM.setPresetIndex(index)
                        }
                    }
                }

                TextFieldCard {
                    id: baseUrlCard
                    Layout.fillWidth: true
                    label: qsTr("Base URL")
                    placeholder: "http://127.0.0.1:11434/v1"
                    onEdited: function(value) {
                        modelsVM.setBaseUrl(value)
                    }
                }

                TextFieldCard {
                    id: apiKeyCard
                    Layout.fillWidth: true
                    label: qsTr("API Key")
                    placeholder: qsTr("本地服务可留空")
                    hint: qsTr("密钥仅保存在本机配置中")
                    password: true
                    onEdited: function(value) {
                        modelsVM.setApiKey(value)
                    }
                }

                TextFieldCard {
                    id: modelCard
                    Layout.fillWidth: true
                    label: qsTr("模型")
                    placeholder: "qwen2.5:7b / deepseek-chat"
                    onEdited: function(value) {
                        modelsVM.setModel(value)
                    }
                }

                SliderField {
                    Layout.fillWidth: true
                    label: qsTr("Temperature")
                    from: 0
                    to: 2
                    stepSize: 0.05
                    decimals: 2
                    value: modelsVM.temperature
                    onMoved: function(value) {
                        modelsVM.setTemperature(value)
                    }
                }

                SliderField {
                    Layout.fillWidth: true
                    label: qsTr("Max Tokens")
                    from: 256
                    to: 8192
                    stepSize: 64
                    decimals: 0
                    value: modelsVM.maxTokens
                    onMoved: function(value) {
                        modelsVM.setMaxTokens(Math.round(value))
                    }
                }

                // 测试结果提示
                Rectangle {
                    Layout.fillWidth: true
                    visible: modelsVM.testResult.length > 0 || modelsVM.testing
                    implicitHeight: testRow.implicitHeight + Theme.sp3 * 2
                    radius: Theme.radiusControl
                    color: Theme.bgSubtle
                    border.width: 1
                    border.color: Theme.border

                    RowLayout {
                        id: testRow
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp3
                        anchors.rightMargin: Theme.sp3
                        spacing: Theme.sp2

                        Icon {
                            name: modelsVM.testing ? "refresh-cw" : "info"
                            size: Theme.iconMd
                            color: Theme.text2
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelsVM.testing ? qsTr("正在测试连接…")
                                                   : modelsVM.testResult
                            color: Theme.text1
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fsCaption
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }

            // ==============================================================
            // 多配置
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("模型配置")
                description: qsTr("保存多套供应商/模型组合，一键切换")

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp2

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("已保存 %1 套配置").arg(modelsVM.profiles.length)
                        color: Theme.text2
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fsCaption
                    }
                    SecondaryButton {
                        text: qsTr("保存当前为配置")
                        iconName: "plus"
                        onClicked: profileDialog.openFor(qsTr("保存为配置"),
                                                         qsTr("配置名称"), "", false)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp1

                    Repeater {
                        model: modelsVM.profiles

                        delegate: Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 52
                            radius: Theme.radiusControl
                            color: modelData.active === true ? Theme.cardActiveBg
                                                             : Theme.bgSubtle
                            border.width: 1
                            border.color: modelData.active === true
                                          ? Theme.cardActiveBorder : Theme.border

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.sp3
                                anchors.rightMargin: Theme.sp2
                                spacing: Theme.sp2

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        color: Theme.text1
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.fsBody
                                        font.weight: 500
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.model === undefined
                                              ? "" : modelData.model
                                        color: Theme.text2
                                        font.family: Theme.monoFamily
                                        font.pixelSize: Theme.fsCaption
                                        elide: Text.ElideRight
                                    }
                                }

                                Tag {
                                    visible: modelData.active === true
                                    text: qsTr("当前")
                                    tone: "accent"
                                }
                                SecondaryButton {
                                    text: qsTr("应用")
                                    compact: true
                                    enabled: modelData.active !== true
                                    onClicked: modelsVM.applyProfile(index)
                                }
                                IconButton {
                                    iconName: "trash-2"
                                    tooltip: qsTr("删除配置")
                                    danger: true
                                    enabled: modelData.active !== true
                                    onClicked: modelsVM.deleteProfile(index)
                                }
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: modelsVM.profiles.length === 0
                    text: qsTr("暂无保存的配置；配置好上方连接后点「保存当前为配置」。")
                    color: Theme.text3
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fsCaption
                    wrapMode: Text.Wrap
                }
            }
        }
    }

    Connections {
        target: modelsVM

        function onBaseUrlChanged() {
            baseUrlCard.text = modelsVM.baseUrl
        }
        function onApiKeyChanged() {
            apiKeyCard.text = modelsVM.apiKey
        }
        function onModelChanged() {
            modelCard.text = modelsVM.model
        }
    }
}
