---
name: future-tech-box-2.0-programmer
description: This skill should be used when users want to program the Future Tech Box 2.0 (未来科技盒2.0) board based on XIAO ESP32S3. It handles the complete workflow from natural language requirement to code generation, compilation and flashing. It also supports NEW CURRICULUM (新课标) project solution generation mode when teachers ask "how to build a project/work with the Future Tech Box" (e.g. 智能浇花系统/循迹小车方案), the skill generates standards-aligned teaching project plans based on 《义务教育信息科技课程标准（2022年版2025年修订）》 before coding. Trigger phrases include "编程未来科技盒2.0", "烧录程序到主板", "让LED亮起来", "控制电机", "如何用未来科技盒做", "未来科技盒方案", "未来科技盒作品设计", or any hardware/project design request mentioning 未来科技盒2.0/XIAO ESP32S3.
---

# 未来科技盒 2.0 自动编程烧录（v2.0 Skill）

## 概述
本 skill 实现从用户自然语言需求到代码生成、编译、烧录的完整自动化流程，并新增**新课标方案生成模式**：当教师提出「如何用未来科技盒做某某方案/作品」时，先输出符合《义务教育信息科技课程标准（2022年版2025年修订）》的项目方案，方案确认后再进入编程烧录主流程。

**目标硬件**：未来科技盒 2.0（基于 Seeed XIAO ESP32S3）  
**开发框架**：PlatformIO + Arduino  
**支持系统**：Windows / macOS / Linux  
**Skill 版本**：v0.3.0（新增新课标方案生成模式）

---

## 🌐 跨平台支持说明

本 SKILL 支持三种操作系统：

| 系统 | 串口命名 | 检测脚本 | 特殊说明 |
|------|----------|----------|----------|
| **Windows** | `COMx` | `detect_port_windows.py` | 使用 pnputil 检测 |
| **macOS** | `/dev/cu.usbmodem*` | `detect_port_macos.py` | 使用 system_profiler/ioreg |
| **Linux** | `/dev/ttyACM*` | `detect_port_linux.py` | 可能需要 dialout 用户组权限 |

### Linux 用户首次使用注意

如果遇到权限问题，需要将用户添加到 `dialout` 组：

```bash
sudo usermod -a -G dialout $USER
# 然后重新登录或重启
```

### macOS 用户注意

- 驱动通常自动安装
- 串口名称格式为 `/dev/cu.usbmodem*`
- 如果未识别到设备，可能需要安装 CH340 驱动（某些版本主板）

---

## ⚠️ 首次执行说明

首次使用本 skill 时，系统需要进行环境准备，包括：

### 需要下载的组件（首次自动下载）

| 组件 | 大小 | 说明 |
|------|------|------|
| **ESP32 平台包** | ~100-200MB | espressif32 平台定义 |
| **Arduino 框架** | ~50-100MB | arduino-esp32 框架 |
| **ESP32-S3 工具链** | ~200-300MB | xtensa-esp32s3-elf-gcc 编译器 |

### 首次编译耗时预估

- **网络良好**：5-10 分钟
- **网络一般**：10-20 分钟
- **后续编译**：10-30 秒（已缓存）

### ⚠️ 默认安装到非系统盘（强制，防止 C 盘爆满）

PlatformIO 默认把 **平台包 + Arduino 框架 + 工具链** 装在 `~/.platformio/`（系统盘）：
- **Windows**：`C:\Users\<用户名>\.platformio\`
- **macOS**：`/Users/<用户名>/.platformio/`
- **Linux**：`/home/<用户名>/.platformio/`

**实际总占用 5-6GB**（ESP32 平台包 ~500MB + Arduino 框架 ~500MB + 工具链 ~1.5GB + 包缓存/平台索引会随编译持续膨胀）。**默认装到 C 盘对系统盘紧张的用户不可接受。**

> 🚨 **本 skill 首次执行时，默认把 PlatformIO core 目录安装到非系统盘**（而不是默认 C 盘）。

#### 首次执行的强制流程（SKILL 必须执行）

```
Step 1: 运行环境检测
   python scripts/check_environment.py
   → 检查 platformio_core_source 字段

Step 2: 判断 core 目录是否在系统盘
   ├─ platformio_core_source == "PLATFORMIO_CORE_DIR"  → 已迁移到非系统盘 ✅ 跳过
   ├─ core 目录在非系统盘（如 D:\...）              → ✅ 无需处理
   └─ core 目录在系统盘（C 盘 / 默认位置）           → 必须执行 Step 3

Step 3: 一键迁移到非系统盘（默认引导）
   # 交互式：脚本列出所有磁盘 → 用户选择目标盘
   python scripts/migrate_core_dir.py
   # 或直接指定目标目录：
   python scripts/migrate_core_dir.py --target D:\DevTools\platformio

Step 4: 迁移完成后必须重启 IDE/终端（让 PLATFORMIO_CORE_DIR 生效）
   ⚠️ 未重启前不要继续编译！否则 pio 仍会写到 C 盘

Step 5: 重新运行 check_environment.py 验证
   → 确认 platformio_core_source 显示为 "PLATFORMIO_CORE_DIR"
   → 确认 platformio_core_dir 指向新目录
```

#### 为什么默认强制迁移

| 项 | 说明 |
|----|------|
| 实际占用 | **5-6GB**，且随编译持续增长 |
| 目标 | 首次 `pio run` 时组件**直接下载到非系统盘**，从一开始就不占 C 盘 |
| 无感知 | 用户只需选择目标盘（如 D 盘），其余全自动 |
| 兼容 | 新目录设置 `PLATFORMIO_CORE_DIR` 后，`pio` 全自动识别，无额外配置 |

#### 交互式迁移示例（用户实际看到的）

```
python scripts/migrate_core_dir.py

检测到的磁盘：
  C:  剩余 23.5GB
  D:  剩余 318.2GB
  E:  剩余 145.0GB

目标目录 (例如 D:\DevTools\platformio): D:\DevTools\platformio

即将把 core 目录从:
  C:\Users\<用户>\.platformio  (大小 5.6 GB)
复制到:
  D:\DevTools\platformio

✅ 迁移完成！
  新 core 目录: D:\DevTools\platformio
  ⚠️ 请关闭并重新打开 IDE / 终端，让环境变量生效。
```

#### 完整参数说明（migrate_core_dir.py）

```bash
python scripts/migrate_core_dir.py --check         # 仅检测现状
python scripts/migrate_core_dir.py                 # 交互式选择目标盘
python scripts/migrate_core_dir.py --target D:\Tools\pio  # 直接指定
python scripts/migrate_core_dir.py --target D:\Tools\pio --move  # 迁移后删 C 盘原目录
python scripts/migrate_core_dir.py --json          # JSON 输出（供 skill 解析）
```
1. **首次执行** `python scripts/check_environment.py`，确认 `platformio_core_source` 是 `PLATFORMIO_CORE_DIR` 而不是 `default`
2. 重新启动 IDE/终端，让所有子进程继承新变量
3. 首次 `pio run` 会自动在新目录下下载平台包、框架、工具链
4. 若想清理 C 盘旧的 `~/.platformio/`，可在确认新目录工作正常后手动删除

> PlatformIO 官方文档：<https://docs.platformio.org/en/latest/core/userguide/cmd_run.html#cmdoption-platformio-run-project-dir>

### 首次执行前的用户提示模板

在执行任何编程任务前，如检测到环境未就绪，向用户显示：

```
╔══════════════════════════════════════════════════════════════════╗
║          ⏳ 首次执行环境准备说明                                  ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                  ║
║  检测到这是首次使用未来科技盒编程功能，需要下载以下组件：          ║
║                                                                  ║
║  📦 ESP32 平台包 ................ 约 100-200 MB                 ║
║  📦 Arduino 框架 ................ 约 50-100 MB                  ║
║  📦 ESP32-S3 编译工具链 ......... 约 200-300 MB                 ║
║                                                                  ║
║  ⏱️  预计耗时：5-20 分钟（取决于网络速度）                        ║
║  💾 下载位置：~/.platformio/                                     ║
║                                                                  ║
║  下载完成后，后续编译将在 10-30 秒内完成。                        ║
║                                                                  ║
╠══════════════════════════════════════════════════════════════════╣
║  是否继续？请回复"确认"开始下载，或"取消"终止操作。               ║
╚══════════════════════════════════════════════════════════════════╝
```

### 🧳 离线安装包方案（国内/机房/无网络首选）

国内网络访问 PlatformIO Registry、dl.espressif.com、github 可能很慢甚至失败。
**推荐在一台能联网的电脑上提前打包好离线安装包，分发到教学电脑直接解压使用。**

#### 生成离线包（在【能联网】的电脑上执行一次）

前提：该电脑已成功完成过一次 espressif32 编译（core 数据齐全）。

```bash
python scripts/build_offline_package.py --output ./offline_package
# 生成: offline_package/future-tech-box-pio-<系统>-<架构>.zip （约 400-700MB）
```

**注意**：
- 包内只保留 espressif32 相关组件（平台包+工具链+Arduino 框架+esptool）
- **跨平台不通用**：Windows 的包只能在 Windows 用；如目标机是 macOS/Linux 需在对应系统打包
- 打包过程会同时下载 PlatformIO CLI 的 wheel（供离线 pip 安装）

#### 安装离线包（在【目标】电脑上执行）

```bash
# 方式 A：使用安装脚本（推荐，自动解压+装CLI+设环境变量）
python scripts/install_offline_package.py <离线包.zip>

# 方式 B：手动安装
# 1. 解压 zip 到任意目录（如 D:\DevTools\platformio）
# 2. 离线装 CLI: cd 解压目录/platformio_wheel && pip install --no-index --find-links=. platformio
# 3. 设置环境变量 PLATFORMIO_CORE_DIR=解压目录/platformio（需重启终端）
# 4. 验证: pio --core-dir 应输出该目录
```

#### 检测脚本对离线包的适配

`check_environment.py` 通过 `get_platformio_core_dir()` 定位 core 目录：
- 离线包解压并设置 `PLATFORMIO_CORE_DIR` 后，检测脚本会**自然判定组件就绪**（实时检测文件系统）
- 无需修改检测逻辑；`platformio_core_source` 会显示为 `PLATFORMIO_CORE_DIR`

#### 离线包放置建议

| 放置位置 | 是否合适 |
|----------|---------|
| 项目目录本地（如 `offline_package/`） | ✅ 推荐，但注意 zip 约 500MB，勿提交到 git |
| 网盘 / 学校共享盘 | ✅ 分发最方便 |
| git 仓库 | ❌ 太大，建议用 .gitignore 排除 |

---

## 执行流程

### Phase 0: 环境扫描

**执行脚本**：`python scripts/check_environment.py`

**检测项目**：
1. Python 版本（≥3.8）
2. PlatformIO CLI（≥6.0）
3. 串口连接状态
4. ESP32 平台包
5. ESP32-S3 工具链缓存状态
6. **Arduino 框架缓存状态（关键）**

**输出格式**：JSON
```json
{
  "python": {"installed": true, "version": "3.11.9", "ok": true},
  "platformio": {"installed": true, "version": "6.1.19", "ok": true},
  "serial": {"found": true, "port": "COM6", "vid": "303A", "pid": "1001", "ok": true},
  "platform": {"installed": true, "version": "6.13.0", "ok": true},
  "toolchain": {"cached": true, "path": "...", "ok": true},
  "framework": {
    "cached": true,
    "version": "3.20017.241212",
    "ok": true,
    "api_note": "Arduino Core 3.x uses ledcAttach(), 2.x uses ledcSetup()+ledcAttachPin()"
  },
  "ready": true,
  "warnings": [],
  "missing": [],
  "estimated_first_compile": "10-30 seconds"
}
```

> ⚠️ **关键预判点**：如果 `framework.cached` 为 `false`，首次编译将触发 Arduino 框架下载（50-100MB），
> 可能导致编译命令长时间无响应。必须在执行前告知用户。

**环境就绪后向用户显示**：
```
╔══════════════════════════════════════════════════════════════════╗
║            ✅ 环境检测通过                                        ║
╠══════════════════════════════════════════════════════════════════╣
║  [✓] Python 3.11.9                                               ║
║  [✓] PlatformIO 6.1.19                                           ║
║  [✓] 串口 COM6 (XIAO ESP32S3)                                    ║
║  [✓] ESP32-S3 工具链已缓存                                        ║
╠══════════════════════════════════════════════════════════════════╣
║  请输入您想要实现的功能，例如：                                    ║
  ║  • "让 LED 灯逐一亮起呼吸灯效果"                                  ║
  ║  • "按下按键 A 时 LED 全部点亮"                                   ║
  ║  • "让小车前进 3 秒后停止"                                        ║
