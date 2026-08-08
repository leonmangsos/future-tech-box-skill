#!/usr/bin/env python3
"""
未来科技盒 离线安装包生成脚本（在【能联网】的机器上运行一次）

作用：把本机已经装好的 PlatformIO 环境（esp32 平台包 + 工具链 + Arduino 框架）
     连同 PlatformIO CLI 打包成一个 zip，供国内/离线电脑直接解压使用。

用法：
    python build_offline_package.py [--output 输出目录] [--os 目标系统]

说明：
  - 本脚本必须在【已成功完成过一次 espressif32 编译】的机器上运行，
    否则 core 数据不完整。
  - 打包的 core 数据仅保留 espressif32 相关组件，体积约 400-700MB（zip 后）。
  - 跨平台限制：Windows 的包只能在 Windows 用，macOS/Linux 同理。
    （目标系统可用 --os 指定，默认取当前系统）
  - PlatformIO CLI 本身以 wheel 形式打进包内，安装时离线 pip 安装。
"""

import argparse
import fnmatch
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

# 强制 UTF-8 输出，避免 Windows GBK 控制台遇到 emoji/中文报错
try:
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
except Exception:
    pass


def get_os_type():
    system = platform.system().lower()
    if system == "windows":
        return "windows"
    elif system == "darwin":
        return "macos"
    elif system == "linux":
        return "linux"
    return "unknown"


def run_command(cmd, timeout=60):
    try:
        result = subprocess.run(
            cmd, shell=True, capture_output=True, text=True,
            timeout=timeout, encoding='utf-8', errors='replace'
        )
        return result.stdout.strip(), result.stderr.strip(), result.returncode
    except Exception as e:
        return "", str(e), -1


def get_platformio_core_dir():
    stdout, stderr, code = run_command("pio --core-dir")
    if code != 0:
        stdout, stderr, code = run_command("python -m platformio --core-dir")
    if code == 0 and stdout:
        return Path(stdout.strip()).resolve()
    env_dir = os.environ.get("PLATFORMIO_CORE_DIR")
    if env_dir:
        return Path(env_dir).resolve()
    return (Path.home() / ".platformio").resolve()


def find_espressif32_related(core_dir):
    """收集 espressif32 平台及其依赖的 packages，返回 (platform_dir, [package_dirs])"""
    platforms_dir = core_dir / "platforms"
    packages_dir = core_dir / "packages"

    esp32_platform = None
    if platforms_dir.exists():
        matches = [d for d in platforms_dir.iterdir()
                   if d.is_dir() and d.name.lower().startswith("espressif32")]
        if matches:
            esp32_platform = matches[0]
            print(f"✅ 找到 ESP32 平台: {esp32_platform.name}")

    # espressif32 相关的 packages（按名称关键字过滤）
    pkg_keywords = [
        "esp32",          # 平台相关框架/工具
        "esptool",        # 烧录工具
        "mkspiffs",       # 文件系统工具
        "pioasm",         # 汇编工具
        "toolchain-xtensa-esp32s3",  # S3 工具链
    ]
    related_packages = []
    if packages_dir.exists():
        for d in sorted(packages_dir.iterdir()):
            if d.is_dir() and any(kw in d.name.lower() for kw in pkg_keywords):
                related_packages.append(d)

    return esp32_platform, related_packages


def download_platformio_wheel(output_dir):
    """下载 platformio 的 wheel 包到临时目录（用于离线 pip 安装）"""
    print("\n📥 下载 PlatformIO CLI wheel（供离线安装）...")
    wheel_dir = output_dir / "platformio_wheel"
    wheel_dir.mkdir(parents=True, exist_ok=True)
    # pip 命令可能不在 PATH，依次尝试 pip / python -m pip
    cmds = [
        f'pip download platformio -d "{wheel_dir}" -q',
        f'python -m pip download platformio -d "{wheel_dir}" -q',
    ]
    _, stderr, code = run_command(cmds[0], timeout=300)
    if code != 0:
        _, stderr, code = run_command(cmds[1], timeout=300)
    if code != 0:
        print(f"  ⚠️ pip download 失败: {stderr}")
        print("  将跳过 CLI 离线包，目标机需自行安装 PlatformIO CLI")
        return None
    wheels = list(wheel_dir.glob("*.whl"))
    if not wheels:
        print("  ⚠️ 未找到下载的 wheel")
        return None
    print(f"  ✅ 已下载 {len(wheels)} 个 wheel 包")
    return wheels


def collect_core_data(core_dir, stage_dir):
    """把 espressif32 相关 core 数据收集到临时目录"""
    print("\n📦 收集 core 数据...")
    esp32_platform, related_packages = find_espressif32_related(core_dir)

    stage_platforms = stage_dir / "platforms"
    stage_packages = stage_dir / "packages"

    total_mb = 0

    # 复制平台包
    if esp32_platform:
        dst = stage_platforms / esp32_platform.name
        shutil.copytree(esp32_platform, dst, dirs_exist_ok=True)
        size = sum(f.stat().st_size for f in dst.rglob("*") if f.is_file())
        total_mb += size / 1024 / 1024
        print(f"  ✅ 平台包: {esp32_platform.name} ({size/1024/1024:.0f} MB)")

    # 复制相关 packages
    for pkg in related_packages:
        dst = stage_packages / pkg.name
        shutil.copytree(pkg, dst, dirs_exist_ok=True)
        size = sum(f.stat().st_size for f in dst.rglob("*") if f.is_file())
        total_mb += size / 1024 / 1024
        print(f"  ✅ 组件: {pkg.name} ({size/1024/1024:.0f} MB)")

    # 复制全局配置（若存在）
    ini = core_dir / "platformio.ini"
    if ini.exists():
        shutil.copy2(ini, stage_dir / "platformio.ini")
        print("  ✅ 全局配置: platformio.ini")

    if total_mb == 0:
        print("\n❌ 未找到任何 espressif32 相关组件！")
        print("   请确认本机已成功完成过一次 espressif32 编译：")
        print("   进入任意 ESP32 项目执行 `pio run` 成功后，再运行本脚本。")
        sys.exit(1)

    print(f"\n📊 核心数据合计: {total_mb:.0f} MB")
    return total_mb


