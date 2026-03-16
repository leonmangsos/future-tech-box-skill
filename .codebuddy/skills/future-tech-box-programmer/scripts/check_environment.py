#!/usr/bin/env python3
"""
未来科技盒 2.0 环境检测脚本
用于在编程前检查所有依赖是否就绪，预判可能的问题

输出 JSON 格式的检测结果，包含：
- python: Python 环境
- platformio: PlatformIO CLI
- serial: 串口连接
- toolchain: 工具链缓存状态
- framework: Arduino 框架状态
- ready: 是否完全就绪
- warnings: 警告信息
- missing: 缺失的组件
"""

import json
import subprocess
import sys
import os
import re
import platform
from pathlib import Path

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

def run_command(cmd, timeout=30):
    """执行命令并返回输出"""
    try:
        result = subprocess.run(
            cmd,
            shell=True,
            capture_output=True,
            text=True,
            timeout=timeout,
            encoding='utf-8',
            errors='replace'
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
    """检查 PlatformIO CLI"""
    stdout, stderr, code = run_command("pio --version")
    if code == 0:
        # 解析版本号，例如 "PlatformIO Core, version 6.1.19"
        match = re.search(r'version\s+(\d+\.\d+\.\d+)', stdout)
        version = match.group(1) if match else stdout
        return {
            "installed": True,
            "version": version,
            "ok": True
        }
    return {
        "installed": False,
        "version": None,
        "ok": False,
        "error": stderr or "PlatformIO not found"
    }

def check_serial():
    """检查串口连接"""
    # 根据操作系统选择对应的串口检测脚本
    os_type = get_os_type()
    script_dir = Path(__file__).parent.parent.parent / "xiao-esp32s3-port-detect" / "scripts"
    
    script_map = {
        "windows": "detect_port_windows.py",
        "macos": "detect_port_macos.py",
        "linux": "detect_port_linux.py"
    }
    
    script_name = script_map.get(os_type, "detect_port_windows.py")
    detect_script = script_dir / script_name
    
    if detect_script.exists():
        stdout, stderr, code = run_command(f'python "{detect_script}"')
        if code == 0:
            try:
                result = json.loads(stdout)
                if result.get("status") == "ok":
                    return {
                        "found": True,
                        "port": result["result"]["port"],
                        "vid": result["result"]["vid"],
                        "pid": result["result"]["pid"],
                        "ok": True
                    }
                # 处理权限问题（Linux 特有）
                elif result.get("status") == "permission_denied":
                    return {
                        "found": True,
                        "port": result["result"]["port"],
                        "ok": False,
                        "error": result.get("message"),
                        "suggestion": result["result"].get("suggestion", "")
                    }
            except json.JSONDecodeError:
                pass
    
    # 备用方案：使用 pio device list
    stdout, stderr, code = run_command("pio device list --json-output")
    if code == 0:
        try:
            devices = json.loads(stdout)
            for device in devices:
                # 检查是否是 ESP32 设备 (VID: 303A)
                hwid = device.get("hwid", "")
                if "303A" in hwid.upper():
                    return {
                        "found": True,
                        "port": device.get("port"),
                        "description": device.get("description"),
                        "ok": True
                    }
        except json.JSONDecodeError:
            pass
    
    return {
        "found": False,
        "ok": False,
        "error": "No XIAO ESP32S3 device found"
    }

def check_toolchain():
    """检查 ESP32-S3 工具链"""
    home_dir = Path.home() / ".platformio" / "packages"
    
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
    home_dir = Path.home() / ".platformio" / "packages"
    
    framework_path = None
    framework_version = None
    
    for item in home_dir.glob("framework-arduinoespressif32*"):
        if item.is_dir():
            framework_path = item
            # 尝试读取版本
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
    """检查 ESP32 平台包"""
    stdout, stderr, code = run_command("pio pkg list -g")
    
    if "espressif32" in stdout.lower():
        # 提取版本号
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
    result = {
        "os": os_type,
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
    
    # 检查是否完全就绪
    all_ok = all([
        result["python"]["ok"],
        result["platformio"]["ok"],
        result["serial"]["ok"],
        result["framework"]["ok"],
        result["toolchain"]["ok"]
    ])
    
    result["ready"] = all_ok
    
    # 收集缺失组件
    if not result["python"]["ok"]:
        result["missing"].append("Python >= 3.8")
    if not result["platformio"]["ok"]:
        result["missing"].append("PlatformIO CLI")
    if not result["serial"]["ok"]:
        result["missing"].append("Serial port (XIAO ESP32S3)")
    if not result["framework"]["ok"]:
        result["missing"].append("Arduino framework (~50-100MB, will auto-download)")
    if not result["toolchain"]["ok"]:
        result["missing"].append("ESP32-S3 toolchain (~200-300MB, will auto-download)")
    
    # 添加警告
    if not result["framework"]["ok"] or not result["toolchain"]["ok"]:
        est_time = estimate_first_compile_time(
            result["framework"]["ok"],
            result["toolchain"]["ok"],
            result.get("platform", {}).get("ok", False)
        )
        result["warnings"].append(f"First compile will take {est_time} to download dependencies")
    
    if result["serial"]["ok"] and result["serial"].get("port"):
        pass  # 串口正常
    else:
        result["warnings"].append("No device connected - compilation possible but upload will fail")
    
    # 添加估算时间
    result["estimated_first_compile"] = estimate_first_compile_time(
        result["framework"]["ok"],
        result["toolchain"]["ok"],
        result.get("platform", {}).get("ok", False)
    )
    
    print(json.dumps(result, indent=2, ensure_ascii=False))
    
    # 返回码：0=就绪，1=有警告但可用，2=缺少关键组件
    if all_ok:
        sys.exit(0)
    elif result["platformio"]["ok"]:
        sys.exit(1)  # PIO 可用，其他问题可在运行时解决
    else:
        sys.exit(2)

if __name__ == "__main__":
    main()
