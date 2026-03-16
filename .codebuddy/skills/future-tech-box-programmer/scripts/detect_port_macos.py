#!/usr/bin/env python3
"""
macOS 串口检测脚本 - XIAO ESP32S3

使用 system_profiler 和 ioreg 检测 USB 设备
输出 JSON 格式结果，与 Windows 版本保持一致

macOS 串口命名规则：
- /dev/cu.usbmodem* - 拨出端口（推荐用于烧录）
- /dev/tty.usbmodem* - 拨入端口

VID: 303A (Espressif)
PID: 1001 (ESP32-S3)
"""

import json
import subprocess
import re
import sys
import glob
from typing import Dict, List, Optional

TARGET_VID = "303a"  # Espressif VID (小写用于匹配)
TARGET_PID = "1001"  # ESP32-S3 PID


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
                
                # 检查 VID/PID
                vendor_id = item.get("vendor_id", "").lower().replace("0x", "")
                product_id = item.get("product_id", "").lower().replace("0x", "")
                
                if TARGET_VID in vendor_id and TARGET_PID in product_id:
                    devices.append({
                        "name": item.get("_name", "Unknown"),
                        "vendor_id": vendor_id,
                        "product_id": product_id,
                        "serial_num": item.get("serial_num", ""),
                        "location_id": item.get("location_id", ""),
                    })
                
                # 递归搜索子设备
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
    
    # 按设备块分割
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
            
            if vid == TARGET_VID and pid == TARGET_PID:
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
    
    # macOS 串口设备路径
    patterns = [
        "/dev/cu.usbmodem*",      # ESP32 通常使用这个
        "/dev/cu.usbserial*",     # 一些 USB 转串口设备
        "/dev/cu.SLAB_USBtoUART*", # Silicon Labs 芯片
        "/dev/cu.wchusbserial*",  # CH340/CH341 芯片
    ]
    
    for pattern in patterns:
        ports.extend(glob.glob(pattern))
    
    return sorted(ports)


def match_port_to_device(ports: List[str], devices: List[Dict]) -> Optional[str]:
    """尝试将串口与检测到的设备匹配"""
    # 优先选择 usbmodem 端口
    for port in ports:
        if "usbmodem" in port:
            return port
    
    # 如果有设备但找不到 usbmodem，返回第一个可用端口
    if devices and ports:
        return ports[0]
    
    return None


def output(payload: Dict):
    """输出 JSON 结果"""
    print(json.dumps(payload, ensure_ascii=False, indent=2))


def main():
    try:
        # 方法 1: 使用 system_profiler
        devices = get_usb_devices_from_system_profiler()
        
        # 方法 2: 如果 system_profiler 失败，使用 ioreg
        if not devices:
            devices = get_usb_devices_from_ioreg()
        
        # 查找串口
        ports = find_serial_ports()
        
        if devices:
            # 找到了 ESP32-S3 设备
            matched_port = match_port_to_device(ports, devices)
            
            if matched_port:
                output({
                    "status": "ok",
                    "message": "已识别到 XIAO ESP32S3 串口",
                    "result": {
                        "port": matched_port,
                        "vid": TARGET_VID.upper(),
                        "pid": TARGET_PID.upper(),
                        "instance_id": devices[0].get("location_id", ""),
                        "description": devices[0].get("name", ""),
                        "state": "connected",
                        "driver": "native"
                    }
                })
                return
            else:
                # 有设备但没找到串口
                output({
                    "status": "device_found_no_port",
                    "message": "检测到 ESP32S3 设备但未找到串口，请检查驱动",
                    "result": {
                        "port": "",
                        "vid": TARGET_VID.upper(),
                        "pid": TARGET_PID.upper(),
                        "instance_id": devices[0].get("location_id", ""),
                        "description": devices[0].get("name", ""),
                        "state": "no_port",
                        "driver": "",
                        "candidates": []
                    }
                })
                return
        
        # 没有找到目标设备
        # 检查是否有其他 ESP32 相关的串口
        esp_ports = [p for p in ports if "usbmodem" in p]
        
        if esp_ports:
            output({
                "status": "possible_match",
                "message": "未检测到 VID_303A 设备，但发现可能的 USB 串口",
                "result": {
                    "port": esp_ports[0],
                    "vid": "",
                    "pid": "",
                    "instance_id": "",
                    "description": "Possible ESP32 device",
                    "state": "unverified",
                    "driver": "",
                    "candidates": esp_ports
                }
            })
            return
        
        # 完全没有找到
        output({
            "status": "not_found",
            "message": "未检测到 XIAO ESP32S3 设备",
            "result": {
                "port": "",
                "vid": TARGET_VID.upper(),
                "pid": TARGET_PID.upper(),
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
                "vid": TARGET_VID.upper(),
                "pid": TARGET_PID.upper(),
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