╚══════════════════════════════════════════════════════════════════╝
```

---

### Phase 0.5: 方案生成模式（新课标对齐，可选前置阶段）

**触发条件**：用户请求是「方案/作品/项目设计」类（如"如何用未来科技盒做智能浇花系统"、"设计一个循迹小车方案"、"未来科技盒能做什么作品"），而非直接要求"写代码/烧录"。

**处理**：
1. 判断请求类型：方案设计类 → 进入本阶段；直接编程类 → 跳过，进入 Phase 1
2. 读取 `references/new_curriculum_solution_design.md` 获取新课标参考（核心素养四维、学段定位、硬件映射、方案输出结构、方案方向库）
3. 按该文件「四、方案输出结构」输出符合课标的完整项目方案（含核心素养目标、系统架构、硬件选型、任务驱动教学过程、评价设计）
4. 方案末尾引导用户确认，确认后再进入 Phase 1 生成代码

> 方案模式核心约束：只描述方案，不写代码；硬件选型必须以参考文档「三、硬件能力映射」为准，不得虚构硬件；学段定位与课标内容模块要匹配。

---

### Phase 1: 需求理解与代码生成

**输入**：用户自然语言描述（若经过 Phase 0.5，则为已确认的方案）  
**处理**：
1. 解析用户意图，识别涉及的硬件模块
2. 读取 `references/pinout_mapping.csv` 获取引脚映射
3. 读取 `references/future_tech_box_v2_hardware.md` 获取硬件约束
4. 若用户在方案阶段指定了学段/课标要求，代码需与方案中的硬件选型一致
5. 生成符合 PlatformIO 结构的代码

**代码生成前向用户显示**：
```
📝 正在生成代码...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
识别到的硬件模块：
  • LED 矩阵 (GPIO33-38)
  • [其他涉及的模块]

即将生成的文件：
  • src/main.cpp
  • include/pins.h
  • platformio.ini
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

### Phase 2: 编译

**执行命令**：`pio run -d <project_path>`

> ⚠️ **重要提示**：由于 CodeBuddy 的命令执行有超时限制，如果编译命令长时间无响应，
> 应引导用户在终端手动执行编译命令。这不是错误，而是正常的超时保护机制。

**编译前向用户显示**：
```
🔨 正在编译程序...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
目标平台：ESP32-S3 (XIAO ESP32S3)
框架：Arduino
预计耗时：10-30 秒（首次编译可能需要更长时间）

⏳ 编译中，请稍候...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**如果编译命令被跳过或超时，向用户显示**：
```
⏱️ 编译命令执行超时
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
这是正常现象，编译过程可能需要较长时间。

请在终端手动执行以下命令完成编译和烧录：

1. 打开终端（按 Ctrl+` 或在 VS Code 菜单选择"终端"）

2. 执行编译：
   cd "<项目完整路径>"
   pio run

3. 编译成功后执行烧录（esptool 直调，--after no_reset 不自动运行）：
   python %USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py ^
     --chip esp32s3 --port <端口> --baud 921600 ^
     --before default_reset --after no_reset ^
     write_flash -z --flash_mode dio --flash_freq 80m --flash_size 8MB ^
     0x0 .pio\build\seeed_xiao_esp32s3\bootloader.bin ^
     0x8000 .pio\build\seeed_xiao_esp32s3\partitions.bin ^
     0x10000 .pio\build\seeed_xiao_esp32s3\firmware.bin

4. 烧录完成后重新开关主板电源，程序才会运行

5. （可选）重新上电后查看串口输出：
   pio device monitor
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**编译成功后向用户显示**：
```
✅ 编译成功！
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
固件大小：xxx KB (占用 xx%)
RAM 使用：xxx KB (占用 xx%)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**编译失败处理**：
1. 解析错误信息
2. 尝试自动修复（语法错误、缺失头文件等）
3. 最多重试 2 次
4. 仍失败则向用户报告具体错误

---

### Phase 3: 烧录

> ⚠️ **重要：采用「先烧录，失败后再确认」策略**
>
> **不需要**在烧录前弹出确认框让用户点击。直接进行烧录，过程中提醒用户，如果失败再让用户确认重启后继续。

> 🚨 **关键经验：必须显式指定串口端口**
>
> **XIAO ESP32S3 使用 USB-Serial/JTAG 模式**，PlatformIO 的自动端口检测在此模式下**不可靠**，
> 可能出现：自动选错端口、检测超时、烧录看似成功但实际未写入等问题。
>
> **强制要求**：
> 1. 烧录命令必须使用 esptool 直调 + `--after no_reset`（禁止 `pio run -t upload`，详见 3.1）
> 2. 必须显式指定串口端口
> ```bash
> # ✅ 正确写法：esptool 直调 + 显式端口 + --after no_reset（见 3.1 完整命令）
> python %USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py \
>   --chip esp32s3 --port COM11 --baud 921600 \
>   --before default_reset --after no_reset write_flash ...
>
> # ❌ 错误写法1：依赖自动检测（可能选错端口）
> pio run -t upload
>
> # ❌ 错误写法2：pio upload（烧录完会 hard_reset 自动运行程序）
> pio run -t upload --upload-port COM11
> ```
>
> **端口获取方式**：在烧录前先运行 `pio device list` 获取当前连接的端口号。

#### 3.1 烧录执行策略

**主烧录命令**：先用 `pio run` 编译生成固件，再用 **esptool 直调 + `--after no_reset`** 烧录。

> 🚨 **为什么不用 `pio run -t upload`？**
>
> `pio run -t upload` 默认在烧录完成后会 `hard_reset`（自动运行程序）。
> 这会导致烧录完成瞬间电机/舵机被驱动，极易触发笔记本 USB 接口"电流过载提醒"（USB 口额定仅 0.5-3A，电机启动瞬间可达 1.5-2.5A）。
>
> 因此**必须使用 esptool 直调并加 `--after no_reset`**，让烧录完成后停留在下载模式，不自动运行程序。

**⚠️ 烧录前必须先获取端口**：
```bash
# Step 1: 获取端口号
pio device list
# 找到 VID:PID=303A:1001 的设备，记录端口号（如 COM11）

# Step 2: 编译固件（确保最新代码）
pio run -d <project_path>

# Step 3: 使用 esptool 直调烧录（--after no_reset 保证烧录后不自动运行）
# Windows:
python %USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py ^
  --chip esp32s3 --port <PORT> --baud 921600 ^
  --before default_reset --after no_reset ^
  write_flash -z --flash_mode dio --flash_freq 80m --flash_size 8MB ^
  0x0 <project_path>\.pio\build\seeed_xiao_esp32s3\bootloader.bin ^
  0x8000 <project_path>\.pio\build\seeed_xiao_esp32s3\partitions.bin ^
  0x10000 <project_path>\.pio\build\seeed_xiao_esp32s3\firmware.bin

# macOS / Linux:
python ~/.platformio/packages/tool-esptoolpy/esptool.py \
  --chip esp32s3 --port <PORT> --baud 921600 \
  --before default_reset --after no_reset \
  write_flash -z --flash_mode dio --flash_freq 80m --flash_size 8MB \
  0x0 <project_path>/.pio/build/seeed_xiao_esp32s3/bootloader.bin \
  0x8000 <project_path>/.pio/build/seeed_xiao_esp32s3/partitions.bin \
  0x10000 <project_path>/.pio/build/seeed_xiao_esp32s3/firmware.bin
```

**说明**：
- `--before default_reset`：esptool 通过 USB-Serial/JTAG 的 DTR/RTS 自动让芯片进入下载模式，**无需用户手动按 BOOT**
- `--after no_reset`：烧录完成后芯片停留在下载模式（ROM bootloader），**不自动运行程序**
- 用户重新开关主板电源（或拔插 USB）后，新程序才会开始运行

**烧录开始时显示**（仅作提醒，不需要等待用户确认）：
```
📤 正在烧录到主板...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
目标端口：COM6
烧录速度：921600 bps
预计耗时：10-20 秒

⚠️  烧录过程中请勿断开 USB 连接！
💡 如遇烧录失败，可能需要按一下 RST 复位按钮后重试
💡 烧录完成后程序不会自动运行，需重新开关电源（防 USB 过载）

⏳ 烧录中，请稍候...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

#### 3.2 烧录失败自动处理流程

**如果主命令失败（错误码非 0 或超时），执行自动重试策略：**

```
烧录失败自动处理流程：
┌─────────────────────────────────────────────────────────┐
│  Step 1: 自动等待 3 秒后重试 esptool 直调烧录           │
│          最多自动重试 2 次（无需用户确认）               │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  Step 2: 如果 2 次自动重试仍失败                        │
│          → 向用户显示提醒，要求确认重启后继续            │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  Step 3: 用户确认后，再重试烧录                         │
│          仍失败则提示按 RST 后重试                      │
└─────────────────────────────────────────────────────────┘
```

**自动重试时显示**（简短提示）：
```
⏳ 烧录遇到问题，自动重试中 (第 x 次)...
```

#### 3.3 自动重试失败后的用户交互

**只有在自动重试 2 次仍失败后，才向用户显示确认框：**

```
╔══════════════════════════════════════════════════════════════════╗
║          ⚠️ 烧录失败，需要您的操作                                ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                  ║
║  自动重试 2 次后仍然失败。                                        ║
║  可能是上一个程序正在运行，导致串口被占用。                        ║
║                                                                  ║
║  请执行以下操作后回复"继续"：                                     ║
║                                                                  ║
║  ✅ 【推荐】按一下主板上的 RST（复位）按钮                        ║
║                                                                  ║
║  或者：                                                           ║
║  • 拔插一次 USB 数据线                                            ║
║  • 关闭可能占用串口的程序（串口监视器、Arduino IDE 等）           ║
║                                                                  ║
╚══════════════════════════════════════════════════════════════════╝
```

#### 3.4 备用烧录命令

**如果主烧录命令失败，再次使用 esptool 直调（与主命令一致，同样带 `--after no_reset`）**：
```bash
python %USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py ^
  --chip esp32s3 --port {PORT} --baud 921600 ^
  --before default_reset --after no_reset ^
  write_flash -z --flash_mode dio --flash_freq 80m --flash_size 8MB ^
  0x0 {PROJECT}\.pio\build\seeed_xiao_esp32s3\bootloader.bin ^
  0x8000 {PROJECT}\.pio\build\seeed_xiao_esp32s3\partitions.bin ^
  0x10000 {PROJECT}\.pio\build\seeed_xiao_esp32s3\firmware.bin
```

#### 3.5 烧录成功提示

**烧录成功后显示**：
```
🎉 烧录成功！
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
程序已写入主板。

⚠️ 程序【不会自动运行】。
请重新开关主板电源（或拔插一次 USB 线）后，程序才会开始运行。

如需查看串口输出，请先重新上电，再说"打开串口监视器"。
如需修改程序，请直接描述新的需求。
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

## 支持的硬件功能

| 功能 | 状态 | 示例指令 |
|------|------|----------|
| LED 矩阵 | ✅ | "让 LED 呼吸灯效果" |
| 按键 | ✅ | "按下按键 A 时..." |
| 超声波传感器 | ✅ | "测量前方距离" |
| **电机控制** | ✅ | "让小车前进"、"原地左转" |
| **循迹传感器** | ✅ | "实现循迹小车" |
| **舵机/机械臂** | ✅ | "控制机械臂抓取物体" |
| **PS2 手柄** | ✅ | "用手柄遥控小车" |
| **I2C 加速度计** | ✅ | "检测倾斜方向" |
| **I2C 颜色传感器** | ✅ | "识别颜色" |
| **I2C 温湿度** | ✅ | "读取当前温度" |
| **WiFi Web 遥控** | ✅ | "用电脑/手机网页控制小车" |
| **USB 串口→网页控制（Web Serial API）** | ✅ | "USB 连电脑控制网页"、"主板当遥控器"、"按钮控制网页"、"手柄控制网页游戏" |
| **FreeRTOS 多任务** | ✅ | "同时循迹+颜色识别+超声波避障" |
| **蜂鸣器** | ✅ | "蜂鸣器响一声"、"播放音乐"、"提示音" |
| **语音模块 (ASR Pro)** | 🔌 | 预刷程序，串口通讯控制，暂不需要编程 |

✅ = 已支持 &emsp; ❌ = 当前版本不支持 &emsp; 🔌 = 外部模块（无需编程）

---

## 🎯 新课标方案生成模式（v0.3.0 新增）

