#!/usr/bin/env python3
"""
未来科技盒 3.0 - 带自动重试功能的烧录脚本

功能：
1. 自动检测串口状态（CH343 1A86:55D3 / ESP32-S3 303A:1001）
2. 串口被占用时自动等待重试
3. 使用 esptool 直调烧录（--after no_reset，烧录后不自动运行程序）
4. 输出 JSON 格式结果

说明：
- 3.0 使用 CH343 外置 USB 转串口（VID:PID = 1A86:55D3），board = esp32-s3-devkitc-1
- 使用 --after no_reset 让烧录完成后芯片停留在下载模式，不自动运行程序
- 避免烧录完成瞬间驱动电机导致笔记本 USB 接口电流过载提醒
- 用户需重新开关主板电源（或拔插 USB）后，新程序才会运行

使用方法：
python upload_with_retry.py <project_path> [--port COM6] [--max-retries 3]
"""

import subprocess
import time
import json
import sys
import os
import argparse
import platform

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

# 获取脚本所在目录
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
OS_TYPE = get_os_type()

# 3.0 的 board 名称
BOARD_NAME = "esp32-s3-devkitc-1"

# 根据操作系统选择串口检测脚本（与 detect_port_*.py 同目录）
PORT_DETECT_SCRIPTS = {
    "windows": os.path.join(SCRIPT_DIR, 'detect_port_windows.py'),
    "macos": os.path.join(SCRIPT_DIR, 'detect_port_macos.py'),
    "linux": os.path.join(SCRIPT_DIR, 'detect_port_linux.py'),
}
PORT_DETECT_SCRIPT = PORT_DETECT_SCRIPTS.get(OS_TYPE, PORT_DETECT_SCRIPTS["windows"])

def detect_port():
    """检测串口状态"""
    try:
        result = subprocess.run(
            ['python', PORT_DETECT_SCRIPT],
            capture_output=True,
            text=True,
            timeout=10
        )
        return json.loads(result.stdout)
    except Exception as e:
        return {"status": "error", "message": str(e)}

def run_pio_build(project_path):
    """使用 PlatformIO 编译（不烧录，避免 hard_reset）"""
    # 优先 pio 命令，失败则用 python -m platformio 兜底
    cmds = [
        ['pio', 'run', '-d', project_path],
        [sys.executable, '-m', 'platformio', 'run', '-d', project_path],
    ]
    last_error = None
    for cmd in cmds:
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=180,
                encoding='utf-8',
                errors='replace'
            )
            if result.returncode == 0:
                return {
                    "success": True,
                    "stdout": result.stdout,
                    "stderr": result.stderr,
                    "method": "pio-build"
                }
            last_error = result.stderr or result.stdout
        except subprocess.TimeoutExpired:
            return {"success": False, "error": "Timeout", "method": "pio-build"}
        except Exception as e:
            last_error = str(e)
    return {"success": False, "error": last_error or "build failed", "method": "pio-build"}

def get_core_dir():
    """获取 PlatformIO core 目录（与 check_environment.py 一致，支持迁移到非系统盘）"""
    try:
        result = subprocess.run(
            ['pio', '--core-dir'], capture_output=True, text=True,
            timeout=10, encoding='utf-8', errors='replace', shell=(os.name == "nt")
        )
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip()
    except Exception:
        pass
    # 兜底：环境变量或默认位置
    env_dir = os.environ.get("PLATFORMIO_CORE_DIR")
    if env_dir:
        return env_dir
    home_dir = os.environ.get('USERPROFILE', os.path.expanduser('~'))
    return os.path.join(home_dir, '.platformio')


def run_esptool_upload(project_path, port):
    """使用 esptool 直接烧录（3.0 适配：board = esp32-s3-devkitc-1）"""
    # 构建文件路径
    build_dir = os.path.join(project_path, '.pio', 'build', BOARD_NAME)
    bootloader = os.path.join(build_dir, 'bootloader.bin')
    partitions = os.path.join(build_dir, 'partitions.bin')
    firmware = os.path.join(build_dir, 'firmware.bin')

    # 检查文件是否存在
    for f in [bootloader, partitions, firmware]:
        if not os.path.exists(f):
            return {"success": False, "error": f"File not found: {f}", "method": "esptool"}

    # esptool 路径从 core 目录动态获取（兼容迁移到非系统盘后的 PLATFORMIO_CORE_DIR）
    core_dir = get_core_dir()
    esptool_path = os.path.join(
        core_dir, 'packages', 'tool-esptoolpy', 'esptool.py'
    )

    if not os.path.exists(esptool_path):
        return {"success": False, "error": f"esptool.py not found: {esptool_path}", "method": "esptool"}

    try:
        result = subprocess.run([
            'python', esptool_path,
            '--chip', 'esp32s3',
            '--port', port,
            '--baud', '460800',
            '--before', 'default_reset',
            '--after', 'no_reset',
            'write_flash', '-z',
            '--flash_mode', 'dio',
            '--flash_freq', '80m',
            '--flash_size', 'detect',
            '0x0', bootloader,
            '0x8000', partitions,
            '0x10000', firmware
        ], capture_output=True, text=True, timeout=60)

        return {
            "success": result.returncode == 0,
            "stdout": result.stdout,
            "stderr": result.stderr,
            "method": "esptool"
        }
    except subprocess.TimeoutExpired:
        return {"success": False, "error": "Timeout", "method": "esptool"}
    except Exception as e:
        return {"success": False, "error": str(e), "method": "esptool"}

