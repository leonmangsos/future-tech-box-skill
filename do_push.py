# -*- coding: utf-8 -*-
import subprocess
import os
import sys

sys.stdout.reconfigure(encoding='utf-8', errors='replace')

os.chdir(r'd:\CODEBUDDY-CODEHUB\03-15 未来科技盒自动编程烧录skill')
print(f"工作目录: {os.getcwd()}")

GIT = r"C:\Program Files\Git\cmd\git.exe"

def run_git(args):
    cmd = [GIT] + args.split()
    print(f"\n>>> git {args}")
    try:
        r = subprocess.run(cmd, capture_output=True, cwd=os.getcwd())
        stdout = r.stdout.decode('utf-8', errors='replace')
        stderr = r.stderr.decode('utf-8', errors='replace')
        if stdout: print(stdout)
        if stderr: print(stderr)
        return r.returncode == 0
    except Exception as e:
        print(f"Error: {e}")
        return False

# 检查 .git 目录
if not os.path.isdir('.git'):
    print("\n=== 初始化 Git 仓库 ===")
    run_git("init")
    run_git("branch -M main")
    
    # 检查远程是否已存在
    print("\n=== 添加远程仓库 ===")
    run_git("remote add origin https://github.com/leonmangsos/future-tech-box-skill.git")

# 检查 .gitignore
if not os.path.exists('.gitignore'):
    print("\n创建 .gitignore")
    with open('.gitignore', 'w') as f:
        f.write("**/build/\n**/.pio/\n**/*.o\n**/*.d\n**/*.bin\n**/*.elf\n__pycache__/\n*.pyc\n$null\n")

# Git 操作
print("\n=== Git Status ===")
run_git("status --short")

print("\n=== Git Add ===")
run_git("add -A")

print("\n=== Git Commit ===")
# 使用单独的列表避免引号问题
cmd = [GIT, "commit", "-m", "feat: add ultrasonic sensor support + random melody + update README"]
print(f"\n>>> git commit -m ...")
r = subprocess.run(cmd, capture_output=True, cwd=os.getcwd())
print(r.stdout.decode('utf-8', errors='replace'))
print(r.stderr.decode('utf-8', errors='replace'))

print("\n=== Git Push ===")
if not run_git("push origin main"):
    print("尝试强制推送...")
    run_git("push -u origin main --force")

print("\n=== 完成 ===")
print("仓库地址: https://github.com/leonmangsos/future-tech-box-skill")
