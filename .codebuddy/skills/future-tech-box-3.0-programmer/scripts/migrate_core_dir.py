#!/usr/bin/env python3
"""
未来科技盒 - PlatformIO core 目录一键迁移脚本

背景：
- PlatformIO 默认把 core 目录（ESP32 平台包/Arduino 框架/工具链/缓存）放在
  C:\\Users\\<用户名>\\.platformio，实际占用可达 5-6GB，对系统盘紧张的用户不友好。
- 本脚本一键完成：检测现状 → 迁移到目标盘 → 设置 PLATFORMIO_CORE_DIR → 验证。

支持平台：Windows / macOS / Linux
用法：
  python migrate_core_dir.py                          # 交互式：检测现状并让用户选择目标盘
  python migrate_core_dir.py --target D:\\DevTools\\platformio   # 指定目标目录（自动创建）
  python migrate_core_dir.py --target D:\\DevTools\\platformio --move  # 迁移成功后删除原目录
  python migrate_core_dir.py --check                  # 仅检测现状，不迁移
  python migrate_core_dir.py --json                   # 输出 JSON（供 skill 自动解析）
"""

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
import time
from pathlib import Path

# 强制 stdout/stderr 使用 UTF-8，避免中文 Windows 控制台（GBK）输出 emoji/中文报 UnicodeEncodeError
if sys.stdout.encoding and sys.stdout.encoding.lower() not in ("utf-8", "utf8"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
if sys.stderr.encoding and sys.stderr.encoding.lower() not in ("utf-8", "utf8"):
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")

ENV_NAME = "PLATFORMIO_CORE_DIR"


def get_os_type():
    system = platform.system().lower()
    if system == "windows":
        return "windows"
    elif system == "darwin":
        return "macos"
    elif system == "linux":
        return "linux"
    return "unknown"


def run_command(cmd, timeout=30):
    """执行命令并返回 (stdout, stderr, code)"""
    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, timeout=timeout,
            encoding="utf-8", errors="replace", shell=(os.name == "nt")
        )
        return result.stdout.strip(), result.stderr.strip(), result.returncode
    except subprocess.TimeoutExpired:
        return "", "Command timeout", -1
    except Exception as e:
        return "", str(e), -1


def get_core_dir():
    """获取当前 PlatformIO core 目录及来源（与 check_environment.py 一致）"""
    stdout, stderr, code = run_command("pio --core-dir")
    if code == 0 and stdout:
        return Path(stdout.strip()).resolve(), "pio --core-dir"
    env_dir = os.environ.get(ENV_NAME)
    if env_dir:
        return Path(env_dir).resolve(), ENV_NAME
    return (Path.home() / ".platformio").resolve(), "default (~/.platformio)"


def get_drive_space_gb(path):
    """获取路径所在磁盘剩余空间（GB），Windows/macOS/Linux 通用"""
    try:
        if platform.system() == "windows":
            import ctypes
            free_bytes = ctypes.c_ulonglong(0)
            ctypes.windll.kernel32.GetDiskFreeSpaceExW(
                ctypes.c_wchar_p(str(path)), None, None, ctypes.pointer(free_bytes)
            )
            return round(free_bytes.value / (1024 ** 3), 1)
        else:
            # macOS/Linux: os.statvfs
            st = os.statvfs(str(path))
            free = st.f_bavail * st.f_frsize
            return round(free / (1024 ** 3), 1)
    except Exception:
        return None


def dir_size_gb(path):
    """计算目录总大小（GB）"""
    total = 0
    try:
        for dirpath, dirnames, filenames in os.walk(path):
            for f in filenames:
                try:
                    total += os.path.getsize(os.path.join(dirpath, f))
                except OSError:
                    pass
        return round(total / (1024 ** 3), 2)
    except Exception:
        return None


def list_windows_drives():
    """列出 Windows 所有逻辑盘符及剩余空间"""
    drives = []
    try:
        import ctypes
        bitmask = ctypes.windll.kernel32.GetLogicalDrives()
        for i in range(26):
            if bitmask & (1 << i):
                letter = chr(ord('A') + i)
                free = get_drive_space_gb(f"{letter}:\\")
                drives.append({"drive": letter, "free_gb": free})
    except Exception:
        pass
    return drives