### 何时触发
当用户提出**方案/作品/项目设计**类请求时触发（而不是直接要求写代码）：
- "如何用未来科技盒做 XX 方案/作品？"
- "设计一个 XX 项目的实施方案"
- "未来科技盒能做什么符合新课标的作品？"
- "为 X 年级学生设计用未来科技盒做的 XX 作品"

### 触发后的流程
1. 读取 `references/new_curriculum_solution_design.md`
2. 输出符合《义务教育信息科技课程标准（2022年版2025年修订）》的项目方案
3. 方案末尾询问用户是否进入编程烧录流程

### 方案输出结构（严格遵循参考文档第四节）
```
## 一、项目基本信息
## 二、课标对齐（核心素养四维 + 学段目标呼应）
## 三、项目概述（驱动问题 + 输入→计算→输出系统架构）
## 四、特色与玩法设计（主题风格 + 地区/文化特色 + 游戏化玩法 + 趣味呈现）
## 五、硬件选型清单
## 六、任务驱动教学过程（情境导入→方案设计→搭建编程→测试优化→拓展迁移）
## 七、评价设计（素养导向，教学评一体化）
## 八、注意事项与降级方案
## 九、非接线创客内容（结构搭建 / 沙盘场景 / 美术道具 / 功能件制作）
## 十、下一步（引导进入编程模式）
```

### 核心约束
1. **只出方案，不出代码**：方案阶段不写 C++/Arduino 代码，代码留到用户确认方案后（Phase 1）
2. **硬件不虚构**：硬件选型必须来自参考文档「三、硬件能力映射」及本 SKILL 引脚表，禁止发明不存在的传感器/接口
3. **课标对齐**：核心素养从参考文档「一、核心素养速查」取用；学段与内容模块从「二、内容模块与学段定位」取用
4. **接口限制合规**：2.0 循迹引脚为 GPIO2/GPIO1、LED 矩阵需 scanDisplay 维持显示、ASR Pro 无需编程，方案设计不得违反
5. **学段适配**：教师指定学段时按该学段目标定难度；未指定时默认推荐 5～6 年级或 7～9 年级并说明理由
6. **方案方向库**：教师问"能做什么"时，先给出参考文档「八、方案方向库」清单，再引导选择其一深入设计
7. **技术克制（第一准则）**：默认只用低难度、已验证、易采买模块（按键/LED矩阵/蜂鸣器/超声波/循迹/温湿度/加速度计/颜色/电机/舵机/PS2/WiFi）。**不主动引导 GPS/TWD 定位、云平台大屏、多机协同、复杂 AI 等高难度功能**；除非用户明确要求，否则不写进方案主体。用户要求时可补充，但必须标注采买/接线/供电风险并给替代建议
8. **特色与风格优先**：方案侧重地区/文化特色、主题风格、游戏化玩法、趣味呈现，而非堆技术。先想"好不好玩、有没有特色"，再想"用什么硬件"
9. **技术可落地**：严格按用户描述要求实现；涉及接线/供电不确定的器件（继电器水泵、GPS、大功率电机）标注"需教师确认接线与供电"；优先用板载模块，外接器件降到最少
10. **非接线创客内容覆盖**：凡作品含实体形态/场景/道具，必须给出结构搭建（瓦楞纸/雪弗板/3D打印/乐高）、沙盘模型、美术道具、功能件制作等简述及安全提示
11. **方案主次结构**：主线（基础可落地玩法 + 特色/趣味）必须完整可执行；可选进阶（用户要求才展开）不影响主线完整性

### 方案 → 代码衔接
用户确认方案后，提示：
"已确认方案，接下来我将基于该方案生成代码并编译烧录。请确保：① 硬件已按方案接线；② USB 数据线已连接主板。"
然后进入 **Phase 1** 正常流程。方案中的硬件选型将决定 Phase 1 的代码生成（引脚、库、模块均按方案执行）。

---

### ⚠️ 语音模块 ASR Pro 说明

本版本搭载的语音模块为 **ASR Pro**，其工作方式如下：

- **无需编程**：ASR Pro 模块已预刷好程序，上电即可工作
- **通讯方式**：通过引脚串口通讯（UART）与 ESP32-S3 主控交互
- **控制逻辑**：ASR Pro 识别语音指令后，通过串口发送控制命令给 ESP32-S3，由主控执行相应动作
- **后续计划**：后续可能独立为 ASR Pro 编写自定义程序，届时将新增对应的 Skill 支持

> 📌 当用户要求"语音控制"功能时，只需在 ESP32-S3 端编写**串口接收和解析**的代码，
> 不需要对 ASR Pro 本身进行编程。ASR Pro 发送的指令格式取决于其预刷固件。

### 小车形态电机布局

```
        前方
    ┌─────────┐
    │  M1  M2 │   M1=左上(GPIO11/12)  M2=右上(GPIO14/13)
    │         │
    │  M3  M4 │   M3=左下(GPIO15/16)  M4=右下(GPIO18/17)
    └─────────┘
        后方
```

**⚠️ 重要**：右侧电机 (M2, M4) 的正反转 GPIO 顺序与左侧相反：
- M1(左上): GPIO11=正转, GPIO12=反转
- M2(右上): GPIO14=正转, GPIO13=反转 ← 注意顺序
- M3(左下): GPIO15=正转, GPIO16=反转
- M4(右下): GPIO18=正转, GPIO17=反转 ← 注意顺序

**电机控制代码模板**：
```cpp
#define M1_FWD 11
#define M1_REV 12
#define M2_FWD 14  // 注意：不是13
#define M2_REV 13
#define M3_FWD 15
#define M3_REV 16
#define M4_FWD 18  // 注意：不是17
#define M4_REV 17

void setMotors(int leftSpeed, int rightSpeed) {
  // leftSpeed/rightSpeed: -255~255, 正数前进，负数后退
  setMotor(M1_FWD, M1_REV, leftSpeed);
  setMotor(M2_FWD, M2_REV, rightSpeed);
  setMotor(M3_FWD, M3_REV, leftSpeed);
  setMotor(M4_FWD, M4_REV, rightSpeed);
}

// 前进: setMotors(180, 180);  // 默认 70% 功率
// 后退: setMotors(-180, -180);
// 左转: setMotors(-180, 180);
// 右转: setMotors(180, -180);
```

### 麦克纳姆轮全向移动

如果小车使用麦克纳姆轮，可以实现横移和斜向移动：

| 方向 | M1(左上) | M2(右上) | M3(左下) | M4(右下) |
|------|----------|----------|----------|----------|
| 前进 | 正 | 正 | 正 | 正 |
| 后退 | 反 | 反 | 反 | 反 |
| **左横移** | **反** | **正** | **正** | **反** |
| **右横移** | **正** | **反** | **反** | **正** |
| 左前楔形 | 停 | 正 | 正 | 停 |
| 右前楔形 | 正 | 停 | 停 | 正 |
| 左后楔形 | 反 | 停 | 停 | 反 |
| 右后楔形 | 停 | 反 | 反 | 停 |
| 原地左转 | 反 | 正 | 反 | 正 |
| 原地右转 | 正 | 反 | 正 | 反 |

**麦克纳姆轮控制代码**：
```cpp
// ⚠️ 默认速度建议 180（70%功率），避免触发电池保护
const int MOTOR_SPEED = 180;

// 设置四个电机独立速度
void setMotorsSeparate(int m1, int m2, int m3, int m4) {
  setMotor(M1_FWD, M1_REV, m1);
  setMotor(M2_FWD, M2_REV, m2);
  setMotor(M3_FWD, M3_REV, m3);
  setMotor(M4_FWD, M4_REV, m4);
}

// 左横移
void carMoveLeft(int speed = MOTOR_SPEED) {
  setMotorsSeparate(-speed, speed, speed, -speed);
}

// 右横移
void carMoveRight(int speed = MOTOR_SPEED) {
  setMotorsSeparate(speed, -speed, -speed, speed);
}

// 左前楔形
void carMoveFrontLeft(int speed = MOTOR_SPEED) {
  setMotorsSeparate(0, speed, speed, 0);
}

// 右前楔形
void carMoveFrontRight(int speed = MOTOR_SPEED) {
  setMotorsSeparate(speed, 0, 0, speed);
}
```

### I²C 传感器接口

| 传感器 | 型号 | I²C 地址 | 库名称 |
|--------|------|----------|--------|
| 加速度计 | LIS3DHTR | 0x18 | `LIS3DHTR` |
| 颜色传感器 | VEML6040 | 0x10 | `VEML6040` |
| 温湿度 | DHT20 | 0x38 | `Grove Temperature And Humidity Sensor` |

**I²C 初始化必须指定引脚**：

未来科技盒有两组 I2C 接口，引脚不同，使用时需根据传感器实际插入的接口选择对应引脚：

| I2C 组 | 对应接口 | SDA | SCL | 说明 |
|--------|----------|-----|-----|------|
| I2C1（默认） | 接口1 / 接口8 | GPIO39 | GPIO40 | 大部分传感器默认使用 |
| I2C2 | 接口7 | GPIO37 | GPIO36 | 摄像头AI板或第二组I2C设备 |

```cpp
#include <Wire.h>

// 传感器插在接口1或接口8时（默认）
Wire.begin(39, 40);  // SDA=GPIO39, SCL=GPIO40

// 传感器插在接口7时
// Wire.begin(37, 36);  // SDA=GPIO37, SCL=GPIO36

// 如需同时使用两组I2C，使用Wire1：
// Wire1.begin(37, 36);  // 第二组I2C: SDA=GPIO37, SCL=GPIO36
```

### 🔴 VEML6040 颜色传感器使用注意事项（重要！必须告知用户）

**当用户使用颜色传感器时，SKILL 必须在烧录成功后向用户显示以下提示：**

```
⚠️ 颜色传感器使用注意事项
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📌 使用环境要求：
  1. 请在相对封闭、光线稳定的环境下使用颜色传感器
  2. 避免强光直射（日光灯直射、阳光照射等）
  3. 避免光线频繁变化的场所（如灯光闪烁、有人走动遮光）

📌 校准说明：
  • 开机后前几秒 LED 全亮 = 正在自动校准环境光基线
  • 校准时请确保传感器前方没有放置彩色物体
  • 校准完成后会有两声提示音

📌 重要提示：
  • 如果更换了使用环境（开/关灯、移到不同房间），
    请按 RST 按钮重新启动以重新校准
  • 颜色识别准确度取决于校准时的环境光是否稳定
  • 将彩色物体尽量靠近传感器（1-3cm 距离最佳）
  • 物体表面最好是纯色，混合色可能识别不准确
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**VEML6040 识别原理与限制（SKILL 内部参考，不必直接展示给用户）**：

1. **自适应基线校准**：程序开机时采集 8 次环境光 RGB 占比求平均，作为"无颜色"的基准参考
2. **相对偏离检测**：当彩色物体放到传感器前，RGB 占比会偏离基线，通过偏离方向和幅度判断颜色
3. **饱和度门槛**：偏离幅度之和（saturation）< 0.025 时判定为无颜色/环境光
4. **局限性**：
   - 环境光变化会导致基线失效，必须重新校准（按 RST）
   - VEML6040 对红色的灵敏度较高，对蓝色灵敏度偏低
   - 室内 LED 灯光通常偏蓝（色温 5000K+），会影响基线
   - 距离越远，颜色信号越弱，识别越不准确
5. **代码实现**：使用 `float baseR/baseG/baseB` 存储基线，`calibrateBaseline()` 在 setup() 中自动调用

**🚨 I²C 传感器编码最佳实践**：

1. **必须在初始化前进行 I2C 扫描**（方便调试）：
```cpp
void scanI2C() {
  Serial.println("正在扫描 I2C 设备...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  找到设备: 0x");
      Serial.println(addr, HEX);
    }
  }
}
```

2. **传感器初始化失败时必须提供降级模式**，不要进入死循环：
```cpp
// ❌ 错误写法：传感器未找到就死循环
if (!sensor) {
  while (1) { 闪灯; }  // 用户无法判断程序是否烧录成功
}

