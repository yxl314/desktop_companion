#!/usr/bin/env python3
"""从 resources/icons/*.svg 生成 qml/components/IconData.js。

用法（在仓库根目录）：
    python3 qml/tools/gen_icon_data.py

Icon.qml 用 QtQuick.Shapes 的 PathSvg 描边这些路径数据，按 Theme 颜色着色，
不依赖 QtSvg 图片插件与 shader，因此软件渲染 / 离屏截图 / Windows 打包都稳定可用。
图标源：lucide-static v0.544.0（ISC License，见 resources/icons/LICENSE）。
"""

import json
import os
import re
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "resources", "icons")
OUT = os.path.join(ROOT, "qml", "components", "IconData.js")


def num(value, default=0.0):
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def fmt(value):
    text = ("%.4f" % value).rstrip("0").rstrip(".")
    return text if text and text != "-0" else "0"


def circle_path(cx, cy, r):
    return (f"M {fmt(cx - r)} {fmt(cy)} "
            f"a {fmt(r)} {fmt(r)} 0 1 0 {fmt(2 * r)} 0 "
            f"a {fmt(r)} {fmt(r)} 0 1 0 {fmt(-2 * r)} 0")


def rect_path(x, y, w, h):
    return f"M {fmt(x)} {fmt(y)} H {fmt(x + w)} V {fmt(y + h)} H {fmt(x)} Z"


def line_path(x1, y1, x2, y2):
    return f"M {fmt(x1)} {fmt(y1)} L {fmt(x2)} {fmt(y2)}"


def points_path(points, close=False):
    nums = [float(p) for p in re.split(r"[\s,]+", (points or "").strip()) if p]
    if len(nums) < 4:
        return ""
    parts = [f"M {fmt(nums[0])} {fmt(nums[1])}"]
    for i in range(2, len(nums) - 1, 2):
        parts.append(f"L {fmt(nums[i])} {fmt(nums[i + 1])}")
    if close:
        parts.append("Z")
    return " ".join(parts)


def attr(element, name, default=0.0):
    return num(element.get(name), default)


def parse_svg(path):
    root = ET.parse(path).getroot()
    pieces = []
    for element in root.iter():
        tag = element.tag.split("}")[-1]
        if tag == "path":
            d = (element.get("d") or "").strip()
            if d:
                pieces.append(d)
        elif tag == "circle":
            pieces.append(circle_path(attr(element, "cx"), attr(element, "cy"),
                                      attr(element, "r")))
        elif tag == "ellipse":
            cx, cy = attr(element, "cx"), attr(element, "cy")
            rx, ry = attr(element, "rx"), attr(element, "ry")
            pieces.append(
                f"M {fmt(cx - rx)} {fmt(cy)} a {fmt(rx)} {fmt(ry)} 0 1 0 {fmt(2 * rx)} 0 "
                f"a {fmt(rx)} {fmt(ry)} 0 1 0 {fmt(-2 * rx)} 0")
        elif tag == "rect":
            pieces.append(rect_path(attr(element, "x"), attr(element, "y"),
                                    attr(element, "width"), attr(element, "height")))
        elif tag == "line":
            pieces.append(line_path(attr(element, "x1"), attr(element, "y1"),
                                    attr(element, "x2"), attr(element, "y2")))
        elif tag == "polyline":
            pieces.append(points_path(element.get("points")))
        elif tag == "polygon":
            pieces.append(points_path(element.get("points"), close=True))
    return " ".join(pieces)


def main():
    icons = {}
    for name in sorted(os.listdir(SRC)):
        if name.endswith(".svg"):
            icons[name[:-4]] = parse_svg(os.path.join(SRC, name))

    lines = [
        "//! \\file IconData.js",
        "//! \\brief 线性图标路径数据（由 resources/icons/*.svg 生成，lucide-static v0.544.0 / ISC）。",
        "//!",
        "//! QML 侧用 QtQuick.Shapes 直接描边这些 SVG path 数据，按 Theme 颜色着色，",
        "//! 不依赖 QtSvg 图片插件与 shader 效果（软件渲染 / 离屏环境也可正常显示）。",
        "//! 重新生成：python3 qml/tools/gen_icon_data.py",
        "",
        ".pragma library",
        "",
        "var pathData = {",
    ]
    keys = list(icons)
    for i, key in enumerate(keys):
        lines.append('    "%s": %s%s' % (
            key, json.dumps(icons[key], ensure_ascii=False),
            "," if i < len(keys) - 1 else ""))
    lines += [
        "}",
        "",
        "//! 取图标路径；未知图标返回空串。",
        "function pathFor(name) {",
        '    return pathData[name] !== undefined ? pathData[name] : ""',
        "}",
    ]
    with open(OUT, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    print("generated %d icons -> %s" % (len(icons), OUT))


if __name__ == "__main__":
    main()