def is_port_busy_error(result):
    """判断是否是串口被占用错误"""
    error_keywords = [
        'PermissionError',
        '拒绝访问',
        'Access is denied',
        'port is busy',
        'could not open port',
        # Linux/macOS 特有
        'Permission denied',
        'Resource busy',
        'Device or resource busy',
        'Input/output error',
    ]
    error_text = result.get('stderr', '') + result.get('stdout', '') + result.get('error', '')
    return any(keyword.lower() in error_text.lower() for keyword in error_keywords)

def upload_with_retry(project_path, port=None, max_retries=3, retry_delay=3):
    """带自动重试的烧录函数"""

    results = {
        "attempts": [],
        "final_success": False,
        "total_time": 0
    }

    start_time = time.time()

    for attempt in range(1, max_retries + 1):
        attempt_result = {
            "attempt": attempt,
            "timestamp": time.strftime("%H:%M:%S")
        }

        # Step 1: 检测串口
        port_status = detect_port()
        attempt_result["port_status"] = port_status

        if port_status.get('status') != 'ok':
            attempt_result["action"] = "wait_for_port"
            attempt_result["message"] = f"串口不可用，等待 {retry_delay} 秒"
            results["attempts"].append(attempt_result)

            if attempt < max_retries:
                time.sleep(retry_delay)
            continue

        # 获取端口
        detected_port = port or port_status.get('result', {}).get('port')
        if not detected_port:
            attempt_result["action"] = "error"
            attempt_result["message"] = "无法确定串口"
            results["attempts"].append(attempt_result)
            break

        attempt_result["port"] = detected_port

        # Step 2: 编译固件（不烧录，避免 hard_reset 自动运行程序）
        build_result = run_pio_build(project_path)
        attempt_result["build_result"] = {
            "success": build_result.get("success"),
            "method": "pio-build"
        }

        if not build_result.get("success"):
            attempt_result["action"] = "build_failed"
            attempt_result["error"] = build_result.get("stderr", build_result.get("error", "编译失败"))
            results["attempts"].append(attempt_result)
            if attempt < max_retries:
                time.sleep(retry_delay)
            continue

        # Step 3: 使用 esptool 直调烧录（--after no_reset，烧录后不自动运行程序）
        esptool_result = run_esptool_upload(project_path, detected_port)
        attempt_result["esptool_result"] = {
            "success": esptool_result.get("success"),
            "method": "esptool"
        }

        if esptool_result.get("success"):
            attempt_result["action"] = "success"
            attempt_result["message"] = "esptool 烧录成功（程序未自动运行，请重新开关电源）"
            results["attempts"].append(attempt_result)
            results["final_success"] = True
            break

        # esptool 失败：如果是端口被占用，等待后重试
        if is_port_busy_error(esptool_result) and attempt < max_retries:
            attempt_result["message"] = f"esptool 失败(端口被占用)，等待 {retry_delay} 秒后重试"
            results["attempts"].append(attempt_result)
            time.sleep(retry_delay)
            continue

        # 其他错误
        attempt_result["action"] = "failed"
        attempt_result["error"] = esptool_result.get("stderr", esptool_result.get("error", "Unknown error"))
        results["attempts"].append(attempt_result)

        if attempt < max_retries:
            time.sleep(retry_delay)

    results["total_time"] = round(time.time() - start_time, 2)

    # 生成最终消息
    if results["final_success"]:
        results["message"] = "烧录成功（程序未自动运行，请重新开关主板电源后运行）"
    else:
        results["message"] = f"烧录失败（已尝试 {max_retries} 次）"
        results["suggestion"] = "请按一下主板上的 RST 按钮，然后重试"

    return results

def main():
    parser = argparse.ArgumentParser(description='带自动重试功能的烧录脚本（未来科技盒 3.0）')
    parser.add_argument('project_path', help='PlatformIO 项目路径')
    parser.add_argument('--port', help='串口号 (如 COM6)，不指定则自动检测')
    parser.add_argument('--max-retries', type=int, default=3, help='最大重试次数')
    parser.add_argument('--retry-delay', type=int, default=3, help='重试间隔(秒)')

    args = parser.parse_args()

    # 检查项目路径
    if not os.path.exists(args.project_path):
        print(json.dumps({
            "success": False,
            "error": f"项目路径不存在: {args.project_path}"
        }, ensure_ascii=False, indent=2))
        sys.exit(1)

    # 执行带重试的烧录
    result = upload_with_retry(
        args.project_path,
        port=args.port,
        max_retries=args.max_retries,
        retry_delay=args.retry_delay
    )

    print(json.dumps(result, ensure_ascii=False, indent=2))
    sys.exit(0 if result.get("final_success") else 1)

if __name__ == '__main__':
    main()
