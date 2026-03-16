#!/usr/bin/env python3
"""
Linux 串口检测脚本 - XIAO ESP32S3

使用 /sys/bus/usb/devices 和 udevadm 检测 USB 设备
输出 JSON 格式结果，与 Windows/macOS 版本保持一致

Linux 串口命名规则：
- /dev/ttyUSB* - 传统 USB 转串口
- /dev/ttyACM* - USB CDC ACM 设备（ESP32 通常使用这个）

VID: 303A (Espressif)
PID: 1001 (ESP32-S3)
"""

import json
import subprocess
import re
import sys
import os
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


def get_usb_devices_from_sysfs() -> List[Dict]:
    """从 /sys/bus/usb/devices 读取 USB 设备信息"""
    devices = []
    usb_path = "/sys/bus/usb/devices"
    
    if not os.path.exists(usb_path):
        return devices
    
    for device_dir in os.listdir(usb_path):
        device_path = os.path.join(usb_path, device_dir)
        
        # 读取 VID
        vid_file = os.path.join(device_path, "idVendor")
        pid_file = os.path.join(device_path, "idProduct")
        
        if not os.path.exists(vid_file) or not os.path.exists(pid_file):
            continue
        
        try:
            with open(vid_file, 'r') as f:
                vid = f.read().strip().lower()
            with open(pid_file, 'r') as f:
                pid = f.read().strip().lower()
            
            if vid == TARGET_VID and pid == TARGET_PID:
                # 读取更多信息
                product_file = os.path.join(device_path, "product")
                manufacturer_file = os.path.join(device_path, "manufacturer")
                serial_file = os.path.join(device_path, "serial")
                
                product = ""
                manufacturer = ""
                serial = ""
                
                if os.path.exists(product_file):
                    with open(product_file, 'r') as f:
                        product = f.read().strip()
                
                if os.path.exists(manufacturer_file):
                    with open(manufacturer_file, 'r') as f:
                        manufacturer = f.read().strip()
                
                if os.path.exists(serial_file):
                    with open(serial_file, 'r') as f:
                        serial = f.read().strip()
                
                devices.append({
                    "device_path": device_path,
                    "device_id": device_dir,
                    "vendor_id": vid,
                    "product_id": pid,
                    "product": product,
                    "manufacturer": manufacturer,
                    "serial": serial,
                })
        except (IOError, PermissionError):
            continue
    
    return devices


def get_usb_devices_from_lsusb() -> List[Dict]:
    """使用 lsusb 获取 USB 设备信息（备用方案）"""
    devices = []
    output = run_command(["lsusb", "-v"])
    
    if not output:
        # 尝试简单版本
        output = run_command(["lsusb"])
        if output:
            for line in output.split('\n'):
                # 格式: Bus 001 Device 003: ID 303a:1001 Espressif USB JTAG/serial debug unit
                match = re.search(
                    r'Bus\s+(\d+)\s+Device\s+(\d+):\s+ID\s+([0-9a-f]{4}):([0-9a-f]{4})\s+(.+)',
                    line, re.IGNORECASE
                )
                if match:
                    vid = match.group(3).lower()
                    pid = match.group(4).lower()
                    if vid == TARGET_VID and pid == TARGET_PID:
                        devices.append({
                            "bus": match.group(1),
                            "device": match.group(2),
                            "vendor_id": vid,
                            "product_id": pid,
                            "product": match.group(5).strip(),
                            "device_id": f"{match.group(1)}-{match.group(2)}",
                        })
    
    return devices


def find_serial_ports() -> List[Dict]:
    """查找所有可用的 USB 串口设备及其关联的 USB 设备信息"""
    ports = []
    
    # Linux 串口设备路径
    patterns = [
        "/dev/ttyACM*",   # USB CDC ACM (ESP32 通常使用)
        "/dev/ttyUSB*",   # USB 转串口
    ]
    
    for pattern in patterns:
        for port_path in glob.glob(pattern):
            port_info = {
                "port": port_path,
                "vid": "",
                "pid": "",
            }
            
            # 尝试通过 udevadm 获取更多信息
            udev_output = run_command(["udevadm", "info", "-q", "property", "-n", port_path])
            if udev_output:
                vid_match = re.search(r'ID_VENDOR_ID=([0-9a-fA-F]+)', udev_output)
                pid_match = re.search(r'ID_MODEL_ID=([0-9a-fA-F]+)', udev_output)
                
                if vid_match:
                    port_info["vid"] = vid_match.group(1).lower()
                if pid_match:
                    port_info["pid"] = pid_match.group(1).lower()
            
            ports.append(port_info)
    
    return ports


