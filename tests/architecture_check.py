#!/usr/bin/env python3
"""桌面桌宠 MVVM 架构静态校验。

对应 docs/架构重构-MVVM.md §1 的依赖方向约束：
    View(QML) → ViewModel → services/data → domain
    platform/common 可被任意层依赖，不得反向依赖上层。

检查项：
  [A] 视图层纯净：src/viewmodels/**（DialogService 除外）不出现
      QQml / QQuick / qml/ / QWidget / QFileDialog。
  [B] 依赖方向：src/{data,services,domain,platform}/** 不 include
      "viewmodels/" 或 "app/"（下层不得反向依赖上层）。
  [C] QML 不直接做数据访问：qml/**/*.qml、qml/**/*.js 不出现
      bridge / .json( / QFile / QSettings（qml/README.md 等文档不在此列）。
  [D] 旧路径 include 清零：src/** 中裸文件名 #include 必须能在同目录解析
      （模块内相对引用），否则视为旧路径 include（应写模块前缀，
      例如 "data/configstore.h"），并识别历史改名。

用法：
    python3 tests/architecture_check.py [--root <repo-root>]

退出码：0 全部通过；1 发现违规。
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

# ---------------------------------------------------------------------------
# 规则定义
# ---------------------------------------------------------------------------

# [A] src/viewmodels/** 禁止出现的符号（DialogService 是唯一允许直接使用
#     QFileDialog/QMessageBox 的适配器）。
VM_FORBIDDEN = (
    ("QQml", re.compile(r"\bQQml")),
    ("QQuick", re.compile(r"\bQQuick")),
    ("qml/", re.compile(r"qml/")),
    ("QWidget", re.compile(r"\bQWidget")),
    ("QFileDialog", re.compile(r"\bQFileDialog")),
)
VM_ALLOWED_PREFIXES = ("dialogservice",)

# [B] 下层模块不允许出现的上层 include。
LOWER_LAYERS = ("data", "services", "domain", "platform")
UPWARD_INCLUDE = re.compile(r'#include\s+"[^"]*\b(viewmodels|app)/')

# [C] QML 侧直接数据/文件/网络访问的典型特征。
QML_FORBIDDEN = (
    ("bridge", re.compile(r"\bbridge\b", re.IGNORECASE)),
    (".json(", re.compile(r"\.json\(")),
    ("QFile", re.compile(r"\bQFile\b")),
    ("QSettings", re.compile(r"\bQSettings\b")),
)
QML_SUFFIXES = (".qml", ".js")

# [D] 旧路径 include：src 内的裸文件名 include 应能在同目录解析。
BARE_INCLUDE = re.compile(r'#include\s+"([^"/]+)"')
# 历史改名（旧名 → 新位置），命中时给出修复建议。
OLD_RENAMES = {
    "intimacyservice.h": "data/intimacyrepository.h",
    "intimacyservice": "data/intimacyrepository.h",
    "configstore.h": "data/configstore.h",
    "chatstore.h": "data/chatstore.h",
    "characterstore.h": "data/characterstore.h",
    "worldstore.h": "data/worldstore.h",
    "personastore.h": "data/personastore.h",
    "memorystore.h": "data/memorystore.h",
}


@dataclass
class Violation:
    """一条架构违规。"""

    rule: str
    path: str
    line: int
    detail: str

    def format(self) -> str:
        """格式化为可读的一行。"""
        return f"[{self.rule}] {self.path}:{self.line}: {self.detail}"


@dataclass
class Report:
    """一次校验的汇总结果。"""

    root: Path
    scanned: int = 0
    violations: list[Violation] = field(default_factory=list)

    def add(self, rule: str, path: Path, line: int, detail: str) -> None:
        """记录一条违规（路径相对仓库根）。"""
        self.violations.append(Violation(rule, str(path.relative_to(self.root)), line, detail))

    def ok(self) -> bool:
        """是否存在违规。"""
        return not self.violations


# ---------------------------------------------------------------------------
# 工具函数
# ---------------------------------------------------------------------------


def read_lines(path: Path) -> list[str]:
    """按 UTF-8 读取文本文件的所有行（解码失败时宽松处理）。"""
    return path.read_text(encoding="utf-8", errors="replace").splitlines()


def iter_files(base: Path, suffixes: tuple[str, ...]):
    """递归遍历指定后缀的文件。"""
    if not base.is_dir():
        return
    for path in sorted(base.rglob("*")):
        if path.is_file() and path.suffix in suffixes:
            yield path


def header_names_under(src_root: Path) -> dict[str, list[Path]]:
    """收集 src/ 下所有头文件的 basename → 路径列表。"""
    names: dict[str, list[Path]] = {}
    for path in iter_files(src_root, (".h", ".hpp")):
        names.setdefault(path.name, []).append(path)
    return names


# ---------------------------------------------------------------------------
# 各项检查
# ---------------------------------------------------------------------------


def check_viewmodel_purity(report: Report, root: Path) -> None:
    """[A] 视图层不依赖 QML / Widgets 具体实现。"""
    viewmodels = root / "src" / "viewmodels"
    for path in iter_files(viewmodels, (".h", ".cpp")):
        if path.name.startswith(VM_ALLOWED_PREFIXES):
            continue
        report.scanned += 1
        for number, line in enumerate(read_lines(path), start=1):
            for label, pattern in VM_FORBIDDEN:
                if pattern.search(line):
                    report.add("A", path, number, f"视图层禁止依赖 {label}（DialogService 例外）")


def check_layer_direction(report: Report, root: Path) -> None:
    """[B] data/services/domain/platform 不得 include viewmodels/ 或 app/。"""
    for layer in LOWER_LAYERS:
        for path in iter_files(root / "src" / layer, (".h", ".cpp")):
            report.scanned += 1
            for number, line in enumerate(read_lines(path), start=1):
                match = UPWARD_INCLUDE.search(line)
                if match:
                    report.add("B", path, number,
                               f"下层不得反向依赖上层 include（{match.group(1)}/）")


def check_qml_data_access(report: Report, root: Path) -> None:
    """[C] QML/JS 不直接引用 bridge、读 JSON/文件/设置。"""
    qml_root = root / "qml"
    for path in iter_files(qml_root, QML_SUFFIXES):
        report.scanned += 1
        for number, line in enumerate(read_lines(path), start=1):
            for label, pattern in QML_FORBIDDEN:
                if pattern.search(line):
                    report.add("C", path, number,
                               f"QML 直接访问数据层特征 {label!r}（应通过 ViewModel）")


def check_old_includes(report: Report, root: Path) -> None:
    """[D] src/** 裸文件名 include 必须能在同目录解析（旧路径为 0）。"""
    src_root = root / "src"
    known_headers = header_names_under(src_root)
    for path in iter_files(src_root, (".h", ".hpp", ".cpp")):
        report.scanned += 1
        for number, line in enumerate(read_lines(path), start=1):
            match = BARE_INCLUDE.search(line)
            if not match:
                continue
            include = match.group(1)
            if (path.parent / include).exists():
                continue  # 模块内相对引用，允许
            suggestion = OLD_RENAMES.get(include)
            if suggestion is None:
                candidates = known_headers.get(include, [])
                if candidates:
                    suggestion = str(candidates[0].relative_to(src_root))
            if suggestion is not None:
                report.add("D", path, number,
                           f"旧路径 include {include!r}，应写模块前缀（如 \"{suggestion}\"）")
            else:
                report.add("D", path, number,
                           f"裸 include {include!r} 在 src/ 下无法解析，应写模块前缀")


# ---------------------------------------------------------------------------
# 入口
# ---------------------------------------------------------------------------


def run_checks(root: Path) -> Report:
    """执行全部检查并返回汇总报告。"""
    root = root.resolve()
    report = Report(root=root)
    check_viewmodel_purity(report, root)
    check_layer_direction(report, root)
    check_qml_data_access(report, root)
    check_old_includes(report, root)
    return report


def main(argv: list[str] | None = None) -> int:
    """命令行入口：输出违规明细并返回 0/1。"""
    parser = argparse.ArgumentParser(description="桌面桌宠 MVVM 架构静态校验")
    parser.add_argument("--root", default=None, help="仓库根目录（默认：脚本所在目录的上一级）")
    args = parser.parse_args(argv)

    root = Path(args.root) if args.root else Path(__file__).resolve().parent.parent
    if not (root / "src").is_dir():
        print(f"错误：{root} 下没有 src/，请用 --root 指定仓库根目录", file=sys.stderr)
        return 2

    report = run_checks(root)
    print(f"架构静态校验（仓库根目录：{report.root}）")
    if report.ok():
        print(f"通过：扫描 {report.scanned} 个文件，4 项检查均无违规。")
        return 0

    for violation in report.violations:
        print(violation.format())
    print(f"失败：扫描 {report.scanned} 个文件，发现 {len(report.violations)} 处违规。")
    return 1


if __name__ == "__main__":
    sys.exit(main())
