@echo off
chcp 65001 >nul
echo ====================================
echo 未来科技盒 Skill 推送到 GitHub
echo ====================================

REM 初始化 git 仓库
git init
if errorlevel 1 (
    echo [错误] git init 失败
    pause
    exit /b 1
)

REM 添加远程仓库（已创建）
git remote add origin https://github.com/leonmangsos/future-tech-box-skill.git 2>nul
echo [信息] 远程仓库: https://github.com/leonmangsos/future-tech-box-skill

REM 添加所有文件
git add .
if errorlevel 1 (
    echo [错误] git add 失败
    pause
    exit /b 1
)

REM 提交
git commit -m "feat: 初始化未来科技盒2.0自动编程Skill v0.1.0-beta"
if errorlevel 1 (
    echo [错误] git commit 失败
    pause
    exit /b 1
)

REM 设置主分支并推送
git branch -M main
git push -u origin main
if errorlevel 1 (
    echo [错误] git push 失败
    pause
    exit /b 1
)

echo ====================================
echo 推送成功!
echo 仓库地址: https://github.com/leonmangsos/future-tech-box-skill
echo ====================================
pause
