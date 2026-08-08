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

# 删除误提交的临时脚本
run(["rm", "-f", "_git_final2.py"], check=False)
run(["add", "-A"])
run(["commit", "-m", "chore: 删除临时推送脚本"], check=False)
run(["push", "origin", "main"])
