@echo off
chcp 65001 >nul
d:
cd "d:\CODEBUDDY-CODEHUB\03-15 未来科技盒自动编程烧录skill"
echo 当前目录: %cd%
python run_push2.py
pause