def match_port_to_device(ports: List[Dict], devices: List[Dict]) -> Optional[Dict]:
    """将串口与检测到的设备匹配"""
    # 优先匹配 VID/PID 相同的端口
    for port_info in ports:
        if port_info.get("vid") == TARGET_VID and port_info.get("pid") == TARGET_PID:
            return port_info
    
    # 如果有设备，尝试返回 ttyACM 端口
    if devices:
        for port_info in ports:
            if "ttyACM" in port_info.get("port", ""):
                return port_info
    
    return None


def check_permission(port: str) -> Dict:
    """检查串口权限"""
    result = {
        "readable": os.access(port, os.R_OK),
        "writable": os.access(port, os.W_OK),
    }
    
    # 检查用户是否在 dialout 组
    try:
        groups_output = run_command(["groups"])
        result["in_dialout"] = "dialout" in groups_output.lower()
    except:
        result["in_dialout"] = None
    
    return result


def output(payload: Dict):
    """输出 JSON 结果"""
    print(json.dumps(payload, ensure_ascii=False, indent=2))


def main():
    try:
        # 方法 1: 从 sysfs 读取
        devices = get_usb_devices_from_sysfs()
        
        # 方法 2: 如果 sysfs 失败，使用 lsusb
        if not devices:
            devices = get_usb_devices_from_lsusb()
        
        # 查找串口
        ports = find_serial_ports()
        
        if devices:
            # 找到了 ESP32-S3 设备
            matched_port = match_port_to_device(ports, devices)
            
            if matched_port:
                port_path = matched_port.get("port", "")
                
                # 检查权限
                perm = check_permission(port_path)
                
                if not perm.get("writable"):
                    # 权限不足
                    suggestion = ""
                    if not perm.get("in_dialout"):
                        suggestion = "运行: sudo usermod -a -G dialout $USER 然后重新登录"
                    else:
                        suggestion = "运行: sudo chmod 666 " + port_path
                    
                    output({
                        "status": "permission_denied",
                        "message": f"串口权限不足: {port_path}",
                        "result": {
                            "port": port_path,
                            "vid": TARGET_VID.upper(),
                            "pid": TARGET_PID.upper(),
                            "instance_id": devices[0].get("device_id", ""),
                            "description": devices[0].get("product", ""),
                            "state": "permission_denied",
                            "driver": "",
                            "permission": perm,
                            "suggestion": suggestion
                        }
                    })
                    return
                
                # 一切正常
                output({
                    "status": "ok",
                    "message": "已识别到 XIAO ESP32S3 串口",
                    "result": {
                        "port": port_path,
                        "vid": TARGET_VID.upper(),
                        "pid": TARGET_PID.upper(),
                        "instance_id": devices[0].get("device_id", ""),
                        "description": devices[0].get("product", ""),
                        "state": "connected",
                        "driver": "cdc_acm",
                        "permission": perm
                    }
                })
                return
            else:
                # 有设备但没找到串口
                output({
                    "status": "device_found_no_port",
                    "message": "检测到 ESP32S3 设备但未找到串口，请检查内核模块",
                    "result": {
                        "port": "",
                        "vid": TARGET_VID.upper(),
                        "pid": TARGET_PID.upper(),
                        "instance_id": devices[0].get("device_id", ""),
                        "description": devices[0].get("product", ""),
                        "state": "no_port",
                        "driver": "",
                        "candidates": [],
                        "suggestion": "运行: lsmod | grep cdc_acm 检查驱动是否加载"
                    }
                })
                return
        
        # 没有找到目标设备
        # 检查是否有其他可用的串口
        if ports:
            candidates = [p["port"] for p in ports]
            output({
                "status": "possible_match",
                "message": "未检测到 VID_303A 设备，但发现可能的 USB 串口",
                "result": {
                    "port": ports[0]["port"],
                    "vid": ports[0].get("vid", "").upper(),
                    "pid": ports[0].get("pid", "").upper(),
                    "instance_id": "",
                    "description": "Possible ESP32 device",
                    "state": "unverified",
                    "driver": "",
                    "candidates": candidates
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
                "candidates": [],
                "suggestion": "请确保设备已连接并检查 USB 线是否为数据线"
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