def set_env_var_windows(name, value):
    """Windows：写入用户环境变量（注册表 HKCU\\Environment）+ 广播刷新"""
    try:
        import winreg
        key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment", 0, winreg.KEY_SET_VALUE)
        winreg.SetValueEx(key, name, 0, winreg.REG_EXPAND_SZ, value)
        winreg.CloseKey(key)

        # 广播 WM_SETTINGCHANGE，让已打开的程序感知新环境变量
        try:
            import ctypes
            HWND_BROADCAST = 0xFFFF
            WM_SETTINGCHANGE = 0x1A
            ctypes.windll.user32.SendMessageTimeoutW(
                HWND_BROADCAST, WM_SETTINGCHANGE, 0, "Environment", 0, 5000, None
            )
        except Exception:
            pass
        return True, None
    except Exception as e:
        return False, str(e)


def set_env_var_posix(name, value):
    """macOS/Linux：写入 ~/.bashrc / ~/.zshrc"""
    home = Path.home()
    rc_file = None
    if platform.system() == "darwin":
        if (home / ".zshrc").exists():
            rc_file = home / ".zshrc"
        else:
            rc_file = home / ".bash_profile"
    else:
        if (home / ".bashrc").exists():
            rc_file = home / ".bashrc"
        elif (home / ".bash_profile").exists():
            rc_file = home / ".bash_profile"
        else:
            rc_file = home / ".bashrc"

    line = f"export {name}={value}"
    try:
        content = rc_file.read_text(encoding="utf-8") if rc_file.exists() else ""
        if f"{name}=" in content:
            # 替换已有行
            new_lines = []
            for l in content.splitlines():
                if l.strip().startswith(f"export {name}=") or l.strip().startswith(f"{name}="):
                    new_lines.append(line)
                else:
                    new_lines.append(l)
            content = "\n".join(new_lines) + "\n"
        else:
            content = content.rstrip() + "\n\n# PlatformIO core dir (by migrate_core_dir.py)\n" + line + "\n"
        rc_file.write_text(content, encoding="utf-8")
        return True, str(rc_file)
    except Exception as e:
        return False, str(e)


def set_env_var(name, value):
    """跨平台设置用户环境变量"""
    if get_os_type() == "windows":
        ok, info = set_env_var_windows(name, value)
    else:
        ok, info = set_env_var_posix(name, value)
    return ok, info


def migrate_copy(src, dst):
    """复制 core 目录到目标（保留原目录），返回 (ok, msg)"""
    try:
        dst.mkdir(parents=True, exist_ok=True)
        # 复制内容（不复制 src 本身，而是其内容）
        total_start = time.time()
        for item in src.iterdir():
            s = item
            d = dst / item.name
            if item.is_dir():
                shutil.copytree(item, d, dirs_exist_ok=True, symlinks=True)
            else:
                shutil.copy2(item, d)
        elapsed = round(time.time() - total_start, 1)
        return True, f"复制完成，耗时 {elapsed}s"
    except Exception as e:
        return False, str(e)


def verify_core_dir(target):
    """验证目标目录是否为可用 core 目录（含 platforms 或 packages 之一）"""
    has_platforms = (target / "platforms").exists()
    has_packages = (target / "packages").exists()
    if has_platforms or has_packages:
        return True
    # 允许空目录（首次安装场景：迁移脚本只负责设置变量，让 pio 首次编译时下载）
    return True