// ✅ 正确写法：切换到降级/演示模式
bool sensorFound = false;
if (!sensor) {
  Serial.println("传感器未检测到，进入演示模式");
  sensorFound = false;
  // 继续运行，用 LED 演示模式证明程序已烧录成功
}
```

3. **setup() 延迟必须 ≥ 2000ms**：
```cpp
void setup() {
  Serial.begin(115200);
  delay(2000);  // ✅ USB-Serial/JTAG 模式需要足够的枚举时间
}
```

### PS2 手柄引脚

| 功能 | GPIO |
|------|------|
| CLK | GPIO41 |
| CMD | GPIO9 |
| CS | GPIO42 |
| DAT | GPIO10 |

### 循迹传感器引脚

| 传感器 | GPIO | 接口 |
|--------|------|------|
| 左循迹 | GPIO2 | 接口3-PIN1 |
| 右循迹 | GPIO1 | 接口3-PIN2 |

> ⚠️ **3.0 版本循迹传感器引脚不同！**
> - 2.0：左=GPIO2，右=GPIO1
> - 3.0：左=GPIO3，右=GPIO4
> - 生成循迹代码前必须确认目标版本！

### 超声波传感器引脚（接口2）

超声波传感器插在**接口2**时的引脚映射：

| 接口2引脚 | GPIO | 超声波模块引脚 | 说明 |
|-----------|------|---------------|------|
| PIN1 | GPIO8 | NC（未连接） | 该引脚无用，不需要操作 |
| PIN2 | GPIO7 | SIG（信号） | **唯一有效引脚**，既做 Trig 又做 Echo |

**⚠️ 这是单线超声波模块（SIG 模式）**，只使用 GPIO7 一个引脚完成距离测量。

**工作原理**：
1. 将 GPIO7 设为 OUTPUT，发送 10μs 高电平触发脉冲（Trig）
2. 将 GPIO7 切换为 INPUT，读取回波高电平持续时间（Echo）
3. 根据声速计算距离：距离(cm) = 脉冲时间(μs) / 58

**超声波传感器代码模板**：
```cpp
#define ULTRASONIC_SIG 7  // 接口2-PIN2 (GPIO7)，单线 SIG 模式

// 读取超声波距离（单位：cm）
float readUltrasonic() {
  // Step 1: 发送触发脉冲
  pinMode(ULTRASONIC_SIG, OUTPUT);
  digitalWrite(ULTRASONIC_SIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_SIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_SIG, LOW);
  
  // Step 2: 切换为输入，读取回波
  pinMode(ULTRASONIC_SIG, INPUT);
  long duration = pulseIn(ULTRASONIC_SIG, HIGH, 30000);  // 超时 30ms（约 5m）
  
  // Step 3: 计算距离
  if (duration == 0) {
    return -1;  // 超时，无有效回波
  }
  float distance = duration / 58.0;
  return distance;
}
```

**使用示例**：
```cpp
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("超声波传感器测试 (接口2, SIG 模式)");
}

void loop() {
  float dist = readUltrasonic();
  if (dist > 0) {
    Serial.print("距离: ");
    Serial.print(dist, 1);
    Serial.println(" cm");
  } else {
    Serial.println("超出范围");
  }
  delay(100);  // 测量间隔至少 60ms，建议 100ms
}
```

**⚠️ 超声波传感器注意事项**：
1. **GPIO8 不需要初始化**，它连接的是超声波模块的 NC（空引脚），忽略即可
2. **测量间隔**：两次测量之间至少间隔 60ms，推荐 100ms，避免前次回波干扰
3. **有效量程**：约 2cm ~ 400cm，过近或过远会返回 -1
4. **pulseIn 超时**：设置为 30000μs（约 5m 距离），超时返回 0 表示无回波
5. **与 LED 扫描共存**：`pulseIn()` 有阻塞等待（最长 30ms），如果同时使用 LED 矩阵扫描，建议将超声波测量放在 FreeRTOS 独立任务中，避免影响 LED 刷新

### ⚠️ LED 矩阵控制要点（必读）

**这是 3×3 行列扫描 LED 矩阵，控制逻辑如下：**

| 行引脚状态 | 列引脚状态 | LED 状态 |
|-----------|-----------|----------|
| **HIGH**  | **LOW**   | ✅ 点亮  |
| HIGH      | HIGH      | ❌ 熄灭  |
| LOW       | LOW       | ❌ 熄灭  |
| LOW       | HIGH      | ❌ 熄灭  |

**⚠️ LED 物理位置映射（重要！）**：
```
物理视角（面对主板）：
     列1(36)  列2(37)  列3(38)
行3(35)  1        2        3      ← 物理上方
行2(34)  4        5        6      ← 物理中间
行1(33)  7        8        9      ← 物理下方
```

**LED 编号转数组索引公式**：
```cpp
int row = 2 - ((num - 1) / 3);  // LED 1-3→row 2, LED 4-6→row 1, LED 7-9→row 0
int col = (num - 1) % 3;
```

**生成 LED 控制代码时必须遵循：**
1. 使用 `ledState[3][3]` 数组存储 LED 状态（1=点亮，0=熄灭）
2. 在 `loop()` 中持续调用 `scanDisplay()` 扫描函数
3. 使用 `setLED(num, state)` 函数按编号（1-9）控制 LED
4. 点亮 LED：`setLED(num, 1)` 或 `turnOnLED(num)`
5. 熄灭 LED：`setLED(num, 0)` 或 `turnOffLED(num)`
6. **禁止**直接用 `digitalWrite(列引脚, HIGH/LOW)` 来控制单个 LED

**详细示例代码参见**：`references/future_tech_box_v2_hardware.md` 的 LED 矩阵章节

### ⚠️ 按键控制要点（必读）

**按键使用内部上拉电阻，按下时为 LOW，松开时为 HIGH。**

| 按键 | GPIO | 说明 |
|------|------|------|
| KEY_A | GPIO21 | 按下时 LOW |
| KEY_B | GPIO0 | 按下时 LOW（兼 BOOT） |

**🚨 必须使用边沿检测方式检测按键！**

**场景1：无 LED 矩阵扫描时（可以用 delay 消抖）**
```cpp
bool lastKeyA = HIGH;

void loop() {
  bool currentKeyA = digitalRead(KEY_A);
  
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    doSomething();
    delay(50);  // ✅ 没有 LED 扫描时可以用 delay
  }
  
  lastKeyA = currentKeyA;
}
```

**场景2：有 LED 矩阵扫描时（禁止用 delay！）**
```cpp
bool lastKeyA = HIGH;
unsigned long lastKeyTime = 0;  // 用于非阻塞消抖

void loop() {
  bool currentKeyA = digitalRead(KEY_A);
  
  // 边沿检测 + 非阻塞消抖
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    if (millis() - lastKeyTime > 50) {  // 50ms 消抖
      doSomething();
      lastKeyTime = millis();
    }
  }
  
  lastKeyA = currentKeyA;
  scanDisplay();  // ⚠️ 不能被 delay 阻塞！
}
```

**生成按键控制代码时必须遵循：**
1. 使用边沿检测 `if (lastKey == HIGH && currentKey == LOW)`
2. 必须更新状态 `lastKey = currentKey`
3. **有 LED 扫描时禁止使用 `delay()` 消抖**，改用 `millis()` 非阻塞方式
4. 为每个按键创建独立的状态变量

### 蜂鸣器控制要点

- **引脚**：GPIO26（有源/无源蜂鸣器）
- **基本用法**：`tone(26, freq, duration)` / `noTone(26)`
- **LEDC 方式**（更灵活）：`ledcAttach(26, freq, 8)` + `ledcWriteTone(26, freq)` / `ledcWriteTone(26, 0)` 停止
- **与 LED 矩阵共存时必须用非阻塞方式**：

```cpp
#define BUZZER_PIN 26
unsigned long buzzerEndTime = 0;
bool buzzerActive = false;

void startBuzzer(unsigned long duration) {
  tone(BUZZER_PIN, 1000);
  buzzerEndTime = millis() + duration;
buzzerActive = true;
}

void updateBuzzer() {
  if (buzzerActive && millis() >= buzzerEndTime) {
    noTone(BUZZER_PIN);
    buzzerActive = false;
  }
}

void loop() {
  scanDisplay();  // LED 矩阵扫描
  updateBuzzer(); // 非阻塞检查蜂鸣器
}
```

### ⚠️ 非阻塞编程原则（重要！）

**当程序中有 LED 矩阵扫描时，`loop()` 中禁止使用任何阻塞操作！**

```cpp
void loop() {
  // ❌ 禁止：
  delay(50);           // 阻塞 50ms
  while(条件) { ... }  // 可能长时间阻塞
  
  // ✅ 必须：
  scanDisplay();       // 每次 loop 都要调用，不能被阻塞
}
```

**所有需要延时的操作都改用 `millis()` 非阻塞方式：**
- 按键消抖：用 `millis() - lastKeyTime > 50` 代替 `delay(50)`
- 动画效果：用 `millis() - lastFrameTime > interval` 代替 `delay(interval)`
- 定时操作：用 `millis() >= endTime` 代替 `delay(duration)`

---

## 引用资源

- **引脚映射**：`references/pinout_mapping.csv`
- **硬件规格**：`references/future_tech_box_v2_hardware.md`
- **新课标方案设计参考**：`references/new_curriculum_solution_design.md`（方案生成模式必读：核心素养四维、学段定位、硬件映射、方案输出结构、方案方向库）
- **USB 串口→网页控制参考**：`references/web_serial_guide.md`（Web Serial API：主板固件模板 + 网页 HTML 模板 + 前置条件 + 排查表）
- **环境检测**：`scripts/check_environment.py`（自动检测操作系统）
- **烧录脚本**：`scripts/upload_with_retry.py`（带自动重试）
- **串口识别**（根据系统自动选择）：
  - Windows：`scripts/detect_port_windows.py`
  - macOS：`scripts/detect_port_macos.py`
  - Linux：`scripts/detect_port_linux.py`

---

## 项目模板结构

生成的 PlatformIO 项目结构：

```
<project_name>/
├── platformio.ini      # 平台配置
├── include/
│   └── pins.h          # 引脚定义
├── src/
│   └── main.cpp        # 主程序
└── lib/                # 第三方库（按需）
```

---

## 约束与限制

1. 仅支持未来科技盒 2.0（XIAO ESP32S3）和 3.0（ESP32-S3 QFN56）
2. 需要稳定的网络连接（首次下载依赖）
3. USB 必须是数据线（非充电线）
4. 某些复杂功能可能需要用户确认细节
5. Linux 用户需要 dialout 用户组权限
6. **烧录时必须显式指定串口端口**，禁止依赖 PlatformIO 自动检测
7. **烧录必须使用 esptool 直调 + `--after no_reset`**（禁止 `pio run -t upload`，因其会 hard_reset 自动运行程序，烧录完成瞬间驱动电机可能触发笔记本 USB 电流过载提醒）。烧录完成后程序不自动运行，需提示用户**重新开关主板电源**后运行
8. **`setup()` 中 `delay()` 必须 ≥ 2000ms**，确保 USB 串口稳定
9. **I2C 传感器必须提供降级模式**，初始化失败不能进入死循环
10. **PlatformIO 命令优先使用 `python -m platformio`**，如果 `pio.exe` 被系统策略阻止则必须使用此方式（见问题 8）
11. **区分 2.0 和 3.0 硬件**：生成代码前必须确认目标版本，LED 控制方式完全不同（见问题 9）
12. **3.0 版本 RGB LED 亮度建议 20-50**，避免 LED 过亮刺眼和电流过大
13. **3.0 版本 platformio.ini 必须配置 CH343 串口**：`board = esp32-s3-devkitc-1` + `ARDUINO_USB_CDC_ON_BOOT=0`（见问题 10）
14. **NeoPixel 亮度调节禁止闪白反馈**：`setBrightness()` 后必须直接用当前颜色 `show()`，不得插入闪白效果（见问题 11）
15. **PS2 手柄**（⚠️ v2 架构，关键约束）：
   - 需要 `PS2X_lib` 库（ESP32 版本，手动放入 `lib/PS2X_lib/`）
   - 初始化时需重试机制（最多 3 次）
   - **必须使用硬件定时器中断（`hw_timer_t`）每 100ms 读取手柄**，禁止在 `loop()` 中直接调用 `read_gamepad()`
   - **必须使用 8 通道 `motor_pwm[8]` 数组 + `motor_pwm_num[4][2]` 映射表**控制电机，禁止用 `setMotor(FWD, REV, speed)` 简单封装
   - `loop()` 中只负责将 `motor_pwm[]` 数组值输出到 GPIO 引脚（注意 M2/M4 的 GPIO13/14、GPIO17/18 正反转顺序与 M1/M3 相反）
   - 摇杆值使用 9 档 `pwm_value[]` 速度等级表（`{0, 150, 160, 170, 190, 210, 220, 230, 240}`），不要直接 `map()` 到 0-255
   - 按键优先级高于摇杆：中断回调中先检测按键，有按键按下则直接 `return`，不执行摇杆逻辑
   - 左摇杆 = 前后+差速转向（`motor_change()`），右摇杆 = 原地旋转+横移（`motor_change1()`），两套独立算法
   - 详细代码模板见 `references/future_tech_box_v2_hardware.md` 的 PS2 手柄章节
16. **2.0 与 3.0 循迹传感器引脚不同**：2.0 用 GPIO2/GPIO1，3.0 用 GPIO3/GPIO4，生成循迹代码前必须确认目标版本

---

## 常见问题排查指南

### 问题 1：编译命令长时间卡住（看似无响应）

**症状**：执行 `pio run` 后超过 1 分钟无任何输出

**根因分析**：
| 可能原因 | 概率 | 验证方法 |
|----------|------|----------|
| Arduino 框架未下载 | ⭐⭐⭐ 高 | 运行 `check_environment.py` 检查 `framework.cached` |
| 网络连接问题 | ⭐⭐ 中 | 检查网络，尝试 ping github.com |
| 杀毒软件干扰 | ⭐ 低 | 检查 Windows Defender 活动 |

**解决方案**：
1. 先运行 `python scripts/check_environment.py` 检查框架状态
2. 如果 `framework.cached = false`，告知用户首次编译需要下载依赖
3. 预估时间：网络良好 5-10 分钟，网络一般 10-20 分钟

### 问题 2：编译错误 - ledcAttach 未定义

**症状**：
```
error: 'ledcAttach' was not declared in this scope
```

**根因**：Arduino ESP32 Core 版本 API 差异

| API | Arduino Core 2.x | Arduino Core 3.x |
|-----|------------------|------------------|
| PWM 配置 | `ledcSetup(channel, freq, resolution)` | `ledcAttach(pin, freq, resolution)` |
| 引脚绑定 | `ledcAttachPin(pin, channel)` | (合并到 ledcAttach) |
| 写入 PWM | `ledcWrite(channel, duty)` | `ledcWrite(pin, duty)` |

**解决方案**：

**PWM 控制（如电机调速）**：使用 2.x 兼容 API
```cpp
// 初始化（仅在 setup() 中调用一次）
ledcSetup(channel, PWM_FREQ, PWM_RESOLUTION);
ledcAttachPin(pin, channel);

