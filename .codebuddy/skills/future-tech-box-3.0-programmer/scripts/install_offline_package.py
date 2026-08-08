#!/usr/bin/env python3
"""
未来科技盒 离线安装包安装脚本（在【目标/离线】电脑上运行）

作用：解压 build_offline_package.py 生成的离线包，自动完成：
  1. 解压 core 数据到指定目录
  2. 离线安装 PlatformIO CLI（从包内 wheel）
  3. 设置 PLATFORMIO_CORE_DIR 环境变量
  4. 验证 pio 可用

用法：
    python install_offline_package.py <离线包.zip> [--dest 安装目录]
    python install_offline_package.py future-tech-box-pio-windows-x86_64.zip

说明：
  - 需要 Python 3.8+（已安装）
  - 安装目录默认为当前目录下的 platformio_offline
  - 设置环境变量后需重启终端/IDE 生效
"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

# 强制 stdout/stderr 使用 UTF-8，避免中文 Windows 控制台（GBK）输出 emoji/中文报 UnicodeEncodeError
if sys.stdout.encoding and sys.stdout.encoding.lower() not in ("utf-8", "utf8"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
if sys.stderr.encoding and sys.stderr.encoding.lower() not in ("utf-8", "utf8"):
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")


def run_command(cmd, timeout=120):
    try:
        result = subprocess.run(
            cmd, shell=True, capture_output=True, text=True,
            timeout=timeout, encoding='utf-8', errors='replace'
        )
        return result.stdout.strip(), result.stderr.strip(), result.returncode
    except Exception as e:
        return "", str(e), -1


def set_env_var(name, value):
    """设置用户级环境变量（Windows 用 setx，macOS/Linux 写 shell profile）"""
    system = platform.system().lower()
    if system == "windows":
        # 用 PowerShell 设置用户环境变量
        cmd = (
            f'[System.Environment]::SetEnvironmentVariable("{name}", '
            f'"{value}", "User")'
        )
        stdout, stderr, code = run_command(f'powershell -Command "{cmd}"')
        if code == 0:
            print(f"  ✅ 已设置用户环境变量 {name} = {value}")
            return True
        print(f"  ⚠️ 设置环境变量失败（可能无 PowerShell 权限）: {stderr}")
        print(f"     请手动执行: setx {name} \"{value}\"")
        return False
    else:
        # macOS / Linux 写入 shell profile
        profile = Path.home() / ".bashrc"
        if platform.system().lower() == "darwin":
            profile = Path.home() / ".zshrc"
            if not profile.exists():
                profile = Path.home() / ".bash_profile"
        line = f'export {name}="{value}"'
        if profile.exists():
            content = profile.read_text(encoding="utf-8")
            if line not in content:
                profile.write_text(content + f"\n{line}\n", encoding="utf-8")
        else:
            profile.write_text(f"{line}\n", encoding="utf-8")
        print(f"  ✅ 已写入 {profile}: {line}")
        return True


def install_platformio_cli(wheel_dir):
    """离线安装 PlatformIO CLI"""
    print("\n📥 离线安装 PlatformIO CLI ...")
    wheels = list(Path(wheel_dir).glob("*.whl")) if Path(wheel_dir).exists() else []
    if not wheels:
        print("  ℹ️ 包内无 wheel（打包时未下载），跳过 CLI 安装")
        print("     请手动安装: pip install -U platformio")
        return False

    cmd = f'pip install --no-index --find-links="{wheel_dir}" platformio'
    stdout, stderr, code = run_command(cmd, timeout=300)
    if code == 0:
        print("  ✅ PlatformIO CLI 安装成功")
        return True
    print(f"  ⚠️ 安装失败: {stderr}")
    return False


def main():
    parser = argparse.ArgumentParser(description="安装未来科技盒 PlatformIO 离线包")
    parser.add_argument("package", help="离线包 zip 路径")
    parser.add_argument("--dest", default=None, help="安装目录（默认: ./platformio_offline）")
    args = parser.parse_args()

    pkg = Path(args.package)
    if not pkg.exists():
        print(f"❌ 找不到离线包: {pkg}")
        sys.exit(1)

    # 1. 安装目录
    dest = Path(args.dest) if args.dest else Path.cwd() / "platformio_offline"
    dest.mkdir(parents=True, exist_ok=True)
    core_dir = dest / "platformio"  # 包内顶层目录名为 platformio/

    print(f"📦 离线包: {pkg}")
    print(f"📂 安装目录: {dest}")

    # 2. 解压
    print("\n🔓 正在解压...")
    if (dest / "platformio" / "platforms").exists():
        print("  ⚠️ 检测到已安装，执行覆盖式更新")
    with zipfile.ZipFile(pkg, "r") as zf:
        zf.extractall(dest)
    print("  ✅ 解压完成")

    # 3. 设置环境变量
    print("\n🌐 设置 PLATFORMIO_CORE_DIR ...")
    set_env_var("PLATFORMIO_CORE_DIR", str(core_dir))

    # 4. 离线安装 CLI
    install_platformio_cli(dest / "platformio_wheel")

    # 5. 验证
    print("\n🔍 验证 ...")
    stdout, _, code = run_command("pio --version")
    if code == 0:
        print(f"  ✅ PlatformIO CLI: {stdout}")
    else:
        print("  ⚠️ pio 命令暂不可用（可能需重启终端/IDE，或 PATH 未更新）")
        print("     可用: python -m platformio --version")

    core_out, _, _ = run_command("pio --core-dir")
    if core_out:
        print(f"  ✅ core 目录: {core_out}")
    else:
        print(f"  ℹ️ 环境变量将在重启终端后生效，届时 core 目录应为: {core_dir}")

    print("\n🎉 安装完成！请执行：")
    print("  1. 关闭并重新打开终端/IDE（让环境变量生效）")
    print("  2. 进入项目目录执行 pio run，即可离线编译烧录")
    print("  3. 如需验证环境，运行: python scripts/check_environment.py")


if __name__ == "__main__":
    main()
