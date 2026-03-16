# 📤 推送到 GitHub 操作指南

## ✅ GitHub 仓库已创建成功！

**仓库地址**: https://github.com/leonmangsos/future-tech-box-skill

由于当前 shell 环境无法访问 git，请按以下步骤完成推送：

---

## 方式一：使用 VS Code 终端（推荐）

### 1. 打开 VS Code 终端

按 `Ctrl + `` 或点击菜单 **终端 → 新建终端**

### 2. 进入项目目录

```bash
cd "d:\CODEBUDDY-CODEHUB\03-15 未来科技盒自动编程烧录skill"
```

### 3. 初始化 Git 仓库（如果还没有）

```bash
git init
```

### 4. 添加所有文件

```bash
git add .
```

### 5. 提交

```bash
git commit -m "feat: 初始化未来科技盒2.0自动编程Skill v0.1.0-beta

✅ 已完成功能:
- LED矩阵控制（行列扫描）
- 按键边沿检测 + 非阻塞消抖
- 蜂鸣器非阻塞控制
- 跨平台环境检测脚本
- 带重试的烧录脚本

🐛 修复问题:
- 按键消抖逻辑BUG
- 蜂鸣器API兼容性问题
- LED扫描卡顿问题

📝 说明:
- 当前仅支持未来科技盒2.0主板
- 部分功能仍在开发中"
```

### 6. 添加远程仓库

```bash
# 仓库已创建，直接使用以下地址：
git remote add origin https://github.com/leonmangsos/future-tech-box-skill.git
```

### 7. 推送到 GitHub

```bash
git branch -M main
git push -u origin main
```

---

## 方式二：使用 GitHub Desktop

1. 打开 GitHub Desktop
2. 选择 **File → Add local repository**
3. 选择项目文件夹：`d:\CODEBUDDY-CODEHUB\03-15 未来科技盒自动编程烧录skill`
4. 如果提示不是 git 仓库，点击 **Create a repository**
5. 填写仓库名称，点击 **Create repository**
6. 点击 **Publish repository** 推送到 GitHub

---

## 方式三：直接在 GitHub 网页创建

1. 打开 https://github.com/new
2. 创建新仓库（名称建议：`future-tech-box-skill`）
3. **不要**勾选 "Add a README file"
4. 创建后，按照页面提示执行本地命令

---

## 📁 需要提交的文件清单

确保以下文件都被包含：

```
✅ .gitignore
✅ README.md
✅ .codebuddy/skills/future-tech-box-programmer/
   ├── SKILL.md
   ├── README.md
   ├── references/
   │   ├── future_tech_box_v2_hardware.md
   │   └── pinout_mapping.csv
   └── scripts/
       ├── check_environment.py
       ├── detect_port_linux.py
       ├── detect_port_macos.py
       ├── detect_port_windows.py
       └── upload_with_retry.py
✅ 未来科技盒2.0主板.png
✅ 未来科技盒2.0引脚设置-工作表1.csv

可选（示例项目）：
📦 buzzer_led_test/
📦 led_key_test/
📦 led_sequence_test/
```

---

## 🏷️ 推荐的仓库设置

在 GitHub 仓库页面：

### Topics（标签）
```
codebuddy, skill, esp32, arduino, xiao-esp32s3, platformio, iot, hardware
```

### Description（描述）
```
🤖 CodeBuddy Skill for Future Tech Box 2.0 - Natural language to hardware code, compile and flash automatically!
```

### About → Website
如果有文档网站可以填写

---

## ✅ 完成后检查

推送成功后，确认以下内容在 GitHub 上正确显示：

1. [ ] README.md 正确渲染
2. [ ] 主板图片正确显示
3. [ ] `.codebuddy/skills/` 目录结构完整
4. [ ] `.gitignore` 生效（没有 `.pio` 编译产物）
