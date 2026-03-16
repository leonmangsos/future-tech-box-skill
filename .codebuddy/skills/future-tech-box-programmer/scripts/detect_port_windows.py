import json
import re
import subprocess
import sys
from typing import Dict, List

TARGET_VID = "303A"
TARGET_PID = "1001"


def run_pnputil() -> str:
    proc = subprocess.run(
        ["pnputil", "/enum-devices", "/connected", "/class", "Ports"],
        capture_output=True,
        text=True,
        encoding="gbk",  # Windows 中文系统默认编码
        errors="ignore",
        shell=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr.strip() or "pnputil 执行失败")
    return proc.stdout


def parse_devices(raw: str) -> List[Dict[str, str]]:
    blocks = re.split(r"\r?\n\r?\n", raw)
    devices = []

    for block in blocks:
        b = block.strip()
        # 兼容"实例 ID:"或"实例 ID:"（多空格）
        if not b or ("实例" not in b and "Instance" not in b):
            continue

        # 兼容中文多空格和英文输出
        instance_id_match = re.search(r"实例\s*ID[:：]\s*(.+)", b) or re.search(r"Instance ID[:：]\s*(.+)", b, re.IGNORECASE)
        desc_match = re.search(r"设备描述[:：]\s*(.+)", b) or re.search(r"Device Description[:：]\s*(.+)", b, re.IGNORECASE)
        status_match = re.search(r"状态[:：]\s*(.+)", b) or re.search(r"Status[:：]\s*(.+)", b, re.IGNORECASE)
        driver_match = re.search(r"驱动程序名称[:：]\s*(.+)", b) or re.search(r"Driver Name[:：]\s*(.+)", b, re.IGNORECASE)

        if not instance_id_match:
            continue

        instance_id = instance_id_match.group(1).strip()
        desc = desc_match.group(1).strip() if desc_match else ""
        status = status_match.group(1).strip() if status_match else ""
        driver = driver_match.group(1).strip() if driver_match else ""

        com_match = re.search(r"\((COM\d+)\)", desc, re.IGNORECASE)
        vid_match = re.search(r"VID_([0-9A-Fa-f]{4})", instance_id)
        pid_match = re.search(r"PID_([0-9A-Fa-f]{4})", instance_id)

        devices.append(
            {
                "port": com_match.group(1).upper() if com_match else "",
                "vid": vid_match.group(1).upper() if vid_match else "",
                "pid": pid_match.group(1).upper() if pid_match else "",
                "instance_id": instance_id,
                "description": desc,
                "state": status,
                "driver": driver,
            }
        )

    return devices


def pick_candidates(devices: List[Dict[str, str]]) -> List[Dict[str, str]]:
    return [
        d
        for d in devices
        if d.get("port") and d.get("vid") == TARGET_VID and d.get("pid") == TARGET_PID
    ]


def output(payload: Dict):
    print(json.dumps(payload, ensure_ascii=False, indent=2))


def main():
    try:
        raw = run_pnputil()
        devices = parse_devices(raw)
        candidates = pick_candidates(devices)

        if len(candidates) == 1:
            output(
                {
                    "status": "ok",
                    "message": "已识别到 XIAO ESP32S3 串口",
                    "result": candidates[0],
                }
            )
            return

        if len(candidates) == 0:
            output(
                {
                    "status": "not_found",
                    "message": "未检测到 VID_303A&PID_1001 的已连接端口",
                    "result": {
                        "port": "",
                        "vid": TARGET_VID,
                        "pid": TARGET_PID,
                        "instance_id": "",
                        "driver": "",
                        "state": "",
                        "candidates": [],
                    },
                }
            )
            return

        output(
            {
                "status": "multiple_found",
                "message": "检测到多个匹配端口，请指定目标 COM 口",
                "result": {
                    "port": "",
                    "vid": TARGET_VID,
                    "pid": TARGET_PID,
                    "instance_id": "",
                    "driver": "",
                    "state": "",
                    "candidates": candidates,
                },
            }
        )
    except Exception as e:
        output(
            {
                "status": "error",
                "message": f"串口识别异常: {e}",
                "result": {
                    "port": "",
                    "vid": TARGET_VID,
                    "pid": TARGET_PID,
                    "instance_id": "",
                    "driver": "",
                    "state": "",
                    "candidates": [],
                },
            }
        )
        sys.exit(1)


if __name__ == "__main__":
    main()
