# -*- coding: utf-8 -*-
import subprocess, sys, os

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

# 清理临时脚本自身
tmp = os.path.join(repo, "_git_fix.py")
if os.path.exists(tmp):
    os.remove(tmp)

print("===== ADD -A =====")
run(["add", "-A"])

print("===== COMMIT (fix gitignore) =====")
run(["commit", "-m", "chore: 修复 .gitignore 忽略 .codebuddy 下 pyc，清理误提交的缓存文件",
     "-m", "- .codebuddy/** 的 !重新包含规则后追加针对性忽略（__pycache__/ *.pyc *.pyo *.log）",
     "-m", "- 从索引移除 3.0 scripts/__pycache__ 和临时脚本"], check=False)

print("===== PUSH =====")
run(["push", "origin", "main"])
