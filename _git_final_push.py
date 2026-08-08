# -*- coding: utf-8 -*-
import subprocess, sys

repo = r"d:\CODEBUDDY-CODEHUB\03-15 未来科技盒自动编程烧录skill"

def run(args, check=True):
    r = subprocess.run(["git", "-C", repo] + args, capture_output=True, text=True, encoding="utf-8", errors="replace")
    out = (r.stdout or "") + (r.stderr or "")
    print(f"$ git {' '.join(args)}")
    print(out)
    if check and r.returncode != 0:
        print(f"[ERROR] exit={r.returncode}")
        sys.exit(1)
    return r

print("===== ADD -A =====")
run(["add", "-A"])

print("===== STATUS (short) =====")
run(["status", "--short"], check=False)

print("===== COMMIT =====")
run(["commit", "-m", "fix: 修复烧录/环境检测链路多个风险点 + 3.0 skill 完整化 + core 目录默认安装到非系统盘",
     "-m", "烧录链路修复：",
     "-m", "- 2.0 check_environment.py 修复串口检测脚本错误路径（引用不存在的 xiao-esp32s3-port-detect 目录），改用同目录 detect_port_*.py",
     "-m", "- 2.0/3.0 upload_with_retry.py 修复 esptool 路径硬编码 ~/.platformio，新增 get_core_dir() 动态获取（兼容 core 迁移到非系统盘）",
     "-m", "- 2.0/3.0 run_pio_build 增加 python -m platformio 兜底，pio 不在 PATH 也能编译",
     "-m", "- 所有脚本强制 stdout UTF-8，修复中文 Windows(GBK) 下 emoji 输出 UnicodeEncodeError 崩溃",
     "-m", "- serial.ok 不作为 ready 硬条件，未插主板也能先编译",
     "-m", "",
     "-m", "3.0 skill 完整化：",
     "-m", "- 新增 detect_port_windows/macos/linux.py（适配 CH343 1A86:55D3）",
     "-m", "- 新增 upload_with_retry.py（esptool 直调 + --after no_reset）",
     "-m", "- SKILL.md Phase 3 烧录策略同步 no_reset，baud 460800 + flash_size detect",
     "-m", "- 新增 docs/migrate_core_dir.md",
     "-m", "",
     "-m", "core 目录默认安装到非系统盘：",
     "-m", "- check_environment.py 新增 needs_relocate/relocate_reason 字段（core 在系统盘时标记）",
     "-m", "- 新增 migrate_core_dir.py 一键迁移脚本（交互选盘+复制+设环境变量+更新当前进程环境）",
     "-m", "- SKILL.md 首次执行改为强制流程：检测→迁移非系统盘→重启→验证",
     "-m", "- 文档修正 500MB 为实际 5-6GB，offline_package/ 加入 .gitignore"], check=False)
