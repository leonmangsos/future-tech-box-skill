# -*- coding: utf-8 -*-
"""Git 初始化和推送脚本"""
import subprocess
import os
import sys

# 设置工作目录
os.chdir(os.path.dirname(os.path.abspath(__file__)))
print(f"工作目录: {os.getcwd()}")

# 尝试找到 git
git_paths = [
    "git",  # 系统 PATH
    r"C:\Program Files\Git\cmd\git.exe",
    r"C:\Program Files (x86)\Git\cmd\git.exe",
    os.path.expandvars(r"%LOCALAPPDATA%\Programs\Git\cmd\git.exe"),
]

git_cmd = None
for path in git_paths:
    try:
        result = subprocess.run([path, "--version"], capture_output=True, text=True)
        if result.returncode == 0:
            git_cmd = path
            print(f"找到 Git: {path}")
            print(f"版本: {result.stdout.strip()}")
            break
    except FileNotFoundError:
        continue

if not git_cmd:
    # 尝试通过 where 命令查找
    try:
        result = subprocess.run(["where", "git"], capture_output=True, text=True, shell=True)
        if result.returncode == 0 and result.stdout.strip():
            git_cmd = result.stdout.strip().split('\n')[0]
            print(f"通过 where 找到 Git: {git_cmd}")
    except:
        pass

if not git_cmd:
    print("错误: 未找到 Git，请确保 Git 已安装并在 PATH 中")
    sys.exit(1)

def run_git(args):
    """执行 git 命令"""
    cmd = [git_cmd] + args
    print(f"\n执行: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.stdout:
        print(result.stdout)
    if result.stderr:
        print(result.stderr)
    return result.returncode

# 执行 git 命令序列
commands = [
    ["init"],
    ["add", "."],
    ["commit", "-m", "feat: 初始化未来科技盒2.0自动编程Skill v0.1.0-beta"],
    ["remote", "add", "origin", "https://github.com/leonmangsos/future-tech-box-skill.git"],
    ["branch", "-M", "main"],
    ["push", "-u", "origin", "main"],
]

for cmd in commands:
    ret = run_git(cmd)
    # remote add 可能失败（已存在），忽略
    if ret != 0 and cmd[0] not in ["remote"]:
        print(f"警告: 命令 {cmd} 返回码 {ret}")

print("\n=== 完成 ===")
print("仓库地址: https://github.com/leonmangsos/future-tech-box-skill")
