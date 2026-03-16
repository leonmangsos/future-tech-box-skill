# -*- coding: utf-8 -*-
"""
使用 GitHub CLI (gh) 推送 Skill 更新到 GitHub
"""
import subprocess
import os
import sys
import json
from datetime import datetime

# 设置工作目录
WORK_DIR = os.path.dirname(os.path.abspath(__file__))
os.chdir(WORK_DIR)
print(f"工作目录: {WORK_DIR}")

def run_cmd(cmd, check=True):
    """执行命令并返回结果"""
    print(f"\n>>> 执行: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    result = subprocess.run(cmd, capture_output=True, text=True, shell=isinstance(cmd, str))
    if result.stdout:
        print(result.stdout)
    if result.stderr:
        print(f"[stderr] {result.stderr}")
    if check and result.returncode != 0:
        print(f"[警告] 命令返回码: {result.returncode}")
    return result

def check_gh():
    """检查 gh CLI"""
    result = run_cmd(["gh", "--version"], check=False)
    if result.returncode != 0:
        print("错误: gh CLI 未安装")
        return False
    
    result = run_cmd(["gh", "auth", "status"], check=False)
    if result.returncode != 0:
        print("错误: gh 未登录，请先执行 'gh auth login'")
        return False
    
    return True

def check_git_repo():
    """检查是否是 git 仓库"""
    return os.path.isdir(".git")

def init_git_repo():
    """初始化 git 仓库"""
    print("\n=== 初始化 Git 仓库 ===")
    run_cmd(["git", "init"])
    run_cmd(["git", "branch", "-M", "main"])

def get_remote_url():
    """获取远程仓库 URL"""
    result = run_cmd(["git", "remote", "get-url", "origin"], check=False)
    if result.returncode == 0:
        return result.stdout.strip()
    return None

def create_github_repo():
    """使用 gh 创建 GitHub 仓库"""
    repo_name = "future-tech-box-skill"
    
    # 检查仓库是否已存在
    result = run_cmd(["gh", "repo", "view", f"leonmangsos/{repo_name}", "--json", "url"], check=False)
    if result.returncode == 0:
        data = json.loads(result.stdout)
        print(f"仓库已存在: {data['url']}")
        return data['url']
    
    # 创建新仓库
    print(f"\n=== 创建 GitHub 仓库: {repo_name} ===")
    result = run_cmd([
        "gh", "repo", "create", repo_name,
        "--public",
        "--description", "未来科技盒 2.0 自动编程 Skill - CodeBuddy 扩展",
        "--source", ".",
        "--remote", "origin",
        "--push"
    ], check=False)
    
    if result.returncode == 0:
        return f"https://github.com/leonmangsos/{repo_name}"
    return None

def git_add_commit_push(message):
    """Git add, commit 和 push"""
    print("\n=== Git 操作 ===")
    
    # 查看状态
    run_cmd(["git", "status", "--short"])
    
    # 添加所有更改（排除编译输出）
    # 先检查 .gitignore
    gitignore_content = """# 编译输出
**/build/
**/.pio/
**/*.o
**/*.d
**/*.bin
**/*.elf

# IDE
.vscode/
*.code-workspace

# 临时文件
*.tmp
*.log
__pycache__/
*.pyc

# 系统文件
.DS_Store
Thumbs.db
$null
"""
    
    if not os.path.exists(".gitignore"):
        print("创建 .gitignore 文件...")
        with open(".gitignore", "w", encoding="utf-8") as f:
            f.write(gitignore_content)
    
    # Git add
    run_cmd(["git", "add", "-A"])
    
    # 查看将要提交的内容
    result = run_cmd(["git", "diff", "--cached", "--stat"], check=False)
    
    if not result.stdout.strip():
        print("\n没有需要提交的更改")
        return False
    
    # Git commit
    run_cmd(["git", "commit", "-m", message])
    
    # Git push
    result = run_cmd(["git", "push", "-u", "origin", "main"], check=False)
    if result.returncode != 0:
        # 可能需要强制推送（首次）
        print("尝试强制推送...")
        run_cmd(["git", "push", "-u", "origin", "main", "--force"])
    
    return True

def main():
    print("=" * 50)
    print("   未来科技盒 Skill 推送到 GitHub")
    print("=" * 50)
    
    # 1. 检查 gh CLI
    if not check_gh():
        return 1
    
    # 2. 检查/初始化 git 仓库
    if not check_git_repo():
        init_git_repo()
    
    # 3. 检查/创建远程仓库
    remote_url = get_remote_url()
    if not remote_url:
        print("\n未设置远程仓库，尝试创建...")
        remote_url = create_github_repo()
        if not remote_url:
            # 手动添加远程
            run_cmd(["git", "remote", "add", "origin", 
                     "https://github.com/leonmangsos/future-tech-box-skill.git"], check=False)
    
    print(f"\n远程仓库: {remote_url or 'https://github.com/leonmangsos/future-tech-box-skill'}")
    
    # 4. 生成 commit message
    today = datetime.now().strftime("%Y-%m-%d")
    commit_msg = f"""feat: Skill 更新 {today}

更新内容:
- 超声波传感器集成（测距 + LED映射）
- 按键A触发随机旋律功能
- 跨平台支持（Windows/macOS/Linux）
- 硬件文档完善

新增示例项目:
- ultrasonic_led_map/ - 超声波距离映射LED
- random_melody/ - 按键触发随机旋律
"""
    
    # 5. 提交并推送
    if git_add_commit_push(commit_msg):
        print("\n" + "=" * 50)
        print("✅ 推送成功!")
        print(f"仓库地址: https://github.com/leonmangsos/future-tech-box-skill")
        print("=" * 50)
    else:
        print("\n没有新的更改需要推送")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