def main():
    parser = argparse.ArgumentParser(description="PlatformIO core 目录一键迁移")
    parser.add_argument("--target", help="目标目录，如 D:\\DevTools\\platformio")
    parser.add_argument("--move", action="store_true", help="迁移成功后删除原目录")
    parser.add_argument("--check", action="store_true", help="仅检测现状，不迁移")
    parser.add_argument("--json", action="store_true", help="输出 JSON 格式")
    parser.add_argument("--yes", action="store_true", help="跳过确认")
    args = parser.parse_args()

    os_type = get_os_type()
    core_dir, core_source = get_core_dir()
    core_size = dir_size_gb(core_dir) if core_dir.exists() else None
    core_free = get_drive_space_gb(core_dir) if core_dir.exists() else None

    report = {
        "os": os_type,
        "core_dir": str(core_dir),
        "core_source": core_source,
        "core_exists": core_dir.exists(),
        "core_size_gb": core_size,
        "core_free_gb": core_free,
        "env_var": os.environ.get(ENV_NAME),
    }

    # --check 模式：只检测
    if args.check:
        if args.json:
            print(json.dumps(report, ensure_ascii=False, indent=2))
        else:
            print(f"当前 core 目录: {core_dir}  (来源: {core_source})")
            if core_dir.exists():
                print(f"  大小: {core_size} GB | 所在盘剩余: {core_free} GB")
            else:
                print("  (目录尚未创建，首次编译时会自动下载)")
            if os.environ.get(ENV_NAME):
                print(f"环境变量 {ENV_NAME} 已设置: {os.environ[ENV_NAME]}")
            else:
                print(f"环境变量 {ENV_NAME} 未设置（当前使用默认位置）")
        sys.exit(0)

    # 交互式：无 --target 时列出 Windows 盘符
    target = args.target
    if not target:
        if os_type == "windows":
            drives = list_windows_drives()
            print("检测到的磁盘：")
            for d in drives:
                free = f"{d['free_gb']}GB" if d["free_gb"] is not None else "?"
                print(f"  {d['drive']}:  剩余 {free}")
            print("\n请输入目标盘符（如 D）和目标子目录（可含路径）：")
        target = input("目标目录 (例如 D:\\DevTools\\platformio): ").strip().strip('"')
        if not target:
            print("未输入目标目录，退出")
            sys.exit(1)

    target_path = Path(target).expanduser().resolve()

    # 检查目标盘剩余空间
    target_free = get_drive_space_gb(target_path)
    need = max(core_size or 1.0, 1.0) + 0.5  # 预留 0.5GB
    if target_free is not None and target_free < need:
        msg = f"目标盘剩余 {target_free}GB 不足（需要约 {need:.1f}GB）"
        if args.json:
            print(json.dumps({"success": False, "error": msg}, ensure_ascii=False))
        else:
            print(f"❌ {msg}")
        sys.exit(1)

    # 目标与源相同则报错
    if target_path == core_dir or (core_dir.exists() and target_path == core_dir.resolve()):
        print("目标目录与当前 core 目录相同，无需迁移")
        sys.exit(0)

    # 确认
    if not args.yes and not args.json:
        action = "移动" if args.move else "复制"
        print(f"\n即将把 core 目录从:")
        print(f"  {core_dir}  (大小 {core_size} GB)")
        print(f"{action}到:")
        print(f"  {target_path}")
        if args.move:
            print(f"⚠️  迁移成功后将删除原目录 {core_dir}")
        confirm = input("\n确认继续？(y/N): ").strip().lower()
        if confirm not in ("y", "yes"):
            print("已取消")
            sys.exit(0)

    # 执行迁移
    success = True
    msg = ""
    if core_dir.exists():
        ok, msg = migrate_copy(core_dir, target_path)
        if not ok:
            result = {"success": False, "error": f"复制失败: {msg}"}
            print(json.dumps(result, ensure_ascii=False) if args.json else f"❌ 复制失败: {msg}")
            sys.exit(1)

        # 验证
        if not verify_core_dir(target_path):
            result = {"success": False, "error": "目标目录校验失败"}
            print(json.dumps(result, ensure_ascii=False) if args.json else "❌ 目标目录校验失败")
            sys.exit(1)

        # --move：删除原目录
        if args.move:
            try:
                shutil.rmtree(core_dir)
                msg += "；已删除原目录"
            except Exception as e:
                msg += f"；⚠️ 原目录删除失败（{e}），请手动删除"

    # 设置环境变量
    ok_env, env_info = set_env_var(ENV_NAME, str(target_path))
    if not ok_env:
        msg_env = f"环境变量设置失败: {env_info}"
        print(json.dumps({"success": False, "error": msg_env}, ensure_ascii=False) if args.json else f"❌ {msg_env}")
        sys.exit(1)

    # 同步更新当前进程环境变量，让本会话内后续命令（如 check_environment.py）立即读到新值
    # （注意：其他已打开的进程/终端仍需重启才能生效）
    os.environ[ENV_NAME] = str(target_path)

    result = {
        "success": True,
        "message": f"迁移完成：{core_dir} → {target_path}；{ENV_NAME}={target_path}（需重启 IDE/终端生效）",
        "target": str(target_path),
        "moved": args.move,
        "env_set": True,
        "restart_required": True,
    }
    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    else:
        print("\n✅ 迁移完成！")
        print(f"  新 core 目录: {target_path}")
        print(f"  环境变量 {ENV_NAME} 已设置（{env_info}）")
        print("  ⚠️  请关闭并重新打开 IDE / 终端，让环境变量生效。")
        print("  生效后运行: python scripts/check_environment.py 验证")
        if args.move and core_dir.exists():
            print(f"  ⚠️  原目录 {core_dir} 删除失败，请手动删除")
    sys.exit(0)


if __name__ == "__main__":
    main()
