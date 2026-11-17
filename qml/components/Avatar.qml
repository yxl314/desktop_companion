//! \file Avatar.qml
//! \brief 头像 / 形象缩略图：支持本地绝对路径、qrc 资源与空态首字占位。
//!
//! \param source   图片路径（绝对路径、相对路径或 qrc:/ URL 均可）
//! \param name     名称，图片缺失时显示其首字作为占位
//! \param size     边长，默认 32
//! \param circular 是否圆形（聊天头像 true，角色卡片方形圆角 false）
//! \param contain  true=完整显示（透明桌宠形象）；false=裁切填满（头像）

import QtQuick
import "../theme"

Item {
    id: root

    property string source: ""
    property string name: ""
    property int size: 32
    property bool circular: false
    property bool contain: false

    implicitWidth: size
    implicitHeight: size

    //! 把路径统一转成 Image 可识别的 URL。
    function resolveSource(path) {
        if (!path || path.length === 0)
            return ""
        if (path.indexOf("qrc:/") === 0 || path.indexOf("file://") === 0
                || path.indexOf("http://") === 0 || path.indexOf("https://") === 0)
            return path
        if (path.charAt(0) === "/")
            return "file://" + path
        if (/^[A-Za-z]:[\\/]/.test(path))
            return "file:///" + path.replace(/\\/g, "/")
        return "file:///" + path.replace(/\\/g, "/")
    }

    Rectangle {
        id: box
        anchors.fill: parent
        radius: root.circular ? root.height / 2 : Theme.radiusIconBox
        color: Theme.bgSubtle
        border.width: 1
        border.color: Theme.border
        clip: true

        Image {
            id: image
            anchors.fill: parent
            source: root.resolveSource(root.source)
            sourceSize: Qt.size(Math.round(root.size * 2), Math.round(root.size * 2))
            fillMode: root.contain ? Image.PreserveAspectFit : Image.PreserveAspectCrop
            smooth: true
            visible: status === Image.Ready
        }

        Text {
            anchors.centerIn: parent
            visible: image.status !== Image.Ready
            text: root.name.length > 0 ? root.name.charAt(0) : "?"
            color: Theme.text3
            font.family: Theme.fontFamily
            font.pixelSize: Math.max(10, Math.round(root.size * 0.42))
            font.weight: 500
        }
    }
}
