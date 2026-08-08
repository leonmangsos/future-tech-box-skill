# 🤖 未来科技盒 3.0 自动编程烧录 Skill

> **版本**：v0.1.0（初始版本）  
> **状态**：开发中  
> **基于**：未来科技盒 3.0（ESP32-S3）

## 与 2.0 版本的主要区别

| 特性 | 2.0 | 3.0 |
|------|-----|-----|
| LED | 9个单色LED（行列扫描） | **3×3 RGB LED（WS2812）** |
| 语音 | ❌ | ✅ UART2 语音模块 |
| 视觉 | ❌ | ✅ I2C2 摄像头AI板 |
| 蜂鸣器 | GPIO26 板载 | ❌ 无蜂鸣器（声音用语音模块） |
| 编程难度 | 较复杂（需非阻塞编程） | **更简单**（RGB LED 无需扫描） |

## 文件结构

```
future-tech-box-3.0-programmer/
├── SKILL.md                           # Skill 主配置（AI 读取）
├── README.md                          # 本文件
├── references/
│   ├── future_tech_box_v3_hardware.md # 完整硬件规格文档
│   ├── pinout_mapping_v3.csv          # GPIO 引脚映射表
│   ├── 3.0主板正面.jpg                 # 主板正面图
│   ├── 3.0主板背面.jpg                 # 主板背面图
│   ├── libraries/                     # 库文件
│   │   ├── Arduino-PS2X-ESP32-master.zip
│   │   ├── Grove_LED_Bar.zip
│   │   ├── Grove_Temperature_And_Humidity_Sensor.zip
│   │   ├── Grove_Ultrasonic_Ranger.zip
│   │   ├── Grove-3-Axis-Digital-Accelerometer-2g-to-16g-LIS3DHTR.zip
│   │   └── VEML6040.zip
│   └── examples/                      # 示例程序
│       ├── followline.7z              # 循迹小车
│       └── ps_mode.7z                 # PS2 手柄控制
└── scripts/                           # 辅助脚本
    ├── check_environment.py           # 环境检测（PlatformIO/串口/工具链）
    ├── detect_port_windows.py         # Windows 串口检测（CH343/ESP32-S3）
    ├── detect_port_macos.py           # macOS 串口检测
    ├── detect_port_linux.py           # Linux 串口检测
    ├── upload_with_retry.py           # esptool 直调烧录（--after no_reset，防 USB 过载）
    ├── build_offline_package.py       # 离线包构建
    └── install_offline_package.py     # 离线包安装
```

## 开发进度

### ✅ 已完成
- [x] 引脚配置整理
- [x] 硬件文档初版
- [x] SKILL.md 框架
- [x] RGB LED 代码模板
- [x] 库文件和示例程序归档
- [x] 环境检测脚本（check_environment.py）
- [x] 串口检测脚本（detect_port_windows/macos/linux.py，适配 CH343 1A86:55D3）
- [x] 烧录脚本（upload_with_retry.py，esptool 直调 + `--after no_reset` 防 USB 过载）

### 🔧 进行中
- [ ] 语音模块驱动
- [ ] 视觉模块驱动
- [x] ✅ 蜂鸣器确认（3.0 无板载蜂鸣器）
- [ ] 电机方向实测
- [ ] RGB LED 实测验证

### 📋 待开始
- [ ] 完整示例项目
- [ ] 多任务编程模板
- [ ] WiFi Web 遥控适配