// 写入
ledcWrite(channel, brightness);  // 注意：使用通道号，不是引脚号
```

**💡 蜂鸣器**：GPIO26 支持 `tone()`/`noTone()`，与 LED 矩阵共存时注意使用非阻塞方式（参见蜂鸣器控制要点）。

### 问题 3：烧录失败 - 端口不可用

**症状**：
```
Error: Unable to open port
Error: PermissionError(13, '拒绝访问')
```

**根因分析**：
| 可能原因 | 概率 | 说明 |
|----------|------|------|
| 串口被短暂占用 | ⭐⭐⭐ 高 | 上一次烧录后设备重启，串口短暂锁定 |
| 串口监视器占用 | ⭐⭐ 中 | Arduino IDE/PlatformIO Monitor 等打开着 |
| 驱动问题 | ⭐ 低 | USB 驱动异常 |

**自动处理策略**（SKILL 应实现）：
1. **自动重试**：等待 2-3 秒后重试 esptool 直调烧录，最多 3 次
2. **检测端口状态**：在烧录前运行 `detect_port_windows.py` 确认串口可用
3. **保持 esptool 直调**：主命令即 esptool 直调（见 3.1），失败时重试同一命令

**手动排查步骤**：
1. 运行串口检测脚本：`python scripts/detect_port_windows.py`
2. 检查 USB 连接是否稳固
3. 尝试：按住主板 BOOT 按钮 → 插入 USB → 松开 BOOT
4. 检查设备管理器是否显示 COM 端口

**烧录命令（与主命令一致，esptool 直调 + `--after no_reset`）**：
```bash
# esptool 直调烧录（--after no_reset，烧录后不自动运行程序）
python %USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py ^
  --chip esp32s3 --port COM6 --baud 921600 ^
  --before default_reset --after no_reset ^
  write_flash -z --flash_mode dio --flash_freq 80m --flash_size 8MB ^
  0x0 .pio\build\seeed_xiao_esp32s3\bootloader.bin ^
  0x8000 .pio\build\seeed_xiao_esp32s3\partitions.bin ^
  0x10000 .pio\build\seeed_xiao_esp32s3\firmware.bin
```

### 问题 4：电机控制相关问题

#### 4.1 电机速度设置

**⚠️ 默认速度建议设置为 70%（180），不要超过 85%（220）**

| 速度值 | 功率占比 | 说明 |
|--------|----------|------|
| 180 | 70% | ✅ **推荐默认值**，平衡动力和安全 |
| 200 | 78% | 较高速度，适合地面阻力大的场景 |
| 220 | 86% | ⚠️ 可能触发电池保护（过流） |
| 255 | 100% | ❌ 不建议，容易触发保护或损坏电机 |

**代码示例**：
```cpp
const int MOTOR_SPEED = 180;  // 默认速度 70%，避免触发电池保护
```

#### 4.2 按键检测在电机运动循环中无响应

**症状**：按下按键无法切换模式或停止电机

**根因**：使用 `delay()` 阻塞式等待，按键检测只在循环开头执行一次

**❌ 错误写法**：
```cpp
void loop() {
  // 按键只在这里检测一次
  if (digitalRead(KEY_A) == LOW) {
    // 切换模式
  }
  
  // 这里阻塞了 3 秒，期间按键无法响应！
  carForward(180);
  delay(1500);
  carStop();
  delay(300);
  carBackward(180);
  delay(1200);
}
```

**✅ 正确写法（非阻塞状态机）**：
```cpp
enum MoveState { STATE_FORWARD, STATE_STOP, STATE_BACKWARD };
MoveState currentState = STATE_FORWARD;
unsigned long stateStartTime = 0;

void loop() {
  // 1. 按键检测（每轮 loop 都执行）
  checkButton();
  
  // 2. 状态机处理（非阻塞）
  unsigned long elapsed = millis() - stateStartTime;
  
  switch (currentState) {
    case STATE_FORWARD:
      if (elapsed >= 1500) {
        carStop();
        currentState = STATE_STOP;
        stateStartTime = millis();
      }
      break;
    case STATE_STOP:
      if (elapsed >= 300) {
        carBackward(180);
        currentState = STATE_BACKWARD;
        stateStartTime = millis();
      }
      break;
    // ...
  }
  
  delay(10);  // 短暂延时，不阻塞按键检测
}
```

#### 4.3 电机方向切换时堵转

**症状**：快速切换方向时电机发出异响或卡顿

**根因**：电机未完全停止就开始反向运动

**解决方案**：方向切换前先停止电机并短暂延时

```cpp
void changeState(MoveState newState) {
  // 先停止当前运动
  carStop();
  delay(50);  // 短暂延时让电机完全停止
  
  // 切换到新状态
  currentState = newState;
  stateStartTime = millis();
  executeState(newState);
}
```

#### 4.4 麦克纳姆轮运动不正常

**可能原因**：
1. 轮子安装方向错误（俯视应组成 "X" 形）
2. 电机速度不一致
3. 电池电量不足

**调试建议**：
- 先测试单个电机正反转是否正常
- 检查四个电机速度是否一致
- 确保电池电量充足

### 问题 5：烧录显示成功但程序未生效（旧程序仍在运行）

**症状**：烧录显示 SUCCESS，但主板上仍然运行旧程序，新程序未生效

**根因分析**：

| 可能原因 | 概率 | 说明 |
|----------|------|------|
| **烧录后未重新上电** | ⭐⭐⭐ 高 | 主烧录命令使用 `--after no_reset`，烧录完成后程序**不会自动运行**，需重新开关主板电源后才运行新程序 |
| **未指定串口端口** | ⭐⭐ 中 | 自动检测在 USB-Serial/JTAG 模式下不可靠，可能选错端口或静默失败 |
| ESP32-S3 USB 重枚举延迟 | ⭐ 低 | 复位后 USB 重枚举，新程序 `setup()` 中 delay 太短，启动信息丢失 |

**解决方案**：

1. **烧录成功后必须重新上电**（no_reset 模式的正常行为）：
   - 重新开关主板电源开关，或拔插一次 USB 线
   - 重新上电后新程序才会开始运行
   - ⚠️ 不要误以为烧录失败！

2. **始终显式指定端口**（烧录前的关键操作）：
   ```bash
   # 先检查端口
   pio device list
   
   # 指定端口烧录（esptool 直调，见 3.1 节）
   ```

3. **代码中增加启动延迟**：
   ```cpp
   void setup() {
     Serial.begin(115200);
     delay(2000);  // ✅ 等待 2 秒让 USB 串口稳定（不要只等 1 秒）
     Serial.println("程序启动...");
   }
   ```

4. **添加 I2C 扫描辅助调试**：当使用 I2C 传感器时，在 `setup()` 中加入 I2C 扫描，便于判断传感器是否连接正确。

5. **添加无传感器降级模式**：如果传感器未检测到，不要进入死循环闪灯，而是切换到 LED 演示模式，这样至少能确认程序已成功烧录。

**预防措施（写入编码规范）**：

| 项目 | 旧写法 | 新写法 |
|------|--------|--------|
| 烧录命令 | `pio run -t upload`（会 hard_reset 自动运行） | esptool 直调 + `--after no_reset`（烧录后停下载模式） |
| 烧录成功后 | 自动运行 | 提示用户重新开关电源后再运行 |
| setup 延迟 | `delay(1000)` | `delay(2000)` |
| 传感器初始化失败 | `while(1) { 闪灯 }` | 切换到演示/降级模式 |
| I2C 传感器 | 直接初始化 | 先 I2C 扫描，再初始化 |

### 问题 6：CodeBuddy 命令执行超时

**症状**：命令被跳过，显示 "may take a long time"

**根因**：CodeBuddy 有命令执行超时保护（约 60-120 秒）

**这不是 BUG，是正常保护机制。**

**解决方案**：
1. 确保依赖已下载（`framework.cached = true`）
2. 依赖就绪后，编译仅需 8-10 秒，不会触发超时
3. 如果首次使用，引导用户在终端手动执行一次 `pio run` 完成依赖下载

### 问题 7：程序烧录后串口无输出

**症状**：烧录成功后使用串口监视器看不到任何输出

**根因分析**：

| 可能原因 | 概率 | 说明 |
|----------|------|------|
| **烧录后未重新上电** | ⭐⭐⭐ 高 | no_reset 模式下芯片停在下载模式，不运行程序，自然没有串口输出；必须重新开关电源后程序才运行 |
| `setup()` 中 delay 太短 | ⭐⭐ 中 | USB-Serial/JTAG 模式重枚举需要时间，1 秒可能不够 |
| 串口波特率不匹配 | ⭐⭐ 中 | 代码中 115200 但监视器用了其他波特率 |
| USB 重枚举后端口号变化 | ⭐ 低 | 复位后 COM 端口可能改变 |

**解决方案**：
1. **先确认已重新上电**：烧录完成后芯片停在下载模式，串口不会有输出。重新开关主板电源（或拔插 USB）后程序运行，再打开串口监视器
2. `setup()` 开头使用 `delay(2000)` 而不是 `delay(1000)`
3. 按 RST 复位按钮重新触发启动日志
4. 确认监视器波特率与代码一致（115200）

### 问题 8：`pio.exe` 被 Windows 应用控制策略阻止

**症状**：
```
程序"pio.exe"无法运行: An Application Control policy has blocked this file
CategoryInfo: ResourceUnavailable
FullyQualifiedErrorId: NativeCommandFailed
```

**根因分析**：

| 可能原因 | 概率 | 说明 |
|----------|------|------|
| Windows AppLocker/WDAC 策略 | ⭐⭐⭐ 高 | 企业/学校电脑常配置应用白名单策略，Python Scripts 目录下的 .exe 不在白名单中 |
| Windows Defender 应用控制 | ⭐⭐ 中 | SmartScreen 或 Defender 策略阻止未签名的可执行文件 |
| 杀毒软件拦截 | ⭐ 低 | 第三方安全软件拦截 |

**解决方案（关键！已验证有效）**：

使用 `python -m platformio` 代替 `pio` 命令。Python 解释器本身通常在白名单中，通过模块调用方式可以绕过对 `pio.exe` 的阻止：

```bash
# ❌ 被阻止的写法
pio run
pio run -t upload --upload-port COM10
pio device list

# ✅ 正确写法（用 python -m platformio 代替 pio）
python -m platformio run
python -m platformio device list
# ⚠️ 烧录不再用 pio upload（会 hard_reset 自动运行），改用 esptool 直调（见 3.1）
python %USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py \
  --chip esp32s3 --port COM10 --baud 921600 \
  --before default_reset --after no_reset write_flash ...
