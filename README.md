# 🤖 未来科技盒自动编程烧录 Skill

<p align="center">
  <b>让 AI 帮你写硬件代码，自然语言 → 编译 → 烧录，一气呵成！</b>
</p>

---

## 📁 项目结构

```
.
├── .codebuddy/
│   └── skills/
│       ├── future-tech-box-programmer/       # 🎯 2.0 版本 Skill（已完成）
│       │   ├── SKILL.md
│       │   ├── README.md
│       │   ├── references/
│       │   └── scripts/
│       │
│       └── future-tech-box-3.0-programmer/   # 🆕 3.0 版本 Skill（开发中）
│           ├── SKILL.md
│           ├── README.md
│           ├── references/
│           │   ├── future_tech_box_v3_hardware.md
│           │   ├── pinout_mapping_v3.csv
│           │   ├── 3.0主板正面.jpg
│           │   ├── 3.0主板背面.jpg
│           │   ├── libraries/     # 库文件（PS2X, Grove传感器等）
│           │   └── examples/      # 示例程序（循迹、PS2控制）
│           └── scripts/
│
├── v2.0-未来科技盒/                          # 📦 2.0 版本所有资料
│   ├── buzzer_led_test/                     # 蜂鸣器+LED按键控制
│   ├── car_button_stop/                     # 小车前进按键停止
│   ├── color_led_7/                         # 颜色传感器+LED联动
│   ├── i2c_sensor_test/                     # I2C传感器综合测试
│   ├── led_key_test/                        # 按键+LED控制
│   ├── led_sequence_test/                   # LED序列点亮
│   ├── line_follower/                       # 循迹小车
│   ├── mecanum_demo/                        # 麦克纳姆轮横移演示
│   ├── motor_test/                          # 电机基础测试
│   ├── ps2_car_control/                     # PS2手柄遥控小车
│   ├── ps2_tank_control/                    # PS2手柄坦克式控制
│   ├── random_melody/                       # 按键触发随机旋律
│   ├── tilt_led_display/                    # 倾斜方向LED显示
│   ├── ultrasonic_led_map/                  # 超声波距离映射LED
│   ├── ultrasonic_test/                     # 超声波基础测距
│   ├── 补充2.0/                             # 额外资料和库文件
│   ├── 未来科技盒2.0主板.png
│   └── future-tech-box-programmer-v0.2.1.zip
│
├── 未来科技盒3.0自动烧录skill/               # 📚 3.0 原始资料
│   ├── 未来科技盒3.0引脚.csv
│   ├── 3.0主板正面.jpg
│   ├── 3.0主板背面.jpg
│   └── [库文件和示例程序压缩包]
│
└── README.md                                # 本文件
```

---

## 版本说明

### 2.0 版本 ✅ 已完成
- 基于 Seeed XIAO ESP32S3
- 支持：LED矩阵、按键、蜂鸣器、超声波、电机、循迹、PS2手柄、I2C传感器、舵机、WiFi遥控
- Skill 路径：`.codebuddy/skills/future-tech-box-programmer/`
- 示例项目：`v2.0-未来科技盒/` 文件夹

### 3.0 版本 🔧 开发中
- 基于 ESP32-S3
- 新增：**RGB LED矩阵**、**语音模块**、**视觉/AI模块**
- Skill 路径：`.codebuddy/skills/future-tech-box-3.0-programmer/`
- 原始资料：`未来科技盒3.0自动烧录skill/` 文件夹

---

## 🚀 快速开始

### 环境要求

| 依赖 | 版本要求 | 说明 |
|------|---------|------|
| CodeBuddy IDE | 最新版 | 支持 Skill 功能 |
| Python | ≥ 3.8 | 运行辅助脚本 |
| PlatformIO CLI | ≥ 6.0 | 编译和烧录 |

### 使用方式

在 CodeBuddy 中输入类似指令：

**2.0 版本**：
```
编程未来科技盒2.0：让 LED 灯逐一亮起
```

**3.0 版本**：
```
编程未来科技盒3.0：让 RGB 灯显示彩虹效果
```

---

<p align="center">
  Made with ❤️ for Future Tech Box
</p>
