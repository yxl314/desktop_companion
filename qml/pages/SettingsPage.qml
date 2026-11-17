//! \file SettingsPage.qml
//! \brief 设置页：外观 / 行为 / 桌宠 / 模式 / 快捷键 / 更新 / 系统与数据。
//!
//! 数据绑定：appVM（§7.2）与 charactersVM.characters（桌宠角色选择）。
//! 主题与语言切换即时生效（语言由 Application 决定是否重启）。
//!
//! \signal aboutRequested() 请求主窗口显示「关于」页（关于页不在 0~8 页导航中）


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: page

    signal aboutRequested()

    //! 角色下拉选项（id 与名字一一对应）
    readonly property var characterNames: charactersVM.characters.map(function(item) {
        return item.name
    })
    readonly property var characterIds: charactersVM.characters.map(function(item) {
        return item.id
    })

    function activeCharacterIndex() {
        const index = page.characterIds.indexOf(appVM.activePetCharacterId)
        return index >= 0 ? index : 0
    }

    function changeLanguage(index) {
        appVM.setLanguageIndex(index)
        if (dialogs.confirm(qsTr("语言切换"),
                            qsTr("语言切换需要重启应用后生效，是否立即重启？")))
            appVM.restartApp()
    }

    function requestRestart() {
        if (dialogs.confirm(qsTr("重启应用"), qsTr("确定立即重启应用吗？")))
            appVM.restartApp()
    }

    Component.onCompleted: updateUrlCard.text = appVM.updateUrl

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("设置")
            description: qsTr("外观、行为、快捷键、更新与数据")

            SecondaryButton {
                text: qsTr("检查更新")
                iconName: "refresh-cw"
                onClicked: appVM.checkUpdate()
            }
            SecondaryButton {
                text: qsTr("关于本软件")
                iconName: "info"
                onClicked: page.aboutRequested()
            }
        }

        PageScroll {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ==============================================================
            // 外观
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("外观")
                description: qsTr("主题与语言即时切换；语言由系统托盘重启策略决定")

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("主题")

                    SegmentedControl {
                        Layout.preferredWidth: 300
                        options: [qsTr("浅色"), qsTr("深色"), qsTr("跟随系统")]
                        currentIndex: appVM.themeIndex
                        onActivated: function(index) {
                            appVM.setThemeIndex(index)
                        }
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("语言")

                    SegmentedControl {
                        Layout.preferredWidth: 300
                        options: [qsTr("中文"), "English", qsTr("跟随系统")]
                        currentIndex: appVM.languageIndex
                        onActivated: function(index) {
                            page.changeLanguage(index)
                        }
                    }
                }
            }

            // ==============================================================
            // 桌宠与行为
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("桌宠与行为")
                description: qsTr("桌宠窗口独立于主窗口，可随时启动或停止")

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("桌宠运行")

                    Text {
                        Layout.fillWidth: true
                        text: appVM.petStatusText
                        color: Theme.text2
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fsCaption
                    }
                    SecondaryButton {
                        text: qsTr("启动")
                        compact: true
                        enabled: !appVM.petVisible
                        onClicked: appVM.launchPet()
                    }
                    SecondaryButton {
                        text: qsTr("停止")
                        compact: true
                        enabled: appVM.petVisible
                        onClicked: appVM.stopPet()
                    }
                    ToggleSwitch {
                        Layout.alignment: Qt.AlignVCenter
                        checked: appVM.petVisible
                        onClicked: appVM.setPetVisible(!appVM.petVisible)
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("桌宠数量")

                    SegmentedControl {
                        Layout.preferredWidth: 240
                        options: [qsTr("1 只"), qsTr("2 只"), qsTr("3 只")]
                        currentIndex: Math.max(0, Math.min(2, appVM.petCount - 1))
                        onActivated: function(index) {
                            appVM.setPetCount(index + 1)
                        }
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("桌宠角色")
                    hint: qsTr("开机自动启动与「启动桌宠」使用的角色")

                    SelectBox {
                        Layout.preferredWidth: 240
                        options: page.characterNames
                        placeholder: qsTr("暂无角色")
                        currentIndex: page.activeCharacterIndex()
                        onActivated: function(index) {
                            if (index >= 0 && index < page.characterIds.length)
                                appVM.setActivePetCharacter(page.characterIds[index])
                        }
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("开机自动启动桌宠")

                    ToggleSwitch {
                        Layout.alignment: Qt.AlignVCenter
                        checked: appVM.autoStartPet
                        onClicked: appVM.setAutoStartPet(!appVM.autoStartPet)
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("贴边隐藏")
                    hint: qsTr("拖到屏幕左右边缘时收起（顶部不隐藏）")

                    ToggleSwitch {
                        Layout.alignment: Qt.AlignVCenter
                        checked: appVM.edgeHide
                        onClicked: appVM.setEdgeHide(!appVM.edgeHide)
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("透明区域点击穿透")
                    hint: qsTr("按形象 alpha 生成窗口区域，透明处不挡鼠标")

                    ToggleSwitch {
                        Layout.alignment: Qt.AlignVCenter
                        checked: appVM.clickThrough
                        onClicked: appVM.setClickThrough(!appVM.clickThrough)
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("待机浮动")

                    ToggleSwitch {
                        Layout.alignment: Qt.AlignVCenter
                        checked: appVM.idleMotion
                        onClicked: appVM.setIdleMotion(!appVM.idleMotion)
                    }
                }

                SliderField {
                    Layout.fillWidth: true
                    label: qsTr("浮动幅度")
                    from: 0.2
                    to: 2.0
                    stepSize: 0.1
                    decimals: 1
                    value: appVM.idleAmplitude
                    onMoved: function(value) {
                        appVM.setIdleAmplitude(value)
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("命中区调试")
                    hint: qsTr("在桌宠窗口叠加命中区域，便于核对形象 meta")

                    ToggleSwitch {
                        Layout.alignment: Qt.AlignVCenter
                        checked: appVM.debugHitAreas
                        onClicked: appVM.setDebugHitAreas(!appVM.debugHitAreas)
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("悬浮球")
                    hint: qsTr("隐藏桌宠时显示悬浮球，点击恢复桌宠")

                    ToggleSwitch {
                        Layout.alignment: Qt.AlignVCenter
                        checked: appVM.ballVisible
                        onClicked: appVM.setBallVisible(!appVM.ballVisible)
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("好感度跨天衰减")

                    ToggleSwitch {
                        Layout.alignment: Qt.AlignVCenter
                        checked: appVM.intimacyDecay
                        onClicked: appVM.setIntimacyDecay(!appVM.intimacyDecay)
                    }
                }
            }

            // ==============================================================
            // 模式与主动对话
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("模式与主动对话")
                description: qsTr("勿扰模式会暂停主动互动与待机动效")

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("模式")

                    SegmentedControl {
                        Layout.preferredWidth: 280
                        options: [qsTr("休闲"), qsTr("工作"), qsTr("勿扰")]
                        currentIndex: appVM.mode
                        onActivated: function(index) {
                            appVM.setMode(index)
                        }
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("前台自动模式")
                    hint: qsTr("按前台应用自动切换工作 / 休闲模式")

                    ToggleSwitch {
                        Layout.alignment: Qt.AlignVCenter
                        checked: appVM.autoMode
                        onClicked: appVM.setAutoMode(!appVM.autoMode)
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("主动对话频率")

                    SegmentedControl {
                        Layout.preferredWidth: 280
                        options: [qsTr("关"), qsTr("低"), qsTr("中"), qsTr("高")]
                        currentIndex: Math.max(0, Math.min(3, appVM.proactiveFrequency))
                        onActivated: function(index) {
                            appVM.setProactiveFrequency(index)
                        }
                    }
                }
            }

            // ==============================================================
            // 快捷键
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("快捷键")
                description: qsTr("聊天输入：Enter 发送，Shift+Enter 换行")

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp2

                    Repeater {
                        model: [
                            { "keys": "Ctrl+Alt+P", "text": qsTr("显示 / 隐藏桌宠") },
                            { "keys": "Ctrl+Alt+C", "text": qsTr("打开主窗口聊天页") },
                            { "keys": "Ctrl+Alt+M", "text": qsTr("切换勿扰模式") }
                        ]

                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp3

                            Rectangle {
                                implicitWidth: 108
                                implicitHeight: 24
                                radius: Theme.radiusControl
                                color: Theme.bgSubtle
                                border.width: 1
                                border.color: Theme.border

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.keys
                                    color: Theme.text2
                                    font.family: Theme.monoFamily
                                    font.pixelSize: Theme.fsBadge
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.text
                                color: Theme.text1
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fsBody
                            }
                        }
                    }
                }
            }

            // ==============================================================
            // 更新
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("更新")
                description: qsTr("更新清单为 JSON：version / url / notes")

                TextFieldCard {
                    id: updateUrlCard
                    Layout.fillWidth: true
                    label: qsTr("更新地址")
                    placeholder: "https://example.com/update.json"
                    hint: qsTr("留空则关闭自动更新检查")
                    onEdited: function(value) {
                        // 保存动作由下方「保存地址」显式触发，避免每次输入都落盘
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp2

                    SecondaryButton {
                        text: qsTr("保存地址")
                        onClicked: appVM.setUpdateUrl(updateUrlCard.text.trim())
                    }
                    SecondaryButton {
                        text: qsTr("检查更新")
                        iconName: "refresh-cw"
                        onClicked: appVM.checkUpdate()
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: appVM.updateStatus.length > 0
                    text: appVM.updateStatus
                    color: Theme.text2
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fsCaption
                    wrapMode: Text.Wrap
                }
            }

            // ==============================================================
            // 系统与数据
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("系统与数据")
                description: qsTr("应用级设置与本地数据入口")

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("应用开机自启")

                    ToggleSwitch {
                        Layout.alignment: Qt.AlignVCenter
                        checked: appVM.autostart
                        onClicked: appVM.setAutostart(!appVM.autostart)
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("新手引导")

                    SecondaryButton {
                        text: qsTr("重新显示")
                        onClicked: appVM.setOnboardingDone(false)
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("重启应用")

                    SecondaryButton {
                        text: qsTr("立即重启")
                        onClicked: page.requestRestart()
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("数据目录")

                    SecondaryButton {
                        text: qsTr("打开数据目录")
                        iconName: "folder-open"
                        onClicked: appVM.openDataDir()
                    }
                    SecondaryButton {
                        text: qsTr("打开日志目录")
                        onClicked: appVM.openLogsDir()
                    }
                    SecondaryButton {
                        text: qsTr("导出日志")
                        iconName: "external-link"
                        onClicked: appVM.exportLogs()
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("关于")

                    SecondaryButton {
                        text: qsTr("关于本软件")
                        iconName: "info"
                        onClicked: page.aboutRequested()
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("版本 %1").arg(appVM.appVersion)
                        color: Theme.text3
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fsCaption
                    }
                }
            }
        }
    }

    Connections {
        target: appVM

        function onUpdateUrlChanged() {
            updateUrlCard.text = appVM.updateUrl
        }
    }
}
