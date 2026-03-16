#!/usr/bin/env python3
"""
PlatformIO 编译执行脚本
自动执行编译并实时输出日志
"""
import subprocess
import sys
import os

def run_pio_build():
    project_path = os.path.join(os.path.dirname(__file__), "led_breathing_project")
    
    print("=" * 60)
    print("[BUILD] Starting compilation...")
    print(f"[PATH] {project_path}")
    print("=" * 60)
    print()
    
    # 执行编译
    process = subprocess.Popen(
        ["pio", "run"],
        cwd=project_path,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace"
    )
    
    # 实时输出
    for line in process.stdout:
        print(line, end="")
    
    process.wait()
    
    print()
    print("=" * 60)
    if process.returncode == 0:
        print("[SUCCESS] Build completed!")
    else:
        print(f"[FAILED] Build failed, return code: {process.returncode}")
    print("=" * 60)
    
    return process.returncode

if __name__ == "__main__":
    sys.exit(run_pio_build())
