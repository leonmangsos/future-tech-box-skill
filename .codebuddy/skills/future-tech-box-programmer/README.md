# 🚀 Future Tech Box 2.0 Programmer Skill

[中文](#中文说明) | [English](#english)

---

## 中文说明

### 📦 简介

这是一个用于 **CodeBuddy IDE** 的自动编程 Skill，专门针对「**未来科技盒 2.0**」主板（基于 Seeed XIAO ESP32S3）。

通过自然语言描述，AI 可以自动：
1. 理解用户需求
2. 生成符合硬件规格的代码
3. 编译程序
4. 烧录到主板

### ⚠️ 重要说明

> **当前版本状态：v0.2.0-beta（小车形态支持）**
>
> - ✅ 仅支持「**未来科技盒 2.0**」主板
> - ❌ 不支持其他版本（1.0、3.0 等）
> - ✅ 新增小车形态完整支持

### ✅ 已支持功能

| 功能模块 | 状态 | 说明 |
|---------|------|------|
| LED 矩阵（3×3） | ✅ 完成 | 行列扫描、单灯/全部控制、动画效果 |
| 按键控制（A/B） | ✅ 完成 | 边沿检测、非阻塞消抖 |
| 蜂鸣器 | ✅ 完成 | 音调播放、非阻塞控制 |
| 超声波传感器 | ✅ 完成 | 测距 3-350cm、LED 联动、非阻塞 |
| **电机控制（小车）** | ✅ **完成** | 4电机 PWM 调速、前进/后退/转弯/横移 |
| **循迹传感器** | ✅ **完成** | 双路循迹、循迹小车 |
| **舵机/机械臂** | ✅ **完成** | 双舵机控制、抓取动作 |
| **PS2 手柄** | ✅ **完成** | 双摇杆控制、按键检测 |
| **I²C 加速度计** | ✅ **完成** | LIS3DHTR ±2g~±16g |
| **I²C 颜色传感器** | ✅ **完成** | VEML6040 RGBW 识别 |
| **I²C 温湿度** | ✅ **完成** | DHT20 温湿度读取 |
| 串口通讯 | ✅ 完成 | 调试输出 |

### 🚗 小车形态电机布局

```
        前方
    ┌─────────┐
    │  M1  M2 │   M1=左上  M2=右上
    │         │
    │  M3  M4 │   M3=左下  M4=右下
    └─────────┘
        后方
```

### 📁 文件结构

```
future-tech-box-programmer/
├── SKILL.md                    # Skill 主配置文件（AI 读取）
├── README.md                   # 本说明文件
├── references/
│   ├── future_tech_box_v2_hardware.md  # 硬件规格文档
│   ├── pinout_mapping.csv              # 引脚映射表
│   └── libraries/                      # 库文件
│       └── PS2X_lib/                   # PS2 手柄库
└── scripts/
    ├── check_environment.py    # 环境检测脚本
    ├── detect_port_windows.py  # Windows 串口检测
    ├── detect_port_macos.py    # macOS 串口检测
    ├── detect_port_linux.py    # Linux 串口检测
    └── upload_with_retry.py    # 带重试的烧录脚本
```

### 🔧 安装方法

#### 方法 1：复制到项目（推荐）

将 `future-tech-box-programmer` 文件夹复制到你的项目中：

```
your-project/
└── .codebuddy/
    └── skills/
        └── future-tech-box-programmer/
            └── ... (所有文件)
```

#### 方法 2：复制到用户目录（全局可用）

```
# Windows
%USERPROFILE%\.codebuddy\skills\future-tech-box-programmer\

# macOS / Linux
~/.codebuddy/skills/future-tech-box-programmer/
```

### 💡 使用示例

在 CodeBuddy 中输入自然语言指令：

```
"让 LED 灯从 1 号到 9 号依次亮起"
"按下按键 A 时蜂鸣器响一声，点亮所有灯"
"让小车前进 3 秒后停止"
"实现循迹小车功能"
"用 PS2 手柄遥控小车"
"读取当前温度和湿度"
"识别前方物体的颜色"
"检测主板的倾斜方向"
```

### ⚙️ 系统要求

- **IDE**：CodeBuddy（支持 Skill 功能）
- **Python**：3.8+
- **PlatformIO CLI**：6.0+
- **操作系统**：Windows / macOS / Linux

### 🐛 已知问题

1. **首次编译较慢**：需要下载 ESP32 工具链（约 300-500MB），请耐心等待
2. **串口权限（Linux）**：需要将用户添加到 `dialout` 组
3. **命令超时**：长时间编译可能触发 IDE 超时，可在终端手动执行

### 📝 更新日志

#### v0.2.0-beta (2026-04-06)
- ✅ **新增小车形态完整支持**
  - 4电机 PWM 控制（M1-M4）
  - 前进/后退/左转/右转/原地转/横移
- ✅ **新增循迹传感器支持**
  - 双路循迹模块
  - 循迹小车示例
- ✅ **新增 I²C 传感器支持**
  - LIS3DHTR 三轴加速度计
  - VEML6040 RGBW 颜色传感器
  - DHT20 温湿度传感器
- ✅ **新增 PS2 手柄遥控**
  - 双摇杆控制
  - 按键检测
- ✅ **新增舵机/机械臂控制**
  - 双舵机控制
  - 抓取动作示例
- 📝 大幅更新硬件文档
- 📝 添加多个示例项目

#### v0.1.1-beta (2026-03-16)
- ✅ 新增超声波传感器支持（测距 3-350cm）
- ✅ 添加超声波 + LED 矩阵联动示例
- ✅ 新增随机旋律功能（按键A触发）
- 📝 更新硬件文档，添加超声波章节

#### v0.1.0-beta (2025-03-16)
- ✅ 完成 LED 矩阵控制（行列扫描）
- ✅ 完成按键边沿检测 + 非阻塞消抖
- ✅ 完成蜂鸣器非阻塞控制
- ✅ 添加跨平台串口检测脚本
- ✅ 添加环境检测脚本
- 🐛 修复按键消抖逻辑 BUG
- 🐛 修复蜂鸣器 API 兼容性问题

### 🤝 贡献

欢迎提交 Issue 和 Pull Request！

如果你有以下需求，欢迎联系：
- 支持其他版本的未来科技盒
- 添加新的传感器/模块支持
- 改进代码生成质量

### 📄 License

MIT License

---

## English

### 📦 Introduction

This is an auto-programming Skill for **CodeBuddy IDE**, specifically designed for the "**Future Tech Box 2.0**" board (based on Seeed XIAO ESP32S3).

Through natural language descriptions, AI can automatically:
1. Understand user requirements
2. Generate code that complies with hardware specifications
3. Compile the program
4. Flash to the board

### ⚠️ Important Notes

> **Current Version Status: v0.2.0-beta (Car Mode Support)**
>
> - ✅ Only supports "**Future Tech Box 2.0**"
> - ❌ Does NOT support other versions (1.0, 3.0, etc.)
> - ✅ Full car mode support added

### ✅ Supported Features

| Module | Status | Description |
|--------|--------|-------------|
| LED Matrix (3×3) | ✅ Done | Row-column scanning, single/all control, animations |
| Button Control (A/B) | ✅ Done | Edge detection, non-blocking debounce |
| Buzzer | ✅ Done | Tone playback, non-blocking control |
| Ultrasonic Sensor | ✅ Done | Range 3-350cm, LED integration, non-blocking |
| **Motor Control (Car)** | ✅ **Done** | 4-motor PWM, forward/backward/turn/strafe |
| **Line Follower** | ✅ **Done** | Dual sensors, line following car |
| **Servo/Arm** | ✅ **Done** | Dual servo, grab actions |
| **PS2 Controller** | ✅ **Done** | Dual joystick, button detection |
| **I²C Accelerometer** | ✅ **Done** | LIS3DHTR ±2g~±16g |
| **I²C Color Sensor** | ✅ **Done** | VEML6040 RGBW detection |
| **I²C Temp/Humidity** | ✅ **Done** | DHT20 reading |
| Serial Communication | ✅ Done | Debug output |

### 📄 License

MIT License
