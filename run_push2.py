# -*- coding: utf-8 -*-
import subprocess
import os
import sys

os.chdir(r'd:\CODEBUDDY-CODEHUB\03-15 未来科技盒自动编程烧录skill')
print(f"工作目录: {os.getcwd()}")

def run(cmd):
    print(f"\n>>> {cmd}")
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if r.stdout: print(r.stdout)
    if r.stderr: print(r.stderr)
    return r.returncode

# 检查 gh
print("\n=== 检查 gh CLI ===")
run("gh --version")
run("gh auth status")

# 检查 git
print("\n=== 检查 Git ===")
has_git = os.path.isdir(".git")
print(f".git 目录存在: {has_git}")

if not has_git:
    print("\n=== 初始化 Git 仓库 ===")
    run("git init")
    run("git branch -M main")

# 检查远程
print("\n=== 检查远程仓库 ===")
ret = run("git remote get-url origin")
if ret != 0:
    print("添加远程仓库...")
    run("git remote add origin https://github.com/leonmangsos/future-tech-box-skill.git")

# 创建 .gitignore
if not os.path.exists(".gitignore"):
    with open(".gitignore", "w") as f:
        f.write("**/build/\n**/.pio/\n**/*.o\n**/*.d\n**/*.bin\n**/*.elf\n__pycache__/\n*.pyc\n$null\n")
    print("已创建 .gitignore")

# Git 操作
print("\n=== Git 状态 ===")
run("git status --short")

print("\n=== Git Add ===")
run("git add -A")

print("\n=== Git Commit ===")
msg = "feat: Skill更新 - 超声波传感器集成 + 随机旋律功能"
run(f'git commit -m "{msg}"')

print("\n=== Git Push ===")
ret = run("git push -u origin main")
if ret != 0:
    print("尝试强制推送...")
    run("git push -u origin main --force")

print("\n=== 完成 ===")
print("仓库: https://github.com/leonmangsos/future-tech-box-skill")