def make_zip(stage_dir, output_path):
    """压缩临时目录为 zip"""
    print(f"\n🗜️ 正在压缩为: {output_path.name} ...")
    with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as zf:
        for root, _, files in os.walk(stage_dir):
            for f in files:
                full = Path(root) / f
                rel = full.relative_to(stage_dir)
                zf.write(full, f"platformio/{rel}")
    size = output_path.stat().st_size / 1024 / 1024
    print(f"✅ 完成！压缩包: {output_path} ({size:.0f} MB)")
    return size


def main():
    parser = argparse.ArgumentParser(description="生成未来科技盒 PlatformIO 离线安装包")
    parser.add_argument("--output", default=None, help="输出目录（默认: 项目根目录/offline_package）")
    parser.add_argument("--os", default=None, choices=["windows", "macos", "linux"],
                        help="目标操作系统（默认: 当前系统）")
    args = parser.parse_args()

    target_os = args.os or get_os_type()
    print(f"🎯 目标系统: {target_os}")
    print(f"💻 当前系统: {platform.system()} {platform.machine()}")

    # 1. 检查 PlatformIO（pio 命令不在 PATH 时用 python -m platformio 兜底）
    print("\n[1/6] 检查 PlatformIO ...")
    _, _, code = run_command("pio --version")
    cli = "pio"
    if code != 0:
        _, _, code = run_command("python -m platformio --version")
        cli = "python -m platformio"
    if code != 0:
        print("[x] 未检测到 PlatformIO CLI！请先安装：pip install -U platformio")
        print("    并完成一次 espressif32 编译后再运行本脚本。")
        sys.exit(1)
    print(f"    OK - CLI 可用 ({cli})")

    core_dir = get_platformio_core_dir()
    print(f"    PlatformIO core 目录: {core_dir}")
    if not core_dir.exists():
        print(f"[x] core 目录不存在: {core_dir}")
        sys.exit(1)

    # 2. 输出目录
    output_dir = Path(args.output) if args.output else Path.cwd() / "offline_package"
    output_dir.mkdir(parents=True, exist_ok=True)

    # 3. 收集 core 数据到临时目录
    with tempfile.TemporaryDirectory() as tmp:
        stage_dir = Path(tmp)
        collect_core_data(core_dir, stage_dir)

        # 4. 下载 platformio wheel
        download_platformio_wheel(stage_dir)

        # 5. 生成安装说明
        readme = """未来科技盒 PlatformIO 离线安装包
================================
生成系统: {sys_name} ({sys_machine})
适用系统: {target_os}
生成时间: {time}

【安装步骤】（在离线电脑上执行）
1. 解压本包到任意目录（如 D:\\DevTools\\platformio）
2. 安装 Python 3.8+（如未安装）
3. 安装 PlatformIO CLI：
     cd 解压目录/platformio_wheel
     pip install --no-index --find-links=. platformio
4. 设置环境变量 PLATFORMIO_CORE_DIR 指向解压目录/platformio：
     Windows: setx PLATFORMIO_CORE_DIR "解压目录\\platformio"
     macOS/Linux: export PLATFORMIO_CORE_DIR=解压目录/platformio
   （或运行 install_offline_package.py 自动完成 3、4 步）
5. 验证：pio --core-dir 应输出解压目录/platformio
6. 进入项目目录执行 pio run，即可离线编译烧录

【注意事项】
- 本包仅适用于 {target_os} 系统，不可跨平台使用
- 请勿修改包内目录结构
- 首次执行 pio run 时 PlatformIO 会扫描本地组件，无需联网下载
""".format(
            sys_name=platform.system(),
            sys_machine=platform.machine(),
            target_os=target_os,
            time=__import__("datetime").datetime.now().strftime("%Y-%m-%d %H:%M")
        )
        (stage_dir / "README_offline.txt").write_text(readme, encoding="utf-8")
        print("  ✅ 已生成 README_offline.txt")

        # 6. 压缩
        pkg_name = f"future-tech-box-pio-{target_os}-{platform.machine().lower()}.zip"
        output_path = output_dir / pkg_name
        size = make_zip(stage_dir, output_path)

    print(f"\n🎉 离线安装包已生成！")
    print(f"   路径: {output_path}")
    print(f"   大小: {size:.0f} MB")
    print(f"   分发: 拷贝到目标电脑，参考包内 README_offline.txt 安装")
    print(f"\n💡 小提示：如目标电脑已装有 PlatformIO CLI，")
    print(f"   可仅解压 core 数据并设置 PLATFORMIO_CORE_DIR，跳过 wheel 安装。")


if __name__ == "__main__":
    main()