```

**⚠️ SKILL 执行时的强制规则**：
1. **所有 PlatformIO 命令一律使用 `python -m platformio` 前缀**，不要直接使用 `pio`
2. 如果 `python -m platformio` 也失败，再尝试用完整路径：`python "C:\Users\用户名\AppData\Local\Programs\Python\Python311\Scripts\pio.exe"`
3. 编译命令：`python -m platformio run -d <项目路径>`
4. **烧录命令：esptool 直调 + `--after no_reset`**（禁止 `python -m platformio run -t upload`，会自动运行程序；完整命令见 3.1）
5. 设备列表：`python -m platformio device list`
6. 串口监视器：`python -m platformio device monitor`

**预防措施**：在 Phase 0 环境扫描时，应检测 `pio` 命令是否可用，如果不可用则自动切换到 `python -m platformio` 方式。

### 问题 9：未来科技盒 3.0 与 2.0 的硬件差异导致代码不兼容

**症状**：为 2.0 编写的代码烧录到 3.0 上无法正常工作，或反之

**根因**：3.0 和 2.0 在关键硬件上有本质差异

**未来科技盒 3.0 vs 2.0 硬件对比**：

| 硬件特征 | 2.0 版本 | 3.0 版本 |
|----------|----------|----------|
| **主控芯片** | XIAO ESP32S3 | ESP32-S3 (QFN56) |
| **USB 转串口** | USB-Serial/JTAG (内置) | CH343 外置芯片 |
| **VID:PID** | 303A:1001 | 1A86:55D3 |
| **LED 类型** | 3×3 行列扫描矩阵 (GPIO33-38) | 9×WS2812 RGB LED (GPIO33 单总线) |
| **LED 控制方式** | 行HIGH+列LOW 点亮，需持续扫描 | Adafruit NeoPixel 库，单总线协议 |
| **LED 颜色** | 单色（红色） | 全彩 RGB（1600万色） |

**⚠️ 关键差异说明**：

1. **LED 控制完全不同**：
   - 2.0 使用 `scanDisplay()` 行列扫描，`loop()` 中不能有阻塞延时
   - 3.0 使用 `Adafruit_NeoPixel` 库的 `strip.setPixelColor()` + `strip.show()`，可以使用 `delay()`

2. **USB 串口芯片不同（关键差异！）**：
   - 2.0 使用 ESP32-S3 内置 USB-Serial/JTAG（VID:PID = `303A:1001`），烧录后会 USB 重枚举，端口号可能变化
   - 3.0 使用 **CH343 外置 USB 转串口芯片**（VID:PID = `1A86:55D3`），端口更稳定，烧录后端口号通常不变
   - CH343 是沁恒（WCH）公司的高速 USB 转串口芯片，支持最高 6Mbps 波特率
   - **Windows 驱动**：CH343 需要安装 WCH 的驱动程序（CH343SER），如果设备管理器中显示为未识别设备或黄色感叹号，需要手动安装驱动
   - **串口识别**：在 `pio device list` 中，3.0 设备显示为 `USB-Enhanced-SERIAL CH343` 而非 `USB-Serial/JTAG`
   - **platformio.ini 配置**：3.0 必须设置 `ARDUINO_USB_CDC_ON_BOOT=0`，否则 `Serial.print` 输出会走 ESP32 内置 USB 而非 CH343，串口监视器将看不到任何输出

3. **烧录可靠性**：
   - 3.0 因使用外置 CH343 芯片，PlatformIO 自动端口检测更可靠（不会因 USB 重枚举导致端口号变化）
   - 但仍**强烈建议**显式指定 `--upload-port` 避免意外
   - 3.0 烧录速度通常为 460800 bps（CH343 默认），比 2.0 的 USB-Serial/JTAG 模式更稳定

**3.0 版本 RGB LED 代码模板**：
```cpp
#include <Adafruit_NeoPixel.h>

#define RGB_PIN     33
#define NUM_LEDS    9
#define BRIGHTNESS  30    // 建议 20-50，避免 LED 过亮刺眼

Adafruit_NeoPixel strip(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();
}

void loop() {
  // 设置第 i 个 LED 的颜色 (R, G, B)
  strip.setPixelColor(0, strip.Color(255, 0, 0));    // 红色
  strip.setPixelColor(1, strip.Color(0, 255, 0));    // 绿色
  strip.setPixelColor(2, strip.Color(0, 0, 255));    // 蓝色
  strip.show();  // 刷新显示
  
  // HSV 颜色（更方便生成彩虹效果）
  uint32_t color = strip.gamma32(strip.ColorHSV(hue, 255, 255));
  strip.setPixelColor(i, color);
}
```

**3.0 版本 platformio.ini 模板**：
```ini
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200

lib_deps = 
    adafruit/Adafruit NeoPixel

; ⚠️ CH343 USB 转串口配置（3.0 必须）
; 3.0 使用外置 CH343 芯片做 USB 转串口，不是 ESP32-S3 内置 USB
; 必须关闭 USB CDC on Boot，否则串口输出会走内置 USB 而非 CH343
build_flags = 
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=0
```

> ⚠️ **3.0 platformio.ini 必须同时满足两个条件**：
> 1. `board = esp32-s3-devkitc-1`（不能用 `seeed_xiao_esp32s3`）
> 2. `build_flags` 中有 `-DARDUINO_USB_CDC_ON_BOOT=0`
> 缺少任一条件都会导致串口无输出或编译失败。

> ⚠️ **3.0 与 2.0 的 board 和串口配置差异**：
> - 2.0 使用 `board = seeed_xiao_esp32s3`，串口走 ESP32-S3 内置 USB-Serial/JTAG
> - 3.0 使用 `board = esp32-s3-devkitc-1`，串口走外置 CH343 芯片，必须设置 `ARDUINO_USB_CDC_ON_BOOT=0`
> - 如果 3.0 不设置 `ARDUINO_USB_CDC_ON_BOOT=0`，Serial.print 的输出会走内置 USB 而非 CH343，导致串口监视器看不到任何输出

**RGB 亮度建议**：

| 亮度值 | 效果 | 适用场景 |
|--------|------|----------|
| 20-30 | 柔和护眼 | ✅ **推荐日常使用** |
| 40-60 | 中等亮度 | 展示演示 |
| 80-100 | 较亮 | ⚠️ 户外或光线强的环境 |
| >100 | 刺眼 | ❌ 不建议，LED 发热大且电流高 |

### 问题 10：3.0 版本烧录试错经验记录

**以下是在 3.0 版本实际烧录过程中遇到的错误和解决方案，供后续参考：**

#### 10.1 board 配置错误

**症状**：编译失败或烧录后串口无输出

**错误写法**：
```ini
board = seeed_xiao_esp32s3    ; ❌ 这是 2.0 的板型
```

**正确写法**：
```ini
board = esp32-s3-devkitc-1    ; ✅ 3.0 使用通用 ESP32-S3 板型
```

**原因**：3.0 使用的是 ESP32-S3 QFN56 封装 + CH343 外置串口芯片，与 XIAO ESP32S3 开发板的硬件架构不同，不能使用 `seeed_xiao_esp32s3` 板型配置。

#### 10.2 USB CDC 配置缺失导致串口无输出

**症状**：程序烧录成功，LED 正常工作，但串口监视器看不到任何 `Serial.print` 输出

**根因**：3.0 使用 CH343 做 USB 转串口，但 ESP32-S3 默认可能会将 Serial 输出路由到内置 USB CDC。如果不显式关闭 USB CDC on Boot，串口输出可能走错通道。

**解决方案**：在 `platformio.ini` 的 `build_flags` 中添加：
```ini
build_flags = 
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=0
```

#### 10.3 CH343 驱动未安装

**症状**：USB 连接后设备管理器中显示未知设备或黄色感叹号，`pio device list` 找不到串口

**解决方案**：
1. 前往 WCH 官网下载 CH343 驱动：https://www.wch.cn/downloads/CH343SER_EXE.html
2. 安装驱动后重新插入 USB，设备管理器应显示 `USB-Enhanced-SERIAL CH343 (COMx)`
3. 在 `pio device list` 中应能看到 VID:PID = `1A86:55D3` 的设备

#### 10.4 烧录端口自动检测失败

**症状**：烧录报错找不到端口或选错了端口

**经验**：虽然 CH343 比内置 USB-Serial/JTAG 更稳定，但在电脑上同时连接了多个 USB 设备时，自动检测仍可能出错。

**强制规则**：3.0 烧录时同样必须显式指定端口，并使用 esptool 直调 + `--after no_reset`：
```bash
# 先查看端口
python -m platformio device list

# 找到 CH343 设备的 COM 端口号，然后 esptool 直调烧录（no_reset，不自动运行）
python %USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py \
  --chip esp32s3 --port COMx --baud 460800 \
  --before default_reset --after no_reset write_flash ...
```

### 问题 11：NeoPixel RGB LED 亮度切换时闪烁（3.0 特有）

**症状**：按键调节亮度时，LED 先整体闪烁一下白光/全亮，然后才恢复到正确的亮度和颜色

**根因分析**：

| 可能原因 | 概率 | 说明 |
|----------|------|------|
| **代码中加了闪白反馈效果** | ⭐⭐⭐ 高 | `brightnessChangeFeedback()` 函数在设置新亮度后先闪白色，造成视觉闪烁 |
| `setBrightness()` 后未立即刷新显示 | ⭐⭐ 中 | 调用 `setBrightness()` 后没有立即用当前颜色数据调用 `show()`，下次 `show()` 时亮度突变 |

**⚠️ NeoPixel `setBrightness()` 的工作原理（重要！）**：

`strip.setBrightness()` 不会立即改变 LED 的显示效果，它只是设置了一个全局亮度缩放因子。**下一次调用 `strip.show()` 时才会应用新的亮度值**。因此：

1. 如果在 `setBrightness()` 之后插入了"闪白"或其他颜色的效果，用户会先看到一次高亮度的全白闪烁
2. 正确做法是 `setBrightness()` 后立即用当前颜色数据调用 `show()`，实现平滑过渡

**❌ 错误写法**（会导致闪烁）：
```cpp
void onKeyPressed() {
  currentBrightness += step;
  strip.setBrightness(currentBrightness);
  
  // ❌ 闪白反馈——这会造成视觉上的突然闪烁
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
  }
  strip.show();
  delay(50);
}
```

**✅ 正确写法**（平滑亮度过渡）：
```cpp
void onKeyPressed() {
  currentBrightness += step;
  strip.setBrightness(currentBrightness);
  
  // ✅ 直接用当前颜色重新刷新，无闪烁
  for (int i = 0; i < NUM_LEDS; i++) {
    int pixelHue = rainbowHue + (i * 65536L / NUM_LEDS);
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
  }
  strip.show();  // 立即以新亮度显示当前颜色
}
```

**生成 NeoPixel 亮度调节代码时的强制规则**：
1. **禁止在亮度切换时加入闪白/闪色反馈效果**
2. 调用 `setBrightness()` 后必须立即用当前颜色数据调用 `show()`
3. 如需按键反馈，可通过串口输出或短暂改变颜色色相（而非白色闪烁）来实现

### 问题 12：2.0 与 3.0 循迹传感器引脚混淆

**症状**：循迹小车程序在不同版本主板上工作异常，循迹传感器无反应或反应颠倒

**根因**：2.0 和 3.0 的循迹传感器使用不同的 GPIO 引脚

| 版本 | 左循迹 GPIO | 右循迹 GPIO | 接口 |
|------|------------|------------|------|
| **2.0** | GPIO2 | GPIO1 | 接口3 |
| **3.0** | GPIO3 | GPIO4 | 接口3 |

**解决方案**：
1. 生成循迹代码前**必须确认目标硬件版本**
2. 在 `pins.h` 中使用宏定义，方便切换：
```cpp
// === 2.0 版本 ===
#define TRACK_LEFT  2
#define TRACK_RIGHT 1

