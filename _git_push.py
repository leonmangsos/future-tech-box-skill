# -*- coding: utf-8 -*-
import subprocess, sys

repo = r"d:\CODEBUDDY-CODEHUB\03-15 未来科技盒自动编程烧录skill"

def run(args, check=True):
    r = subprocess.run(["git", "-C", repo] + args, capture_output=True, text=True, encoding="utf-8", errors="replace")
    out = r.stdout + r.stderr
    print(f"$ git {' '.join(args)}")
    print(out)
    if check and r.returncode != 0:
        print(f"[ERROR] exit={r.returncode}")
        sys.exit(1)
    return r

print("===== ADD -A =====")
run(["add", "-A"])

print("===== STAGED STATUS =====")
run(["status", "--short"], check=False)

print("===== COMMIT =====")
run(["commit", "-m", "feat: 烧录策略改用 esptool no_reset 防 USB 过载 + 项目目录整理",
     "-m", "- 烧录改用 esptool 直调 + --after no_reset，烧录完成后停在下载模式不自动运行，避免驱动电机触发笔记本USB电流过载提醒",
     "-m", "- 同步更新 SKILL.md 烧录流程/成功提示/故障排查/约束，upload_with_retry.py 改用 pio build + esptool 直调",
     "-m", "- 整理项目结构：新增 dice_roller/key_brightness_control/ps2_servo_control 等示例项目",
     "-m", "- 新增未来科技盒3.0自动烧录skill，.fue/ 加入 gitignore"], check=False)
