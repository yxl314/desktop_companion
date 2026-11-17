//! \file Icon.qml
//! \brief 线性图标组件：用 QtQuick.Shapes 描边 SVG path 数据，按主题色着色。
//!
//! \param name  图标名，对应 IconData.js（由 resources/icons/*.svg 生成），如 "settings"
//! \param size  图标边长，默认 Theme.iconMd（16）
//! \param color 描边颜色，默认 Theme.text2
//!
//! \note 不依赖 QtSvg 图片插件与 shader（MultiEffect），软件渲染 / 离屏截图同样可见；
//!       图标源数据来自 lucide-static v0.544.0（ISC License，见 resources/icons/LICENSE）。

import QtQuick
import QtQuick.Shapes
import "../theme"
import "IconData.js" as IconData

Item {
    id: root

    property string name: ""
    property int size: Theme.iconMd
    property color color: Theme.text2

    implicitWidth: size
    implicitHeight: size

    //! 线性图标以 24×24 viewBox 绘制，按 size 等比缩放（描边宽度同步缩放）
    Shape {
        width: 24
        height: 24
        scale: root.size / 24
        transformOrigin: Item.TopLeft
        antialiasing: true
        smooth: true

        ShapePath {
            strokeColor: root.color
            strokeWidth: 2
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin

            PathSvg {
                path: IconData.pathFor(root.name)
            }
        }
    }
}
