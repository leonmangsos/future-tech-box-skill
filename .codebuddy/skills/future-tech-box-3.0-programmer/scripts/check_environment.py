#!/usr/bin/env python3
"""
未来科技盒 3.0 环境检测脚本
用于在编程前检查所有依赖是否就绪，预判可能的问题

3.0 与 2.0 的关键差异：
- 主板用 CH343 串口芯片（VID:PID = 1A86:55D3），USB 桥接 ESP32-S3
- 平台包相同：espressif32（含 esp32-s3 工具链）
- ESP32-S3 在 USB 下还能看到 303A（Espressif 自身 VID）
- 因此串口匹配规则：hwid 同时包含 1A86:55D3（CH343）或 303A（ESP32-S3 native USB）

输出 JSON 格式的检测结果，包含：
- os / platformio_core_dir / platformio_core_source
- python / platformio / serial / platform / toolchain / framework
- ready / warnings / missing
"""

import json
import subprocess
import sys
import os
import re
import platform
from pathlib import Path

# 强制 stdout/stderr 使用 UTF-8，避免中文 Windows 控制台（GBK）输出 emoji/中文报 UnicodeEncodeError
if sys.stdout.encoding and sys.stdout.encoding.lower() not in ("utf-8", "utf8"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
if sys.stderr.encoding and sys.stderr.encoding.lower() not in ("utf-8", "utf8"):
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")

def get_os_type():
    """获取操作系统类型"""
    system = platform.system().lower()
    if system == "windows":
        return "windows"
    elif system == "darwin":
        return "macos"
    elif system == "linux":
        return "linux"
    else:
        return "unknown"


def get_platformio_core_dir():
    """
    获取 PlatformIO core 目录。
    遵循 PlatformIO 上游的优先级约定：
      1) `pio --core-dir` 命令输出（最权威，反映运行时实际路径）
      2) 环境变量 PLATFORMIO_CORE_DIR（用户主动设置）
      3) ~/.platformio（默认；Windows 为 C:\\Users\\<用户>\\.platformio，macOS/Linux 为 ~/.platformio）
    """
    stdout, stderr, code = run_command("pio --core-dir")
    if code == 0 and stdout:
        return Path(stdout.strip()).resolve(), "pio --core-dir"

    env_dir = os.environ.get("PLATFORMIO_CORE_DIR")
    if env_dir:
        return Path(env_dir).resolve(), "PLATFORMIO_CORE_DIR"

    return (Path.home() / ".platformio").resolve(), "default (~/.platformio)"


def get_drive_space_gb(path):
    """获取指定路径所在磁盘的剩余空间（GB），仅 Windows 有效"""
    try:
        if platform.system() != "windows":
            return None
        import ctypes
        free_bytes = ctypes.c_ulonglong(0)
        ctypes.windll.kernel32.GetDiskFreeSpaceExW(
            ctypes.c_wchar_p(str(path)),
            None, None, ctypes.pointer(free_bytes)
        )
        return round(free_bytes.value / (1024 ** 3), 1)
    except Exception:
        return None


def run_command(cmd, timeout=30):
    """执行命令并返回输出"""
    try:
        # 自动把 PLATFORMIO_CORE_DIR 注入子进程环境，确保 pio 命令使用正确的 core 目录
        env = os.environ.copy()
        core_env = os.environ.get("PLATFORMIO_CORE_DIR")
        if core_env:
            env["PLATFORMIO_CORE_DIR"] = core_env
        result = subprocess.run(
            cmd,
            shell=True,
            capture_output=True,
            text=True,
            timeout=timeout,
            encoding='utf-8',
            errors='replace',
            env=env
        )
        return result.stdout.strip(), result.stderr.strip(), result.returncode
    except subprocess.TimeoutExpired:
        return "", "Command timeout", -1
    except Exception as e:
        return "", str(e), -1


def check_python():
    """检查 Python 环境"""
    version = f"{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}"
    return {
        "installed": True,
        "version": version,
        "ok": sys.version_info >= (3, 8)
    }


def check_platformio():
    """检查 PlatformIO CLI（优先 pio 命令，其次 python -m platformio 兜底）"""
    stdout, stderr, code = run_command("pio --version")
    cli_source = "pio"
    if code != 0:
        stdout, stderr, code = run_command("python -m platformio --version")
        cli_source = "python -m platformio"
    if code == 0:
        match = re.search(r'version\s+(\d+\.\d+\.\d+)', stdout)
        version = match.group(1) if match else stdout
        return {
            "installed": True,
            "version": version,
            "ok": True,
            "cli_source": cli_source
        }
    return {
        "installed": False,
        "version": None,
        "ok": False,
        "error": stderr or "PlatformIO not found"
    }


def check_serial():
    """
    检查串口连接（3.0 适配）。
    3.0 主板通过 CH343 桥接串口，典型 VID:PID = 1A86:55D3；
    部分批次使用 ESP32-S3 native USB（303A:XXXX）。
    """
    # 3.0 使用独立串口检测脚本（scripts/detect_port_*.py）
    # 此处优先用 pio device list --json-output 解析，脚本调用见 upload_with_retry.py
    stdout, stderr, code = run_command("pio device list --json-output")
    if code == 0:
        try:
            devices = json.loads(stdout)
            for device in devices:
                hwid = device.get("hwid", "")
                hwid_upper = hwid.upper()
                port = device.get("port", "")
                desc = device.get("description", "")

                # 匹配 3.0 串口芯片：CH343 (1A86:55D3) 或 ESP32-S3 native (303A)
                is_ch343 = "1A86:55D3" in hwid_upper
                is_esp32s3_usb = "303A" in hwid_upper
                # 描述符兼容匹配（不同平台 hwid 格式可能不同）
                desc_match = ("CH343" in desc) or ("CP210" in desc) or ("USB-SERIAL" in desc.upper())

                if is_ch343 or is_esp32s3_usb or desc_match:
                    return {
                        "found": True,
                        "port": port,
                        "description": desc,
                        "hwid": hwid,
                        "chip": "CH343" if is_ch343 else ("ESP32-S3 native USB" if is_esp32s3_usb else "unknown"),
                        "ok": True
                    }
        except json.JSONDecodeError:
            pass

    return {
        "found": False,
        "ok": False,
        "error": "No Future Tech Box 3.0 device found (expected VID:PID 1A86:55D3 or 303A:xxxx)"
    }


def check_toolchain():
    """检查 ESP32-S3 工具链"""
    core_dir, _core_src = get_platformio_core_dir()
    home_dir = core_dir / "packages"

    toolchain_path = None
    for item in home_dir.glob("toolchain-xtensa-esp32s3*"):
        if item.is_dir():
            toolchain_path = item
            break

    if toolchain_path:
        return {
            "cached": True,
            "path": str(toolchain_path),
            "ok": True
        }

    return {
        "cached": False,
        "ok": False,
        "error": "ESP32-S3 toolchain not found, will be downloaded on first compile"
    }


def check_framework():
    """检查 Arduino 框架（关键检查项）"""
    core_dir, _core_src = get_platformio_core_dir()
    home_dir = core_dir / "packages"

    framework_path = None
    framework_version = None

    for item in home_dir.glob("framework-arduinoespressif32*"):
        if item.is_dir():
            framework_path = item
            package_json = item / "package.json"
            if package_json.exists():
                try:
                    with open(package_json, 'r', encoding='utf-8') as f:
                        pkg = json.load(f)
                        framework_version = pkg.get("version", "unknown")
                except:
                    pass
            break

    if framework_path:
        return {
            "cached": True,
            "path": str(framework_path),
            "version": framework_version,
            "ok": True,
            "api_note": "Arduino Core 3.x uses ledcAttach(), 2.x uses ledcSetup()+ledcAttachPin()"
        }

    return {
        "cached": False,
        "ok": False,
        "download_size": "~50-100MB",
        "download_time": "~5-15 minutes (depends on network)",
        "error": "Arduino framework not cached, will be downloaded on first compile"
    }


def check_platform():
    """检查 ESP32 平台包（pio 命令不在 PATH 时用 python -m platformio 兜底）"""
    stdout, stderr, code = run_command("pio pkg list -g")
    if code != 0:
        stdout, stderr, code = run_command("python -m platformio pkg list -g")

    if "espressif32" in stdout.lower():
        match = re.search(r'espressif32\s*@?\s*([\d.]+)', stdout, re.IGNORECASE)
        version = match.group(1) if match else "installed"
        return {
            "installed": True,
            "version": version,
            "ok": True
        }

    return {
        "installed": False,
        "ok": False,
        "download_size": "~100-200MB",
        "error": "ESP32 platform not installed"
    }


def estimate_first_compile_time(framework_ok, toolchain_ok, platform_ok):
    """估算首次编译时间"""
    if framework_ok and toolchain_ok and platform_ok:
        return "10-30 seconds"

    missing_count = sum([not framework_ok, not toolchain_ok, not platform_ok])
    if missing_count == 1:
        return "3-10 minutes"
    elif missing_count == 2:
        return "10-15 minutes"
    else:
        return "15-25 minutes"


def main():
    os_type = get_os_type()
    core_dir, core_source = get_platformio_core_dir()

    result = {
        "os": os_type,
        "board": "future-tech-box-3.0",
        "platformio_core_dir": str(core_dir),
        "platformio_core_source": core_source,
        "python": check_python(),
        "platformio": check_platformio(),
        "serial": check_serial(),
        "platform": check_platform(),
        "toolchain": check_toolchain(),
        "framework": check_framework(),
        "ready": False,
        "warnings": [],
        "missing": []
    }

    # 判断 core 目录是否在系统盘（需要迁移到非系统盘）
    # 系统盘判断：Windows 为 %SystemDrive% (通常 C:)，macOS/Linux 为 /
    needs_relocate = False
    relocate_reason = ""
    if os_type == "windows":
        sys_drive = os.environ.get("SystemDrive", "C:").rstrip("\\")
        is_system = str(core_dir).startswith(sys_drive + "\\") or str(core_dir).startswith(sys_drive + "/")
        if is_system:
            needs_relocate = True
            relocate_reason = f"core 目录在系统盘 {sys_drive}"
    else:
        # macOS/Linux：/ 分区即系统盘；core 目录通常在 /home 或 /Users 下
        if core_source != "PLATFORMIO_CORE_DIR":
            needs_relocate = True
            relocate_reason = f"未设置 PLATFORMIO_CORE_DIR（当前使用默认位置）"

    result["needs_relocate"] = needs_relocate
    result["relocate_reason"] = relocate_reason
    if needs_relocate:
        result["warnings"].append(
            f"🚨 {relocate_reason}。PlatformIO core 实际占用 5-6GB，"
            f"请运行一键迁移脚本安装到非系统盘：\n"
            f"  python scripts/migrate_core_dir.py\n"
            f"或指定目录：python scripts/migrate_core_dir.py --target D:\\DevTools\\platformio\n"
            f"迁移后必须重启 IDE/终端。详见 docs/migrate_core_dir.md"
        )

    # 磁盘空间检查
    free_gb = get_drive_space_gb(core_dir)
    if free_gb is not None:
        result["core_dir_free_gb"] = free_gb
        if free_gb < 2.0:
            result["warnings"].append(
                f"PlatformIO core dir ({core_dir}) 所在磁盘剩余 {free_gb}GB，"
                f"首次下载 ESP32 平台包+Arduino 框架+工具链共需约 5-6GB。"
                f"强烈建议迁移到非系统盘：python scripts/migrate_core_dir.py"
            )

    # 注意：serial.ok 不作为 ready 硬条件——用户可能尚未插主板，
    # 此时仍可先编译（编译不依赖串口）。串口问题单独告警（见下方 warning）。
    all_ok = all([
        result["python"]["ok"],
        result["platformio"]["ok"],
        result["framework"]["ok"],
        result["toolchain"]["ok"]
    ])
    result["ready"] = all_ok

    if not result["python"]["ok"]:
        result["missing"].append("Python >= 3.8")
    if not result["platformio"]["ok"]:
        result["missing"].append("PlatformIO CLI")
    if not result["serial"]["ok"]:
        result["missing"].append("Serial port (Future Tech Box 3.0) - 需连接主板才能烧录")
    if not result["framework"]["ok"]:
        result["missing"].append("Arduino framework (~50-100MB, will auto-download)")
    if not result["toolchain"]["ok"]:
        result["missing"].append("ESP32-S3 toolchain (~200-300MB, will auto-download)")

    if not result["framework"]["ok"] or not result["toolchain"]["ok"]:
        est_time = estimate_first_compile_time(
            result["framework"]["ok"],
            result["toolchain"]["ok"],
            result.get("platform", {}).get("ok", False)
        )
        result["warnings"].append(f"First compile will take {est_time} to download dependencies")

    if not (result["serial"]["ok"] and result["serial"].get("port")):
        result["warnings"].append("No device connected - compilation possible but upload will fail")

    result["estimated_first_compile"] = estimate_first_compile_time(
        result["framework"]["ok"],
        result["toolchain"]["ok"],
        result.get("platform", {}).get("ok", False)
    )

    print(json.dumps(result, indent=2, ensure_ascii=False))

    # 退出码约定（供 SKILL 脚本调用判断）：
    #   0 = 环境就绪（编译依赖齐全，串口是否连接不影响）
    #   1 = 编译环境不完整（缺 Python/PlatformIO/框架/工具链）→ 需先安装
    #   2 = PlatformIO 都不可用 → 严重环境问题
    if all_ok:
        sys.exit(0)
    elif result["platformio"]["ok"]:
        sys.exit(1)
    else:
        sys.exit(2)


if __name__ == "__main__":
    main()