// === 3.0 版本 ===
// #define TRACK_LEFT  3
// #define TRACK_RIGHT 4
```

**预防措施**：当用户提到"循迹"但未指定版本时，必须询问确认是 2.0 还是 3.0。

---

## 问题预判检查清单

在执行编程任务前，运行以下检查：

```
✅ 检查项                        命令/方法                           通过条件
──────────────────────────────────────────────────────────────────────────────
[ ] Python 版本                  python --version                   >= 3.8
[ ] PlatformIO CLI               python -m platformio --version     >= 6.0
[ ] pio.exe 可用性               pio --version                      如被阻止→用 python -m platformio
[ ] 串口连接                      detect_port_windows.py             status=ok
[ ] ESP32 平台                   python -m platformio pkg list -g   有 espressif32
[ ] Arduino 框架（关键）          check_environment.py               framework.cached=true
[ ] 工具链                        check_environment.py               toolchain.cached=true
```

> ⚠️ **重要**：如果 `pio` 命令被系统策略阻止（见问题 8），所有后续步骤必须使用
> `python -m platformio` 代替 `pio`。这在企业/学校管理的电脑上非常常见。

**如果 Arduino 框架未缓存，必须提前告知用户**：
- 首次编译将下载 50-100MB 依赖
- 预计耗时 5-20 分钟
- 下载期间命令可能无响应（正常现象）

---

## 连续烧录场景处理（重要）

当用户在同一会话中多次烧录不同程序时，采用「**先尝试，失败后再确认**」策略。

### 场景识别

以下情况属于"连续烧录场景"：
1. 用户刚完成一个程序的烧录，又提出新的编程需求
2. 用户要求"再跑一个 case" 或 "换个功能试试"
3. 主板上当前正在运行之前烧录的程序

### 处理策略

**✅ 正确做法**：直接开始编译烧录，过程中提醒用户，失败后再让用户确认

**❌ 错误做法**：~~每次烧录前都弹出确认框让用户点击后才继续~~

### 自动处理流程

```
用户提出新需求
       ↓
直接生成代码、编译
       ↓
尝试烧录（过程中提示：如遇失败可能需按 RST）
       ↓
   ┌───┴───┐
   ↓       ↓
 成功    失败
   ↓       ↓
 完成   自动等待 3 秒，重试（最多 2 次）
           ↓
       ┌───┴───┐
       ↓       ↓
     成功    仍失败
       ↓       ↓
     完成   此时向用户显示确认框
              要求按 RST 后回复"继续"
```

### 烧录提示（融入过程中，无需等待确认）

在烧录开始时的提示信息中包含：
```
💡 如遇烧录失败，可能需要按一下 RST 复位按钮后重试
```

### 代码实现参考

在烧录函数中加入自动重试逻辑：

```python
def upload_with_retry(project_path, port, max_retries=2):
    """带自动重试的烧录函数 - 无需用户提前确认"""
    for attempt in range(max_retries + 1):
        if attempt > 0:
            print(f"⏳ 烧录遇到问题，自动重试中 (第 {attempt} 次)...")
            time.sleep(3)  # 等待 3 秒后重试
        
        # 尝试烧录
        result = run_upload_command(project_path)
        if result.success:
            return {"success": True, "attempts": attempt + 1}
        
        # 如果是权限错误，继续重试
        if "PermissionError" in result.error or "拒绝访问" in result.error:
            continue
        
        # 其他错误也继续重试
        continue
    
    # 所有自动重试都失败，此时才需要用户介入
    return {"success": False, "need_user_action": True, "error": result.error}
```

---

## 🌐 WiFi Web 遥控方案（网页控制小车）

当用户需求涉及**通过电脑/手机网页控制小车**时，使用本方案。ESP32-S3 自带 WiFi 802.11 b/g/n，可直接作为 Web Server 提供控制页面。

### 方案选择流程

```
用户提出网页控制需求
       ↓
╔══════════════════════════════════════════════════════════════════╗
║  📶 WiFi Web 遥控方案说明                                         ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                  ║
║  本方案需要电脑与小车在同一个网络下。                              ║
║                                                                  ║
║  🅰️ 【推荐】STA 模式（小车连接路由器 WiFi）                      ║
║     小车连接到与电脑相同的 WiFi 网络，电脑浏览器打开小车 IP       ║
║     即可控制。                                                    ║
║     👉 请提供当前网络的 WiFi 名称和密码                           ║
║                                                                  ║
║  🅱️ AP 模式（小车创建热点，无需路由器）                          ║
║     如果没有可用的 WiFi 网络，小车会自动创建热点，电脑连接        ║
║     该热点后打开 192.168.4.1 即可控制。                           ║
║     ⚠️  电脑连接热点后将无法上网                                  ║
║                                                                  ║
╚══════════════════════════════════════════════════════════════════╝
```

**优先使用 STA 模式**，用户无法提供 WiFi 信息时才使用 AP 模式。

### 方案 A：STA 模式（小车连接路由器 WiFi）⭐ 推荐

```
┌──────────────┐       WiFi（同一局域网）       ┌───────────────┐
│  电脑浏览器   │ ←────── WebSocket ──────────→ │  ESP32-S3 小车  │
│  (控制页面)   │     低延迟 ~10-50ms           │  (Web Server)  │
└──────────────┘                                └───────────────┘
        ↑                    ↑                          ↑
        └────────── 同一个 WiFi 路由器 ─────────────────┘
```

**工作原理**：
1. ESP32 连接用户指定的 WiFi 路由器
2. 启动 Web Server + WebSocket 服务
3. 电脑浏览器打开 ESP32 的 IP 地址，显示控制页面
4. 通过 WebSocket 实现实时低延迟双向通信

**优点**：
- ✅ 电脑正常上网，小车同时可控
- ✅ 手机也能同时控制（多设备支持）
- ✅ 延迟低（10-50ms）

**用户提示**：需要用户提供 WiFi 名称（SSID）和密码

### 方案 B：AP 模式（无需路由器）

```
┌──────────────┐    连接 ESP32 创建的热点     ┌───────────────┐
│  电脑/手机    │ ←────── WebSocket ─────────→ │  ESP32-S3 小车  │
│              │      192.168.4.1             │  (AP + Server) │
└──────────────┘                               └───────────────┘
```

**工作原理**：
1. ESP32 创建一个 WiFi 热点（如 `FutureCar_XXXX`）
2. 电脑/手机连接该热点
3. 浏览器打开 `192.168.4.1`，显示控制页面

**优点**：
- ✅ 无需路由器，户外也能用
- ✅ 连接更稳定（直连）

**缺点**：
- ❌ 电脑连接热点后断网

### PlatformIO 库依赖配置

```ini
[env:seeed_xiao_esp32s3]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
monitor_speed = 115200

lib_deps = 
    ESP Async WebServer
    AsyncTCP
```

### WiFi Web 遥控代码模板

#### STA 模式完整代码

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ==================== WiFi 配置 ====================
// ⚠️ 需要用户提供，替换为实际的 WiFi 名称和密码
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ==================== 电机引脚定义 ====================
#define M1_FWD 11
#define M1_REV 12
#define M2_FWD 14
#define M2_REV 13
#define M3_FWD 15
#define M3_REV 16
#define M4_FWD 18
#define M4_REV 17

// ==================== Web Server ====================
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ==================== 电机控制函数 ====================
void setMotor(int fwdPin, int revPin, int speed) {
  speed = constrain(speed, -255, 255);
  if (speed > 0) {
    analogWrite(fwdPin, speed);
    analogWrite(revPin, 0);
  } else if (speed < 0) {
    analogWrite(fwdPin, 0);
    analogWrite(revPin, -speed);
  } else {
    analogWrite(fwdPin, 0);
    analogWrite(revPin, 0);
  }
}

void setMotorsSeparate(int m1, int m2, int m3, int m4) {
  setMotor(M1_FWD, M1_REV, m1);
  setMotor(M2_FWD, M2_REV, m2);
  setMotor(M3_FWD, M3_REV, m3);
  setMotor(M4_FWD, M4_REV, m4);
}

void carStop()                    { setMotorsSeparate(0, 0, 0, 0); }
void carForward(int s = 180)     { setMotorsSeparate(s, s, s, s); }
void carBackward(int s = 180)    { setMotorsSeparate(-s, -s, -s, -s); }
void carTurnLeft(int s = 180)    { setMotorsSeparate(-s, s, -s, s); }
void carTurnRight(int s = 180)   { setMotorsSeparate(s, -s, s, -s); }
void carMoveLeft(int s = 180)    { setMotorsSeparate(-s, s, s, -s); }
void carMoveRight(int s = 180)   { setMotorsSeparate(s, -s, -s, s); }

// ==================== 当前速度 ====================
int currentSpeed = 180;

// ==================== 处理 WebSocket 消息 ====================
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String msg = (char*)data;
    
    if (msg == "F")       carForward(currentSpeed);
    else if (msg == "B")  carBackward(currentSpeed);
    else if (msg == "L")  carTurnLeft(currentSpeed);
    else if (msg == "R")  carTurnRight(currentSpeed);
    else if (msg == "ML") carMoveLeft(currentSpeed);
    else if (msg == "MR") carMoveRight(currentSpeed);
    else if (msg == "S")  carStop();
    else if (msg.startsWith("SPD:")) {
      currentSpeed = msg.substring(4).toInt();
      currentSpeed = constrain(currentSpeed, 0, 220);
    }
  }
}

// ==================== WebSocket 事件 ====================
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket 客户端 #%u 已连接\n", client->id());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket 客户端 #%u 已断开\n", client->id());
      carStop();  // 断开时自动停车
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// ==================== 控制页面 HTML ====================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>小车遥控器</title>
<style>
  * { margin:0; padding:0; box-sizing:border-box; touch-action:manipulation; }
  body { font-family:-apple-system,sans-serif; background:#1a1a2e; color:#fff;
         display:flex; flex-direction:column; align-items:center;
         min-height:100vh; padding:20px; user-select:none; }
  h1 { font-size:24px; margin-bottom:10px; }
  .status { font-size:14px; margin-bottom:20px; padding:6px 16px;
            border-radius:20px; background:#16213e; }
  .status.connected { color:#0f0; }
  .status.disconnected { color:#f44; }
  .controls { display:grid; grid-template-columns:repeat(3,80px);
              grid-template-rows:repeat(3,80px); gap:10px; margin-bottom:20px; }
  .btn { width:80px; height:80px; border:none; border-radius:16px; font-size:28px;
         background:#16213e; color:#fff; cursor:pointer; display:flex;
         align-items:center; justify-content:center; transition:all .1s; }
  .btn:active, .btn.active { background:#e94560; transform:scale(0.95); }
  .btn.empty { visibility:hidden; }
  .side-controls { display:flex; gap:20px; margin-bottom:20px; }
  .side-btn { width:100px; height:60px; border:none; border-radius:12px;
              font-size:16px; background:#16213e; color:#fff; cursor:pointer; }
  .side-btn:active, .side-btn.active { background:#0f3460; transform:scale(0.95); }
  .speed-control { width:280px; text-align:center; margin-bottom:10px; }
  .speed-control input { width:100%; }
  .speed-label { font-size:14px; color:#aaa; }
  .keyboard-hint { font-size:12px; color:#666; margin-top:15px; text-align:center; }
</style>
</head>
<body>
<h1>🚗 小车遥控器</h1>
<div class="status disconnected" id="status">⏳ 连接中...</div>

<div class="controls">
  <div class="btn empty"></div>
  <div class="btn" id="btnF" data-cmd="F">↑</div>
  <div class="btn empty"></div>
  <div class="btn" id="btnL" data-cmd="L">↺</div>
  <div class="btn" id="btnS" data-cmd="S" style="background:#e94560;">⏹</div>
  <div class="btn" id="btnR" data-cmd="R">↻</div>
  <div class="btn empty"></div>
  <div class="btn" id="btnB" data-cmd="B">↓</div>
  <div class="btn empty"></div>
</div>

<div class="side-controls">
  <div class="side-btn" id="btnML" data-cmd="ML">◀ 左横移</div>
  <div class="side-btn" id="btnMR" data-cmd="MR">右横移 ▶</div>
</div>

<div class="speed-control">
  <div class="speed-label">速度: <span id="speedVal">180</span> / 220</div>
  <input type="range" id="speedSlider" min="80" max="220" value="180" step="10">
</div>

<div class="keyboard-hint">
  💡 键盘: W/A/S/D=方向 Q/E=横移 Space=停止
</div>

<script>
let ws;
let connected = false;
const statusEl = document.getElementById('status');

function connect() {
  ws = new WebSocket('ws://' + location.host + '/ws');
  ws.onopen = () => {
    connected = true;
    statusEl.textContent = '✅ 已连接';
    statusEl.className = 'status connected';
  };
  ws.onclose = () => {
    connected = false;
    statusEl.textContent = '❌ 已断开，重连中...';
    statusEl.className = 'status disconnected';
    setTimeout(connect, 2000);
  };
  ws.onerror = () => { ws.close(); };
}
connect();

function send(cmd) {
  if (connected && ws.readyState === WebSocket.OPEN) ws.send(cmd);
}

// 触摸/鼠标控制 - 按下发送指令，松开发送停止
document.querySelectorAll('.btn[data-cmd], .side-btn[data-cmd]').forEach(btn => {
  const cmd = btn.dataset.cmd;
  
  function onDown(e) {
    e.preventDefault();
    btn.classList.add('active');
    send(cmd);
  }
  function onUp(e) {
    e.preventDefault();
    btn.classList.remove('active');
    if (cmd !== 'S') send('S');
  }
  
  btn.addEventListener('mousedown', onDown);
  btn.addEventListener('mouseup', onUp);
  btn.addEventListener('mouseleave', onUp);
  btn.addEventListener('touchstart', onDown, {passive:false});
  btn.addEventListener('touchend', onUp, {passive:false});
});

// 速度滑块
const slider = document.getElementById('speedSlider');
const speedVal = document.getElementById('speedVal');
slider.addEventListener('input', () => {
  speedVal.textContent = slider.value;
  send('SPD:' + slider.value);
});

// 键盘控制
const keyMap = {
  'w':'F','W':'F', 'ArrowUp':'F',
  's':'B','S':'B', 'ArrowDown':'B',
  'a':'L','A':'L', 'ArrowLeft':'L',
  'd':'R','D':'R', 'ArrowRight':'R',
  'q':'ML','Q':'ML',
  'e':'MR','E':'MR',
  ' ':'S'
};
const btnMap = {'F':'btnF','B':'btnB','L':'btnL','R':'btnR','ML':'btnML','MR':'btnMR','S':'btnS'};
const activeKeys = new Set();

document.addEventListener('keydown', (e) => {
  if (e.repeat) return;
  const cmd = keyMap[e.key];
  if (cmd) {
    e.preventDefault();
    activeKeys.add(e.key);
    send(cmd);
    const b = document.getElementById(btnMap[cmd]);
    if (b) b.classList.add('active');
  }
});
document.addEventListener('keyup', (e) => {
  const cmd = keyMap[e.key];
  if (cmd) {
    activeKeys.delete(e.key);
    const b = document.getElementById(btnMap[cmd]);
    if (b) b.classList.remove('active');
    if (activeKeys.size === 0 && cmd !== 'S') send('S');
  }
});
</script>
</body>
</html>
)rawliteral";

// ==================== setup ====================
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // 初始化电机引脚
  pinMode(M1_FWD, OUTPUT); pinMode(M1_REV, OUTPUT);
  pinMode(M2_FWD, OUTPUT); pinMode(M2_REV, OUTPUT);
  pinMode(M3_FWD, OUTPUT); pinMode(M3_REV, OUTPUT);
  pinMode(M4_FWD, OUTPUT); pinMode(M4_REV, OUTPUT);
  analogWriteFrequency(10000);
  carStop();
  
  // 连接 WiFi
  Serial.println("正在连接 WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 30) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi 已连接!");
    Serial.print("📡 IP 地址: ");
    Serial.println(WiFi.localIP());
    Serial.println("在浏览器中打开上面的 IP 地址即可控制小车");
  } else {
    Serial.println("\n❌ WiFi 连接失败，切换到 AP 模式...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("FutureCar_Control", "12345678");
    Serial.print("📡 热点 IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("请连接 WiFi: FutureCar_Control (密码: 12345678)");
    Serial.println("然后在浏览器打开 192.168.4.1");
  }
  
  // 配置 WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  // 配置 Web 页面
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  server.begin();
  Serial.println("🚀 Web Server 已启动!");
}

// ==================== loop ====================
void loop() {
  ws.cleanupClients();
  delay(10);
}
```

