//! \file ChatPage.qml
//! \brief 聊天页（专注模式）：760px 居中消息区、卡片式输入区、流式/重试/回溯。
//!
//! 数据绑定（docs/架构重构-MVVM.md §7.3 与 §7.14）：
//!   chatVM.messages / streaming / chatTitle / modelSummary / llmReady /
//!   welcomeText / quickPrompts / intimacyScore / intimacyLevel / errorText / busy
//! 命令：send / retry / stop / restart / clear / backtrack / setFeedback /
//!       toggleExpanded / exportChat / openModelSettings
//!
//! 交互：Enter 发送、Shift+Enter 换行；流式时显示停止；失败显示重试；
//!       空会话展示 welcomeText + quickPrompts 推荐问句；```card 消息可折叠。


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: page

    //! 发送输入框内容
    function sendCurrent() {
        const text = composer.text.trim()
        if (text.length === 0 || chatVM.busy || chatVM.streaming)
            return
        composer.text = ""
        chatVM.send(text)
    }

    //! 点击推荐问句：填入输入框并聚焦
    function quickAsk(text) {
        composer.text = text
        composer.forceActiveFocus()
    }

    //! 回溯确认（dialogs 抽象，不直接弹 QWidget 对话框）
    function requestBacktrack(messageIndex) {
        if (dialogs.confirm(qsTr("回溯对话"),
                            qsTr("回溯到这条消息，其后的消息与关联记忆会被清除，确定继续吗？")))
            chatVM.backtrack(messageIndex)
    }

    function requestClear() {
        if (dialogs.confirm(qsTr("清空对话"), qsTr("确定清空当前对话的聊天记录吗？")))
            chatVM.clear()
    }

    function requestRestart() {
        if (dialogs.confirm(qsTr("重启对话"),
                            qsTr("重启对话会保留聊天记录，但重置当前人设的记忆与上下文，确定继续吗？")))
            chatVM.restart()
    }

    function scrollToEnd() {
        messageList.positionViewAtEnd()
    }

    Timer {
        id: scrollTimer
        interval: 60
        onTriggered: page.scrollToEnd()
    }

    Connections {
        target: chatVM
        function onMessagesChanged() { scrollTimer.restart() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ==================================================================
        // 页头：标题 / 模型摘要 / 导出·清空·重启 / 模型设置
        // ==================================================================
        PageHeader {
            Layout.fillWidth: true
            title: chatVM.chatTitle.length > 0 ? chatVM.chatTitle : qsTr("聊天")
            description: {
                const parts = []
                if (chatVM.modelSummary.length > 0)
                    parts.push(chatVM.modelSummary)
                if (chatVM.intimacyLevel.length > 0)
                    parts.push(qsTr("好感度 %1（%2）").arg(chatVM.intimacyLevel).arg(chatVM.intimacyScore))
                return parts.join(" · ")
            }

            IconButton {
                iconName: "refresh-cw"
                tooltip: qsTr("刷新对话")
                onClicked: chatVM.restart()
            }
            SecondaryButton {
                text: qsTr("导出")
                iconName: "external-link"
                onClicked: chatVM.exportChat()
            }
            SecondaryButton {
                text: qsTr("清空")
                onClicked: page.requestClear()
            }
            SecondaryButton {
                text: qsTr("重启")
                iconName: "rotate-ccw"
                onClicked: page.requestRestart()
            }
            PrimaryButton {
                text: qsTr("模型设置")
                iconName: "settings"
                onClicked: chatVM.openModelSettings()
            }
        }

        // ==================================================================
        // 模型未就绪提示
        // ==================================================================
        Rectangle {
            Layout.fillWidth: true
            visible: !chatVM.llmReady
            implicitHeight: 44
            color: Theme.warningSoft

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.pagePadding
                anchors.rightMargin: Theme.pagePadding
                spacing: Theme.sp2

                Icon {
                    name: "alert-triangle"
                    size: Theme.iconMd
                    color: Theme.warningText
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("未配置模型，当前使用本地 mock 回复。")
                    color: Theme.warningText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fsCaption
                    elide: Text.ElideRight
                }
                SecondaryButton {
                    text: qsTr("去配置")
                    compact: true
                    onClicked: chatVM.openModelSettings()
                }
            }
        }

        // ==================================================================
        // 消息区：内容最大 760px 居中
        // ==================================================================
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                id: centerColumn
                width: Math.min(760, Math.max(0, page.width - Theme.pagePadding * 2))
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.topMargin: Theme.sp3
                anchors.bottomMargin: Theme.sp3
                spacing: Theme.sp3

                // 消息列表
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ListView {
                        id: messageList
                        anchors.fill: parent
                        clip: true
                        model: chatVM.messages
                        spacing: Theme.bubbleSpacing
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                            width: 8
                        }

                        onCountChanged: scrollTimer.restart()
                        onContentHeightChanged: scrollTimer.restart()

                        delegate: Item {
                            id: messageItem
                            width: messageList.width
                            height: contentColumn.implicitHeight

                            readonly property bool isUser: modelData.role === "user"
                            readonly property bool isCard: modelData.card === true
                            readonly property bool expanded: modelData.expanded === true
                            readonly property int feedbackValue: modelData.feedback === undefined ? 0
                                                                                                  : modelData.feedback
                            readonly property color bubbleColor: isUser ? Theme.bubbleUserBg
                                                                        : Theme.bubbleAssistantBg
                            //! 气泡最大宽度：内容区 560 或消息列表 72%（专注模式阅读宽度）
                            readonly property real maxBubbleWidth: Math.min(560,
                                                                            messageList.width * 0.72)

                            // 仅用于测量文本自然宽度（不参与布局/绘制）
                            Text {
                                id: naturalText
                                visible: false
                                text: messageItem.isCard || modelData.text === undefined
                                      ? "" : modelData.text
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fsBody
                                wrapMode: Text.NoWrap
                            }

                            Column {
                                id: contentColumn
                                width: parent.width
                                spacing: Theme.sp1

                                Row {
                                    width: parent.width
                                    layoutDirection: messageItem.isUser ? Qt.RightToLeft
                                                                       : Qt.LeftToRight
                                    spacing: Theme.sp2

                                    Avatar {
                                        visible: !messageItem.isUser
                                        size: 28
                                        circular: true
                                        name: chatVM.chatTitle
                                    }

                                    Column {
                                        spacing: Theme.spHalf

                                        Row {
                                            spacing: Theme.sp2

                                            Text {
                                                visible: !messageItem.isUser
                                                text: chatVM.chatTitle
                                                color: Theme.text3
                                                font.family: Theme.fontFamily
                                                font.pixelSize: Theme.fsCaption
                                            }
                                            Text {
                                                text: modelData.time === undefined ? "" : modelData.time
                                                color: Theme.text3
                                                font.family: Theme.fontFamily
                                                font.pixelSize: Theme.fsCaption
                                            }
                                        }

                                        Rectangle {
                                            id: bubble
                                            width: messageItem.isCard
                                                   ? Math.min(520, messageItem.maxBubbleWidth)
                                                   : Math.min(naturalText.implicitWidth
                                                              + Theme.sp5 + Theme.sp1,
                                                              messageItem.maxBubbleWidth)
                                            height: bubbleContent.implicitHeight + Theme.sp4 + Theme.sp1
                                            radius: Theme.radiusPanel
                                            color: messageItem.bubbleColor
                                            border.width: 1
                                            border.color: messageItem.isUser
                                                          ? Theme.bubbleUserBorder
                                                          : Theme.bubbleAssistantBorder

                                            Column {
                                                id: bubbleContent
                                                x: Theme.sp3
                                                y: Theme.sp2 + Theme.spHalf
                                                width: parent.width - Theme.sp3 * 2
                                                spacing: Theme.sp1

                                                Text {
                                                    width: parent.width
                                                    visible: !messageItem.isCard
                                                    text: modelData.text
                                                    color: Theme.text1
                                                    font.family: Theme.fontFamily
                                                    font.pixelSize: Theme.fsBody
                                                    lineHeight: 1.45
                                                    wrapMode: Text.Wrap
                                                    textFormat: Text.PlainText
                                                }

                                                // ```card 折叠卡片
                                                Column {
                                                    width: parent.width
                                                    visible: messageItem.isCard
                                                    spacing: Theme.sp1

                                                    Row {
                                                        width: parent.width
                                                        spacing: Theme.sp1

                                                        Icon {
                                                            name: messageItem.expanded
                                                                  ? "chevron-down" : "chevron-right"
                                                            size: Theme.iconSm
                                                            color: Theme.accentText
                                                            anchors.verticalCenter: parent.verticalCenter
                                                        }
                                                        Text {
                                                            width: parent.width - 20
                                                            text: modelData.cardTitle === undefined
                                                                  ? "" : modelData.cardTitle
                                                            color: Theme.text1
                                                            font.family: Theme.fontFamily
                                                            font.pixelSize: Theme.fsBody
                                                            font.weight: 500
                                                            wrapMode: Text.Wrap
                                                            elide: Text.ElideRight
                                                        }
                                                    }

                                                    Text {
                                                        width: parent.width
                                                        visible: messageItem.expanded
                                                        text: modelData.cardBody === undefined
                                                              ? "" : modelData.cardBody
                                                        color: Theme.text2
                                                        font.family: Theme.fontFamily
                                                        font.pixelSize: Theme.fsCaption
                                                        lineHeight: 1.45
                                                        wrapMode: Text.Wrap
                                                        textFormat: Text.MarkdownText
                                                    }

                                                    Item {
                                                        width: parent.width
                                                        height: 20

                                                        Text {
                                                            text: messageItem.expanded ? qsTr("收起")
                                                                                       : qsTr("展开")
                                                            color: Theme.accentText
                                                            font.family: Theme.fontFamily
                                                            font.pixelSize: Theme.fsCaption
                                                        }
                                                        MouseArea {
                                                            anchors.fill: parent
                                                            cursorShape: Qt.PointingHandCursor
                                                            onClicked: chatVM.toggleExpanded(index)
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        // 助手消息操作：👍 👎 回溯
                                        Row {
                                            visible: !messageItem.isUser
                                            spacing: Theme.sp1

                                            Text {
                                                width: 24
                                                height: 20
                                                text: "👍"
                                                font.pixelSize: Theme.fsCaption
                                                opacity: messageItem.feedbackValue === 1 ? 1.0 : 0.45
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                                MouseArea {
                                                    anchors.fill: parent
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: chatVM.setFeedback(
                                                                   index,
                                                                   messageItem.feedbackValue === 1 ? 0 : 1)
                                                }
                                            }
                                            Text {
                                                width: 24
                                                height: 20
                                                text: "👎"
                                                font.pixelSize: Theme.fsCaption
                                                opacity: messageItem.feedbackValue === -1 ? 1.0 : 0.45
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                                MouseArea {
                                                    anchors.fill: parent
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: chatVM.setFeedback(
                                                                   index,
                                                                   messageItem.feedbackValue === -1 ? 0 : -1)
                                                }
                                            }
                                            IconButton {
                                                iconName: "rotate-ccw"
                                                tooltip: qsTr("回溯到这里")
                                                onClicked: page.requestBacktrack(index)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 空会话：welcomeText + 推荐问句
                    ColumnLayout {
                        anchors.centerIn: parent
                        width: Math.min(560, messageList.width)
                        spacing: Theme.sp3
                        visible: chatVM.messages.length === 0

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 56
                            Layout.preferredHeight: 56
                            radius: Theme.sp4
                            color: Theme.accentSoft
                            Icon {
                                anchors.centerIn: parent
                                name: "sparkles"
                                size: 26
                                color: Theme.accentText
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text: chatVM.welcomeText.length > 0
                                  ? chatVM.welcomeText : qsTr("说点什么开始对话吧")
                            color: Theme.text1
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fsSection
                            font.weight: 600
                            wrapMode: Text.Wrap
                        }

                        Text {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Enter 发送 · Shift+Enter 换行")
                            color: Theme.text3
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fsCaption
                        }

                        Flow {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: Math.min(560, childrenRect.width)
                            Layout.preferredHeight: childrenRect.height
                            spacing: Theme.sp2

                            Repeater {
                                model: chatVM.quickPrompts

                                delegate: Rectangle {
                                    id: promptChip
                                    width: promptText.implicitWidth + Theme.sp6
                                    height: Theme.hControl
                                    radius: Theme.radiusFull
                                    color: chipHover.hovered ? Theme.accentSoft : Theme.bgSubtle
                                    border.width: 1
                                    border.color: chipHover.hovered ? Theme.accentText : Theme.border

                                    Text {
                                        id: promptText
                                        anchors.centerIn: parent
                                        text: modelData
                                        color: Theme.accentText
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.fsCaption
                                    }
                                    HoverHandler {
                                        id: chipHover
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: page.quickAsk(modelData)
                                    }
                                }
                            }
                        }
                    }
                }

                // 错误提示 + 重试
                Rectangle {
                    Layout.fillWidth: true
                    visible: chatVM.errorText.length > 0
                    implicitHeight: errorRow.implicitHeight + Theme.sp3 * 2
                    radius: Theme.radiusControl
                    color: Theme.dangerSoft

                    RowLayout {
                        id: errorRow
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp3
                        anchors.rightMargin: Theme.sp3
                        spacing: Theme.sp2

                        Icon {
                            name: "alert-triangle"
                            size: Theme.iconMd
                            color: Theme.dangerText
                        }
                        Text {
                            Layout.fillWidth: true
                            text: chatVM.errorText
                            color: Theme.dangerText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fsCaption
                            wrapMode: Text.Wrap
                        }
                        SecondaryButton {
                            text: qsTr("重试")
                            compact: true
                            onClicked: chatVM.retry()
                        }
                    }
                }

                // ==============================================================
                // 输入卡
                // ==============================================================
                Rectangle {
                    id: inputCard
                    Layout.fillWidth: true
                    implicitHeight: inputColumn.implicitHeight + Theme.sp3 * 2
                    radius: Theme.radiusPanel
                    color: Theme.bgCard
                    border.width: 1
                    border.color: composer.activeFocus ? Theme.ring : Theme.borderStrong

                    ColumnLayout {
                        id: inputColumn
                        x: Theme.sp3
                        y: Theme.sp3
                        width: Math.max(0, inputCard.width - Theme.sp3 * 2)
                        spacing: Theme.sp2

                        RowLayout {
                            Layout.fillWidth: true
                            visible: chatVM.streaming
                            spacing: Theme.sp2

                            Rectangle {
                                Layout.preferredWidth: 6
                                Layout.preferredHeight: 6
                                radius: 3
                                color: Theme.accent

                                SequentialAnimation on opacity {
                                    running: chatVM.streaming
                                    loops: Animation.Infinite
                                    NumberAnimation { to: 0.25; duration: 600 }
                                    NumberAnimation { to: 1.0; duration: 600 }
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("正在输入…")
                                color: Theme.text3
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fsCaption
                            }
                            SecondaryButton {
                                text: qsTr("停止")
                                compact: true
                                onClicked: chatVM.stop()
                            }
                        }

                        ScrollView {
                            id: composerScroll
                            Layout.fillWidth: true
                            Layout.preferredHeight: 72
                            clip: true
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                            TextArea {
                                id: composer
                                width: composerScroll.availableWidth
                                placeholderText: qsTr("说点什么…（Enter 发送，Shift+Enter 换行）")
                                placeholderTextColor: Theme.text3
                                color: Theme.text1
                                selectionColor: Theme.accent
                                selectedTextColor: Theme.accentFg
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fsBody
                                wrapMode: TextArea.Wrap
                                background: Rectangle {
                                    color: "transparent"
                                }
                                Keys.onPressed: function(event) {
                                    if (event.key === Qt.Key_Return
                                            || event.key === Qt.Key_Enter) {
                                        if (!(event.modifiers & Qt.ShiftModifier)) {
                                            page.sendCurrent()
                                            event.accepted = true
                                        }
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp2

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Enter 发送 · Shift+Enter 换行")
                                color: Theme.text3
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fsCaption
                                elide: Text.ElideRight
                            }

                            PrimaryButton {
                                text: chatVM.streaming ? qsTr("生成中…") : qsTr("发送")
                                compact: true
                                enabled: composer.text.trim().length > 0
                                         && !chatVM.busy && !chatVM.streaming
                                onClicked: page.sendCurrent()
                            }
                        }
                    }
                }
            }
        }
    }

    // 流式/历史变化后自动滚到底部
    Connections {
        target: chatVM

        function onMessagesChanged() {
            Qt.callLater(page.scrollToEnd)
        }
    }
}
