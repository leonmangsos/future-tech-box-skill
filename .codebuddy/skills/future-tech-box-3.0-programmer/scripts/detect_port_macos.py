#!/usr/bin/env python3
"""
macOS 串口检测脚本 - 未来科技盒 3.0

使用 system_profiler 和 ioreg 检测 USB 设备
输出 JSON 格式结果，与 Windows 版本保持一致

3.0 主板使用 CH343 外置 USB 转串口芯片（VID:PID = 1A86:55D3）
部分批次使用 ESP32-S3 内置 USB-Serial/JTAG（303A:1001），兼容匹配

macOS 串口命名规则：
- /dev/cu.wchusbserial* - CH340/CH343 芯片（3.0 主板上常见）
- /dev/cu.usbserial* - USB 转串口
- /dev/cu.usbmodem* - ESP32-S3 内置 USB
"""

import json
import subprocess
import re
import sys
import glob
from typing import Dict, List, Optional

# CH343 (WCH) 与 ESP32-S3 内置 USB 均视为目标
TARGET_VIDS = {"1a86", "303a"}
TARGET_PIDS = {"55d3", "1001"}


def run_command(cmd: List[str], timeout: int = 10) -> str:
    """执行命令并返回输出"""
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout
        )
        return result.stdout
    except Exception as e:
        return ""


def get_usb_devices_from_system_profiler() -> List[Dict]:
    """使用 system_profiler 获取 USB 设备信息"""
    devices = []
    output = run_command(["system_profiler", "SPUSBDataType", "-json"])

    if not output:
        return devices

    try:
        data = json.loads(output)
        usb_data = data.get("SPUSBDataType", [])

        def search_devices(items, parent_name=""):
            """递归搜索 USB 设备"""
            if not isinstance(items, list):
                return

            for item in items:
                if not isinstance(item, dict):
                    continue

                vendor_id = item.get("vendor_id", "").lower().replace("0x", "")
                product_id = item.get("product_id", "").lower().replace("0x", "")

                if vendor_id in TARGET_VIDS and product_id in TARGET_PIDS:
                    devices.append({
                        "name": item.get("_name", "Unknown"),
                        "vendor_id": vendor_id,
                        "product_id": product_id,
                        "serial_num": item.get("serial_num", ""),
                        "location_id": item.get("location_id", ""),
                    })

                for key in item:
                    if isinstance(item[key], list):
                        search_devices(item[key], item.get("_name", ""))

        search_devices(usb_data)
    except json.JSONDecodeError:
        pass

    return devices


def get_usb_devices_from_ioreg() -> List[Dict]:
    """使用 ioreg 获取 USB 设备信息（备用方案）"""
    devices = []
    output = run_command(["ioreg", "-p", "IOUSB", "-l", "-w", "0"])

    if not output:
        return devices

    blocks = output.split("+-o ")

    for block in blocks:
        vid_match = re.search(r'"idVendor"\s*=\s*(\d+)', block)
        pid_match = re.search(r'"idProduct"\s*=\s*(\d+)', block)
        name_match = re.search(r'^([^\n<]+)', block)
        serial_match = re.search(r'"USB Serial Number"\s*=\s*"([^"]+)"', block)
        location_match = re.search(r'"locationID"\s*=\s*(\d+)', block)

        if vid_match and pid_match:
            vid = format(int(vid_match.group(1)), '04x')
            pid = format(int(pid_match.group(1)), '04x')

            if vid in TARGET_VIDS and pid in TARGET_PIDS:
                devices.append({
                    "name": name_match.group(1).strip() if name_match else "Unknown",
                    "vendor_id": vid,
                    "product_id": pid,
                    "serial_num": serial_match.group(1) if serial_match else "",
                    "location_id": location_match.group(1) if location_match else "",
                })

    return devices


def find_serial_ports() -> List[str]:
    """查找所有可用的 USB 串口设备"""
    ports = []

    patterns = [
        "/dev/cu.wchusbserial*",  # CH340/CH343 芯片（3.0 主板）
        "/dev/cu.usbserial*",     # USB 转串口
        "/dev/cu.usbmodem*",      # ESP32-S3 内置 USB
        "/dev/cu.SLAB_USBtoUART*", # Silicon Labs
    ]

    for pattern in patterns:
        ports.extend(glob.glob(pattern))

    return sorted(ports)


def match_port_to_device(ports: List[str], devices: List[Dict]) -> Optional[str]:
    """尝试将串口与检测到的设备匹配"""
    # 优先 CH343 端口（3.0 主串口）
    for port in ports:
        if "wchusbserial" in port or "usbserial" in port:
            return port

    # 其次 usbmodem（ESP32-S3 内置 USB 场景）
    for port in ports:
        if "usbmodem" in port:
            return port

    if devices and ports:
        return ports[0]

    return None


def output(payload: Dict):
    """输出 JSON 结果"""
    print(json.dumps(payload, ensure_ascii=False, indent=2))


def main():
    try:
        devices = get_usb_devices_from_system_profiler()

        if not devices:
            devices = get_usb_devices_from_ioreg()

        ports = find_serial_ports()

        if devices:
            matched_port = match_port_to_device(ports, devices)
            chip = "CH343" if devices[0].get("vendor_id") == "1a86" else "ESP32-S3 native USB"

            if matched_port:
                output({
                    "status": "ok",
                    "message": f"已识别到未来科技盒 3.0 串口（{chip}）",
                    "result": {
                        "port": matched_port,
                        "vid": devices[0].get("vendor_id", "").upper(),
                        "pid": devices[0].get("product_id", "").upper(),
                        "instance_id": devices[0].get("location_id", ""),
                        "description": devices[0].get("name", ""),
                        "state": "connected",
                        "driver": chip,
                    }
                })
                return
            else:
                output({
                    "status": "device_found_no_port",
                    "message": "检测到未来科技盒 3.0 设备但未找到串口，请检查驱动",
                    "result": {
                        "port": "",
                        "vid": devices[0].get("vendor_id", "").upper(),
                        "pid": devices[0].get("product_id", "").upper(),
                        "instance_id": devices[0].get("location_id", ""),
                        "description": devices[0].get("name", ""),
                        "state": "no_port",
                        "driver": "",
                        "candidates": []
                    }
                })
                return

        # 没有找到目标设备，检查可能的串口
        if ports:
            output({
                "status": "possible_match",
                "message": "未检测到 CH343/ESP32-S3 设备，但发现可能的 USB 串口",
                "result": {
                    "port": ports[0],
                    "vid": "",
                    "pid": "",
                    "instance_id": "",
                    "description": "Possible device",
                    "state": "unverified",
                    "driver": "",
                    "candidates": ports
                }
            })
            return

        output({
            "status": "not_found",
            "message": "未检测到未来科技盒 3.0 设备（CH343 或 ESP32-S3 USB）",
            "result": {
                "port": "",
                "vid": "",
                "pid": "",
                "instance_id": "",
                "description": "",
                "state": "",
                "driver": "",
                "candidates": []
            }
        })

    except Exception as e:
        output({
            "status": "error",
            "message": f"串口识别异常: {e}",
            "result": {
                "port": "",
                "vid": "",
                "pid": "",
                "instance_id": "",
                "description": "",
                "state": "",
                "driver": "",
                "candidates": []
            }
        })
        sys.exit(1)


if __name__ == "__main__":
    main()