#### AP 模式代码差异

如果用户没有 WiFi，只需修改 `setup()` 中的 WiFi 连接部分：

```cpp
void setup() {
  // ... 电机初始化同上 ...
  
  // AP 模式：小车创建热点
  WiFi.mode(WIFI_AP);
  WiFi.softAP("FutureCar_Control", "12345678");
  
  Serial.println("📡 热点已创建!");
  Serial.println("WiFi 名称: FutureCar_Control");
  Serial.println("WiFi 密码: 12345678");
  Serial.print("控制地址: http://");
  Serial.println(WiFi.softAPIP());  // 192.168.4.1
  
  // ... WebSocket 和 Web Server 配置同上 ...
}
```

### WiFi Web 遥控 - 用户交互流程

生成 WiFi Web 遥控代码时，必须按以下流程与用户交互：

```
Step 1: 显示方案说明提示框（见上方"方案选择流程"）
       ↓
Step 2: 用户选择方案
       ├─ 提供了 WiFi 名称和密码 → 使用 STA 模式（方案 A）
       └─ 没有 WiFi / 不想提供  → 使用 AP 模式（方案 B）
       ↓
Step 3: 生成代码，替换 WiFi 凭据
       ↓
Step 4: 编译烧录（按常规 Phase 2/3 流程）
       ↓
Step 5: 烧录成功后提示用户操作：
```

**烧录成功后的提示（STA 模式）**：
```
🎉 烧录成功！WiFi 遥控小车已就绪！
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
操作步骤：
0. ⚠️ 程序不会自动运行，请先重新开关主板电源（或拔插 USB）
1. 等待 5 秒，小车自动连接 WiFi
2. 打开串口监视器查看小车获取到的 IP 地址
3. 在电脑浏览器中打开该 IP 地址（如 http://192.168.x.x）
4. 使用页面按钮或键盘 WASD 控制小车！

💡 键盘快捷键：
   W=前进  S=后退  A=左转  D=右转
   Q=左横移  E=右横移  空格=停止
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**烧录成功后的提示（AP 模式）**：
```
🎉 烧录成功！WiFi 遥控小车已就绪！
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
操作步骤：
0. ⚠️ 程序不会自动运行，请先重新开关主板电源（或拔插 USB）
1. 在电脑/手机的 WiFi 列表中找到 "FutureCar_Control"
2. 连接该热点，密码: 12345678
3. 在浏览器中打开 http://192.168.4.1
4. 使用页面按钮或键盘 WASD 控制小车！

⚠️ 连接小车热点后电脑将无法上网，控制完成后请切回原 WiFi。
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### WebSocket 通信协议

| 消息 | 方向 | 说明 |
|------|------|------|
| `F` | 客户端→ESP32 | 前进 |
| `B` | 客户端→ESP32 | 后退 |
| `L` | 客户端→ESP32 | 左转 |
| `R` | 客户端→ESP32 | 右转 |
| `ML` | 客户端→ESP32 | 左横移（麦克纳姆轮） |
| `MR` | 客户端→ESP32 | 右横移（麦克纳姆轮） |
| `S` | 客户端→ESP32 | 停止 |
| `SPD:xxx` | 客户端→ESP32 | 设置速度（0-220） |

### ⚠️ WiFi Web 遥控注意事项

1. **WiFi 连接超时处理**：STA 模式下如果 15 秒内连不上 WiFi，自动回退到 AP 模式
2. **断连自动停车**：WebSocket 断开时必须调用 `carStop()`，防止小车失控
3. **速度限制**：Web 端最大速度限制 220（86%），避免触发电池保护
4. **多客户端**：WebSocket 支持多个客户端同时连接，但可能产生指令冲突
5. **内存占用**：HTML 页面使用 `PROGMEM` 存储在 Flash 中，不占用 RAM
6. **安全性**：局域网内使用，无需额外加密；AP 模式设置了密码

---

## 🔌 USB 串口→网页控制方案（Web Serial API）

当用户需求涉及**主板通过 USB 连电脑，网页直接控制/接收主板数据**时，使用本方案。适合场景：
- 主板当遥控器（按键/传感器 → 控制网页效果）
- 主板按钮直接控制网页
- PS2 手柄 → 主板 → 串口 → 网页游戏/动画

### 与 WiFi Web 遥控的区别（先判断再选方案）

| 判断点 | 用 USB 串口→网页（Web Serial） | 用 WiFi Web 遥控 |
|--------|-------------------------------|------------------|
| 连接方式 | 主板 USB 线连电脑 | 主板开热点/连路由 |
| 是否需要 WiFi | ❌ 不需要 | ✅ 需要 |
| 对电脑网络影响 | 无 | AP 模式会占用/断网 |
| 浏览器 | 仅 Chrome/Edge | 任何浏览器 |
| 场景 | 近距离、单机、教学演示 | 远程、无线、移动控制 |

> 用户说"网页控制主板"但有 USB 线连接 → 优先 Web Serial（更简单可靠）；
> 用户说"手机远程控制""无线控制小车" → 用 WiFi Web 遥控。

### 处理流程
1. 提示用户：需要 **Chrome/Edge 浏览器**，通过 **localhost 或 HTTPS** 打开网页（file:// 可能受限）
2. 生成主板固件代码（`Serial.println` 按行发指令）
3. 生成网页 HTML 文件（Web Serial API，含连接/断开/收发/日志）
4. 提示用户：本地起服务 `python -m http.server 8000`，浏览器打开 `http://localhost:8000`
5. 编译烧录主板固件（按常规 Phase 2/3 流程）

### 前置条件提醒（生成代码时必须告知用户）
- 浏览器：Chrome / Edge（Firefox/Safari 不支持）
- 打开方式：localhost 或 HTTPS（secure context）
- USB 数据线（充电线无法通讯）
- 波特率一致：固件 `Serial.begin(115200)` = 网页 `port.open({baudRate: 115200})`
- 串口独占：关闭串口监视器等占用程序
- **2.0 无需装驱动**（内置 USB-Serial/JTAG）；`setup()` 中 `delay() ≥ 2000ms`
- **⚠️ 连接触发规范（强制）：串口连接必须由用户主动点击「🔌 连接串口」按钮触发，禁止自动连接**。原因：USB 烧录时串口会被烧录工具占用，自动连接会与烧录冲突、打扰未插主板的用户。所有生成的网页必须遵守此规范（加载时不自动 `requestPort()`，不自动重连）

### 代码模板
完整模板见 `references/web_serial_guide.md`（含主板固件 3 个场景 + 网页 HTML 单文件 + 排查表）。

### 用户交互流程
```
Step 1: 确认用户意图是"USB 串口连电脑控制网页"
       ↓
Step 2: 生成主板固件（Serial 按行输出，如 "KEY:A" "DIST:35"）
       ↓
Step 3: 生成网页 HTML（Web Serial API 完整模板）
       ↓
Step 4: 编译烧录主板（常规流程）
       ↓
Step 5: 提示用户：
       ① 用 Chrome/Edge 打开网页（建议 python -m http.server 8000 起服务）
       ② 点击"连接串口"，选择主板的 COM 口
       ③ 按下主板按钮/操作手柄，网页实时响应
```

### 烧录成功后的提示
```
🎉 烧录成功！USB 串口→网页控制已就绪！
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
操作步骤：
0. ⚠️ 程序不会自动运行，请先重新开关主板电源（或拔插 USB）
1. 将生成的 controller.html 放到一个文件夹
2. 在该文件夹打开终端执行：python -m http.server 8000
3. 用 Chrome/Edge 打开 http://localhost:8000/controller.html
4. 点击"🔌 连接串口"，选择主板对应的 COM 口
5. 按下主板按键或操作手柄，网页实时响应！
（如无 Python，也可直接双击 HTML 文件，部分 Chrome 版本可用）
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

### FreeRTOS 多任务编程约束

当用户需要**多个传感器同时并行工作**（如循迹 + 颜色识别 + 超声波避障 + 舵机抓取），应使用 FreeRTOS 多任务：

1. **判断是否需要多任务**：
   - ≤2 个传感器 → 使用 `millis()` 非阻塞编程即可，**不需要多任务**
   - ≥3 个传感器并行 → 推荐使用 FreeRTOS 多任务
   - WiFi + 多传感器 → 推荐使用 FreeRTOS 多任务

2. **核心分配规则**：
   - **Core 0**：传感器读取任务（颜色传感器、超声波、加速度计等）
   - **Core 1**：主控制任务（电机控制、循迹逻辑、WiFi/WebSocket）← `loop()` 默认运行在 Core 1

3. **代码规范**：
   - 使用 `xTaskCreatePinnedToCore()` 创建任务并绑定核心
   - 用 `vTaskDelay(pdMS_TO_TICKS(ms))` 替代 `delay()`
   - 跨任务共享变量必须声明为 `volatile`
   - I2C 操作集中在同一个任务中（I2C 非线程安全）
   - 任务栈大小：使用传感器库时至少 4096 字节，简单任务 2048 字节
   - 任务函数必须包含无限循环 `for(;;){}`，不能退出

4. **详细代码模板**见 `references/future_tech_box_v2_hardware.md` 的 "FreeRTOS 多任务编程" 章节
