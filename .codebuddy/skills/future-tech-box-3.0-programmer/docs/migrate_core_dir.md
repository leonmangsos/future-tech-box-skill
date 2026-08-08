# PlatformIO core 目录迁移指南（移到非系统盘）

> ⚠️ **为什么需要迁移？**
>
> PlatformIO 默认把全部工具链装在 `C:\Users\<用户名>\.platformio\`（系统盘），
> 包含 **ESP32 平台包 + Arduino 框架 + 编译工具链 + 包缓存 + 平台索引**。
> **实际总占用可达 5-6GB**（远超早期文档估计的 500MB），首次编译还会继续膨胀。
> 对系统盘（C 盘）空间紧张的用户，这是不可接受的。

---

## 方式一：一键迁移脚本（推荐 ⭐）

本 skill 自带 `scripts/migrate_core_dir.py`，自动完成：检测现状 → 迁移 → 设置环境变量。

### 使用步骤

```bash
# 1. 先查看当前状态（不迁移，只检测）
python scripts/migrate_core_dir.py --check

# 2. 一键迁移到 D 盘（复制，保留原目录）
python scripts/migrate_core_dir.py --target D:\DevTools\platformio

# 3. 确认新目录工作正常后，可再迁移并删除原目录（释放 C 盘空间）
python scripts/migrate_core_dir.py --target D:\DevTools\platformio --move
```

脚本会自动：
1. 检测当前 core 目录位置和大小
2. 检查目标盘剩余空间是否足够
3. 复制 core 数据到目标目录
4. 写入用户级环境变量 `PLATFORMIO_CORE_DIR`
5. 提示重启 IDE/终端生效

> macOS / Linux 示例：
> ```bash
> python scripts/migrate_core_dir.py --target /Volumes/External/platformio   # macOS 外接盘
> python scripts/migrate_core_dir.py --target /home/leon/devtools/platformio # Linux
> ```

---

## 方式二：手动设置环境变量

### Windows PowerShell（永久，推荐）

```powershell
[System.Environment]::SetEnvironmentVariable('PLATFORMIO_CORE_DIR', 'D:\DevTools\platformio', 'User')
# 关闭并重新打开终端/IDE
```

### Windows CMD（永久）

```cmd
setx PLATFORMIO_CORE_DIR "D:\DevTools\platformio"
# 关闭并重新打开终端/IDE
```

### macOS / Linux（永久）

把下面加到 `~/.bashrc` 或 `~/.zshrc`：
```bash
export PLATFORMIO_CORE_DIR=/Volumes/External/platformio   # macOS 外接硬盘
# 或
export PLATFORMIO_CORE_DIR=/home/leon/devtools/platformio # Linux
```

---

## 验证迁移是否生效

```bash
# 1. 检查环境变量
echo %PLATFORMIO_CORE_DIR%   # Windows CMD
echo $env:PLATFORMIO_CORE_DIR  # Windows PowerShell
echo $PLATFORMIO_CORE_DIR    # macOS/Linux

# 2. 运行 skill 自带检测脚本
python scripts/check_environment.py
# 确认输出中 platformio_core_source 显示为 PLATFORMIO_CORE_DIR（而不是 default）
# platformio_core_dir 指向新目录
```

> ⚠️ **重要**：环境变量设置后必须**关闭并重新打开 IDE / 终端**，
> 让所有子进程继承新变量。否则 `pio` 仍会使用旧位置。

---

## 首次安装即设置到非系统盘（新用户）

如果 **core 目录还不存在**（从未编译过），直接用一键脚本设置目标目录即可，
首次 `pio run` 会自动在新目录下载所有组件：

```bash
python scripts/migrate_core_dir.py --target D:\DevTools\platformio --yes
```

脚本会自动创建目录并写入环境变量，之后的首次编译会下载到 D 盘。

---

## 清理旧的 C 盘目录

确认新目录编译正常后，可以手动删除 C 盘旧目录释放空间：

```powershell
# Windows - 删除旧的默认 core 目录（确认新目录已正常工作后）
Remove-Item -Recurse -Force "$env:USERPROFILE\.platformio"
```

---

## 相关参考

- PlatformIO 官方：<https://docs.platformio.org/en/latest/core/userguide/cmd_run.html>
- `check_environment.py` 会自动检测 `PLATFORMIO_CORE_DIR`，输出 `platformio_core_dir` 字段
