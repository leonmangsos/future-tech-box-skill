---
name: future-tech-box-3.0-programmer
description: This skill should be used when users want to program the Future Tech Box 3.0 (未来科技盒3.0) board based on ESP32-S3. It handles the complete workflow from natural language requirement to code generation, compilation and flashing. It also supports NEW CURRICULUM (新课标) project solution generation mode when teachers ask "how to build a project/work with the Future Tech Box" (e.g. 智能浇花系统/循迹小车方案), the skill generates standards-aligned teaching project plans based on 《义务教育信息科技课程标准（2022年版2025年修订）》 before coding. Key differences from v2.0 include RGB LED matrix (WS2812), voice module, and vision/AI camera module. Trigger phrases include "编程未来科技盒3.0", "烧录程序到3.0主板", "RGB灯", "语音模块", "视觉模块", "如何用未来科技盒做", "未来科技盒方案", "未来科技盒作品设计", or any hardware/project design request mentioning 未来科技盒3.0.
---

# 未来科技盒 3.0 自动编程烧录（v3.0 Skill）

## 概述
本 skill 实现从用户自然语言需求到代码生成、编译、烧录的完整自动化流程，并新增**新课标方案生成模式**：当教师提出「如何用未来科技盒做某某方案/作品」时，先输出符合《义务教育信息科技课程标准（2022年版2025年修订）》的项目方案，方案确认后再进入编程烧录主流程。

**目标硬件**：未来科技盒 3.0（基于 ESP32-S3）  
**开发框架**：PlatformIO + Arduino  
**支持系统**：Windows / macOS / Linux  
**Skill 版本**：v0.2.0（新增新课标方案生成模式）

### ⚠️ 与 2.0 版本的关键区别

| 特性 | 2.0 | 3.0 | 影响 |
|------|-----|-----|------|
| LED 矩阵 | 9个单色LED (行列扫描) | **3×3 RGB LED (WS2812)** | 不再需要 scanDisplay()，可用 delay() |
| 语音模块 | ❌ | ✅ UART2 (GPIO34/35) | 新功能 |
| 视觉/AI | ❌ | ✅ I2C2 (GPIO36/37) | 新功能 |
| 蜂鸣器 | GPIO26 板载 | ❌ 无蜂鸣器 | 3.0 无蜂鸣器模块；GPIO26 变为 Grove 接口4，如需声音可用语音模块 |

---

## 🌐 跨平台支持说明

本 SKILL 支持三种操作系统：

| 系统 | 串口命名 | 检测脚本 | 特殊说明 |
|------|----------|----------|----------|
| **Windows** | `COMx` | `detect_port_windows.py` | 使用 pnputil 检测 |
| **macOS** | `/dev/cu.usbmodem*` | `detect_port_macos.py` | 使用 system_profiler/ioreg |
| **Linux** | `/dev/ttyACM*` | `detect_port_linux.py` | 可能需要 dialout 用户组权限 |

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
6. Arduino 框架缓存状态

> **🚨 默认安装到非系统盘（强制，防止 C 盘爆满）**：PlatformIO 默认把所有内容装在 `~/.platformio/`（系统盘），实际总占用 **5-6GB**（平台包 ~500MB + Arduino 框架 ~500MB + 工具链 ~1.5GB + 包缓存随编译持续膨胀）。**本 skill 首次执行时，默认把 core 目录安装到非系统盘**，而不是默认 C 盘。
>
> **首次执行强制流程**：
> 1. 运行 `python scripts/check_environment.py`，检查 `platformio_core_source` 字段
> 2. 若 core 目录在系统盘（`platformio_core_source` 非 `PLATFORMIO_CORE_DIR`）→ **必须执行一键迁移脚本**：
>    ```bash
>    python scripts/migrate_core_dir.py
>    # 交互式列出磁盘，选择目标盘（如 D 盘）；或直接指定：
>    # python scripts/migrate_core_dir.py --target D:\DevTools\platformio
>    ```
> 3. 迁移完成后**必须重启 IDE/终端**（让 `PLATFORMIO_CORE_DIR` 生效）
> 4. 重新运行 `check_environment.py` 验证：`platformio_core_source` 应显示为 `PLATFORMIO_CORE_DIR`
>
> `check_environment.py` 检测到 core 在系统盘时会输出 `needs_relocate: true` 标记并追加 warning。
> 环境变量 `PLATFORMIO_CORE_DIR` 的值即为新 core 目录（用户环境变量，永久生效）。

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
2. 读取 `references/pinout_mapping_v3.csv` 获取引脚映射
3. 读取 `references/future_tech_box_v3_hardware.md` 获取硬件约束
4. 若用户在方案阶段指定了学段/课标要求，代码需与方案中的硬件选型一致
5. 生成符合 PlatformIO 结构的代码

---

### Phase 2: 编译

**执行命令**：`pio run -d <project_path>`

---

### Phase 3: 烧录

**主烧录命令**：先用 `pio run` 编译生成固件，再用 **esptool 直调 + `--after no_reset`** 烧录。

> 🚨 **为什么不用 `pio run -t upload`？**
>
> `pio run -t upload` 默认在烧录完成后会 `hard_reset`（自动运行程序）。
> 这会导致烧录完成瞬间电机/舵机被驱动，极易触发笔记本 USB 接口"电流过载提醒"（USB 口额定仅 0.5-3A，电机启动瞬间可达 1.5-2.5A）。
>
> 因此**必须使用 esptool 直调并加 `--after no_reset`**，让烧录完成后停留在下载模式，不自动运行程序。

**⚠️ 烧录前必须先获取端口**（3.0 使用 CH343，VID:PID = `1A86:55D3`）：
```bash
# Step 1: 获取端口号（运行 3.0 专用检测脚本）
python .codebuddy/skills/future-tech-box-3.0-programmer/scripts/detect_port_windows.py
# 或通用方法：
pio device list
# 找到 CH343 / USB-Enhanced-SERIAL 的设备，记录端口号（如 COM6）

# Step 2: 编译固件（确保最新代码）
pio run -d <project_path>

# Step 3: 使用 esptool 直调烧录（--after no_reset 保证烧录后不自动运行）
# Windows:
python %USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py ^
  --chip esp32s3 --port <PORT> --baud 460800 ^
  --before default_reset --after no_reset ^
  write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect ^
  0x0 <project_path>\.pio\build\esp32-s3-devkitc-1\bootloader.bin ^
  0x8000 <project_path>\.pio\build\esp32-s3-devkitc-1\partitions.bin ^
  0x10000 <project_path>\.pio\build\esp32-s3-devkitc-1\firmware.bin

# macOS / Linux:
python ~/.platformio/packages/tool-esptoolpy/esptool.py \
  --chip esp32s3 --port <PORT> --baud 460800 \
  --before default_reset --after no_reset \
  write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect \
  0x0 <project_path>/.pio/build/esp32-s3-devkitc-1/bootloader.bin \
  0x8000 <project_path>/.pio/build/esp32-s3-devkitc-1/partitions.bin \
  0x10000 <project_path>/.pio/build/esp32-s3-devkitc-1/firmware.bin
```

**说明**：
- `--flash_size detect`：esptool 自动探测 3.0 主板 Flash 容量（不同批次可能为 8MB/16MB），避免硬编码出错
- `--baud 460800`：CH343 串口稳定烧录速度（比 2.0 的 USB-Serial/JTAG 921600 略低，兼容性更好）
- `--before default_reset`：esptool 通过 DTR/RTS 自动让芯片进入下载模式，**无需用户手动按 BOOT**
- `--after no_reset`：烧录完成后芯片停留在下载模式，**不自动运行程序**
- 用户重新开关主板电源（或拔插 USB）后，新程序才会开始运行

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
| **RGB LED 矩阵** | ✅ | "LED 显示彩虹色"、"中间灯亮红色" |
| 按键 | ✅ | "按下按键 A 时..." |
| 电机控制 | ✅ | "让小车前进"、"原地左转" |
| 循迹传感器 | ✅ | "实现循迹小车" |
| 舵机/机械臂 | ✅ | "控制机械臂抓取物体" |
| PS2 手柄 | ✅ | "用手柄遥控小车" |
| I2C 加速度计 | ✅ | "检测倾斜方向" |
| I2C 颜色传感器 | ✅ | "识别颜色" |
| I2C 温湿度 | ✅ | "读取当前温度" |
| 超声波传感器 | ✅ | "测量前方距离" |
| **语音模块** | 🔧 待完善 | "语音播报距离"、"语音控制小车" |
| **视觉模块** | 🔧 待完善 | "识别前方物体"、"检测人脸" |
| 蜂鸣器 | ❌ 无蜂鸣器 | "蜂鸣器响一声"（3.0 无此模块，提示用户改用语音模块/外部喇叭） |
| WiFi Web 遥控 | ✅ | "用网页控制小车" |
| **USB 串口→网页控制（Web Serial API）** | ✅ | "USB 连电脑控制网页"、"主板当遥控器"、"按钮控制网页"、"手柄控制网页游戏" |

✅ = 已支持  🔧 = 开发中  ⚠️ = 待确认

---

## 🎯 新课标方案生成模式（v0.2.0 新增）

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
4. **接口限制合规**：接口2/3/4 为数字引脚、**3.0 无蜂鸣器**（需要声音反馈时改用语音模块或外部喇叭）、3.0 循迹引脚与 2.0 不同，方案设计不得违反
5. **学段适配**：教师指定学段时按该学段目标定难度；未指定时默认推荐 5～6 年级或 7～9 年级并说明理由
6. **方案方向库**：教师问"能做什么"时，先给出参考文档「八、方案方向库」清单，再引导选择其一深入设计
7. **技术克制（第一准则）**：默认只用低难度、已验证、易采买模块（按键/RGB LED/超声波/循迹/温湿度/加速度计/颜色/电机/舵机/PS2/WiFi；注：3.0 无蜂鸣器）。**不主动引导 GPS/TWD 定位、云平台大屏、多机协同、复杂 AI 等高难度功能**；除非用户明确要求，否则不写进方案主体。用户要求时可补充，但必须标注采买/接线/供电风险并给替代建议
8. **特色与风格优先**：方案侧重地区/文化特色、主题风格、游戏化玩法、趣味呈现，而非堆技术。先想"好不好玩、有没有特色"，再想"用什么硬件"
9. **技术可落地**：严格按用户描述要求实现；涉及接线/供电不确定的器件（继电器水泵、GPS、大功率电机）标注"需教师确认接线与供电"；优先用板载模块，外接器件降到最少
10. **非接线创客内容覆盖**：凡作品含实体形态/场景/道具，必须给出结构搭建（瓦楞纸/雪弗板/3D打印/乐高）、沙盘模型、美术道具、功能件制作等简述及安全提示
11. **方案主次结构**：主线（基础可落地玩法 + 特色/趣味）必须完整可执行；可选进阶（用户要求才展开）不影响主线完整性

### 方案 → 代码衔接
用户确认方案后，提示：
"已确认方案，接下来我将基于该方案生成代码并编译烧录。请确保：① 硬件已按方案接线；② USB 数据线已连接主板。"
然后进入 **Phase 1** 正常流程。方案中的硬件选型将决定 Phase 1 的代码生成（引脚、库、模块均按方案执行）。

---

## ⭐ RGB LED 矩阵控制要点（3.0 核心新特性）

### 控制方式
- **控制引脚**：GPIO33（单总线）
- **LED 数量**：9 个（3×3 矩阵）
- **类型**：WS2812 / NeoPixel（全彩 RGB）
- **推荐库**：`Adafruit NeoPixel`

### ⚠️ 重要：与 2.0 的编程范式差异

**2.0 单色LED（行列扫描）**：
- 需要持续调用 `scanDisplay()` 维持显示
- loop() 中禁止使用 `delay()`
- 按键必须用非阻塞方式（3.0 无板载蜂鸣器；如需提示音用语音模块）

**3.0 RGB LED（WS2812）**：
- 设置颜色后调用 `strip.show()` 即生效，**无需持续扫描**
- ✅ 可以在 loop() 中自由使用 `delay()`
- ✅ 编程大幅简化

### 代码模板

```cpp
#include <Adafruit_NeoPixel.h>

#define RGB_PIN     33
#define NUM_LEDS    9
#define BRIGHTNESS  50

Adafruit_NeoPixel strip(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

// 按编号(1-9)设置LED颜色
void setLED(int num, uint8_t r, uint8_t g, uint8_t b) {
  if (num >= 1 && num <= 9) {
    strip.setPixelColor(num - 1, strip.Color(r, g, b));
    strip.show();
  }
}

// 设置所有LED
void setAllLEDs(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

// 关闭所有LED
void clearLEDs() {
  strip.clear();
  strip.show();
}
```

### 常用效果代码

```cpp
// 彩虹效果
void rainbowEffect(int wait = 50) {
  for (long firstPixelHue = 0; firstPixelHue < 65536; firstPixelHue += 256) {
    for (int i = 0; i < NUM_LEDS; i++) {
      int pixelHue = firstPixelHue + (i * 65536L / NUM_LEDS);
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    delay(wait);
  }
}

// 逐个点亮效果
void wipeEffect(uint32_t color, int wait = 100) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}

// 呼吸灯效果（全彩）
void breatheEffect(uint8_t r, uint8_t g, uint8_t b, int speed = 10) {
  for (int brightness = 0; brightness < 255; brightness++) {
    strip.setBrightness(brightness);
    setAllLEDs(r, g, b);
    delay(speed);
  }
  for (int brightness = 255; brightness >= 0; brightness--) {
    strip.setBrightness(brightness);
    setAllLEDs(r, g, b);
    delay(speed);
  }
  strip.setBrightness(BRIGHTNESS);  // 恢复默认亮度
}

// 闪烁效果
void blinkEffect(uint32_t color, int times = 3, int interval = 200) {
  for (int t = 0; t < times; t++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, color);
    }
    strip.show();
    delay(interval);
    strip.clear();
    strip.show();
    delay(interval);
  }
}
```

### PlatformIO 配置

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

> 🚨 **关键配置说明（已验证）**：
> - 3.0 必须使用 `board = esp32-s3-devkitc-1`，**不能**使用 `seeed_xiao_esp32s3`（那是 2.0 的板型）
> - 必须设置 `ARDUINO_USB_CDC_ON_BOOT=0`，否则 `Serial.print` 输出会走 ESP32-S3 内置 USB 而非 CH343，串口监视器将看不到任何输出
> - CH343 的 VID:PID = `1A86:55D3`，在 `pio device list` 中显示为 `USB-Enhanced-SERIAL CH343`

---

## 📌 完整引脚映射表（3.0 版本）

### Grove 接口

| 接口 | GPIO_1 | GPIO_2 | 常用传感器 |
|------|--------|--------|-----------|
| **接口2** | GPIO7 | GPIO8 | 超声波传感器（SIG=GPIO7，GPIO8=NC） |
| **接口3** | GPIO3 | GPIO4 | 循迹传感器（左循迹=GPIO3, 右循迹=GPIO4） |
| **接口4** | GPIO26 | GPIO38 | 通用 Grove 接口 |
| **接口5** | GPIO5 | GPIO6 | 通用 Grove 接口 |
| **接口6** | GPIO1 | GPIO2 | 通用 Grove 接口 |

> ⚠️ **3.0 循迹传感器引脚与 2.0 不同！**
> - 2.0 循迹传感器：左=GPIO2，右=GPIO1（接口3）
> - 3.0 循迹传感器：左=GPIO3，右=GPIO4（接口3）
> - 生成循迹代码前必须确认目标版本！

### I²C 接口

| I2C 组 | 对应接口 | SDA | SCL | 说明 |
|--------|----------|-----|-----|------|
| I2C1（默认） | 接口1 / 接口8 | GPIO39 | GPIO40 | 大部分传感器默认使用 |
| I2C2 | 接口7 | GPIO37 | GPIO36 | 视觉/AI 摄像头模块 |

### 电机引脚

| 电机 | 正转 GPIO | 反转 GPIO | 位置 |
|------|----------|----------|------|
| M1 | GPIO11 | GPIO12 | 前左 |
| M2 | GPIO14 | GPIO13 | 前右 ⚠️ 正反转 GPIO 顺序相反 |
| M3 | GPIO15 | GPIO16 | 后左 |
| M4 | GPIO18 | GPIO17 | 后右 ⚠️ 正反转 GPIO 顺序相反 |

### 舵机引脚（云台结构）

| 舵机 | GPIO | 位置 | 居中角度 | 最小角度 | 最大角度 | 说明 |
|------|------|------|----------|----------|----------|------|
| S1 | GPIO47 | **云台底部（水平旋转）** | 104° | 0° | 220° | 底部无物理限位，行程较大 |
| S2 | GPIO48 | **云台顶部（俯仰）** | 180° | 90° | 270° | 受结构限制，只有一半行程 |

#### ⚠️ 舵机使用强制规范（写舵机程序前必须确认！）

1. **接线标准**：用户必须保证 **S1 = 云台底部舵机，S2 = 云台顶部舵机**。在编写任何舵机相关程序之前，必须向用户确认接线是否符合此标准。
2. **角度范围不同**：S1 和 S2 的居中位置、最小值、最大值完全不同，不能使用统一的 0~180 范围！
3. **空闲保护**：舵机无操作超过 2~3 秒必须自动 detach()，防止堵转烧毁。
4. **边界保护**：角度必须严格 constrain 在安全范围内，到达边界时停止响应。

**舵机控制代码模板**：
```cpp
#include <ESP32Servo.h>

#define SERVO1_PIN 47  // 云台底部（水平旋转）
#define SERVO2_PIN 48  // 云台顶部（俯仰）

// S1 云台底部参数
#define S1_CENTER 104
#define S1_MIN    0
#define S1_MAX    220

// S2 云台顶部参数
#define S2_CENTER 180
#define S2_MIN    90
#define S2_MAX    270

Servo servo1;
Servo servo2;

void setup() {
  // 注意：ESP32Servo 的 attach 可指定脉宽范围以支持 >180° 舵机
  // 标准舵机 500~2500μs 对应 0~270°
  servo1.attach(SERVO1_PIN, 500, 2500);
  servo2.attach(SERVO2_PIN, 500, 2500);
  servo1.write(S1_CENTER);  // 底部居中 104°
  servo2.write(S2_CENTER);  // 顶部居中 180°
}
```

> **重要提醒**：每次用户要求写舵机/云台相关程序时，必须先确认：
> "请确认您的接线：S1（GPIO47）接的是云台底部（水平旋转）舵机，S2（GPIO48）接的是云台顶部（俯仰）舵机，对吗？"

### 语音模块（UART2）

| 功能 | GPIO | 说明 |
|------|------|------|
| TX | GPIO34 | 语音模块通信 |
| RX | GPIO35 | 语音模块通信 |

**语音模块通信代码模板**：
```cpp
#define VOICE_TX 34
#define VOICE_RX 35

// 使用 Serial1 作为语音模块串口
void setup() {
  Serial.begin(115200);   // USB 调试串口
  delay(2000);
  Serial1.begin(9600, SERIAL_8N1, VOICE_RX, VOICE_TX);  // 语音模块串口
  Serial.println("语音模块串口初始化完成");
}

void loop() {
  // 接收语音模块发来的指令
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n');
    Serial.print("收到语音指令: ");
    Serial.println(cmd);
    // 根据指令执行相应操作...
  }
}
```

### PS2 手柄引脚

| 功能 | GPIO |
|------|------|
| CLK | GPIO41 |
| CMD | GPIO9 |
| CS | GPIO42 |
| DAT | GPIO10 |

### 按键引脚

| 按键 | GPIO | 说明 |
|------|------|------|
| KEY_A | GPIO21 | 按下时 LOW |
| KEY_B | GPIO0 | 按下时 LOW（兼 BOOT） |

### 排针引出引脚

| GPIO | 功能 |
|------|------|
| GPIO19 | 串口TX |
| GPIO20 | 串口RX |
| GPIO43 | TX2 |
| GPIO44 | RX2 |
| GPIO45 | Flash电压选择IO |
| GPIO46 | 启动模式选择IO |

### RGB LED

| 功能 | GPIO | 说明 |
|------|------|------|
| RGB LED 数据总线 | GPIO33 | WS2812 × 9，单总线控制 |

---

## 按键控制要点

**与 2.0 相同**：按键使用内部上拉电阻，按下时为 LOW。

| 按键 | GPIO | 说明 |
|------|------|------|
| KEY_A | GPIO21 | 按下时 LOW |
| KEY_B | GPIO0 | 按下时 LOW（兼 BOOT） |

**🎉 3.0 简化点**：由于 RGB LED 不需要扫描，按键可以使用简单的 delay() 消抖方式：

```cpp
bool lastKeyA = HIGH;

void loop() {
  bool currentKeyA = digitalRead(KEY_A);
  
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    delay(50);  // ✅ 3.0 可以用 delay 消抖
    onKeyAPressed();
  }
  
  lastKeyA = currentKeyA;
}
```

---

## 电机控制要点

与 2.0 版本引脚相同（但循迹传感器引脚不同，见引脚映射表）。

**⚠️ 重要**：右侧电机 (M2, M4) 的正反转 GPIO 顺序与左侧相反。

**默认速度建议**：180（70%功率），不超过 220。

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

void setMotors(int leftSpeed, int rightSpeed) {
  setMotor(M1_FWD, M1_REV, leftSpeed);
  setMotor(M2_FWD, M2_REV, rightSpeed);
  setMotor(M3_FWD, M3_REV, leftSpeed);
  setMotor(M4_FWD, M4_REV, rightSpeed);
}

// 前进: setMotors(180, 180);
// 后退: setMotors(-180, -180);
// 左转: setMotors(-180, 180);
// 右转: setMotors(180, -180);
```

### 循迹传感器（3.0 引脚！）

```cpp
// ⚠️ 3.0 循迹引脚与 2.0 不同！
#define TRACK_LEFT  3   // 接口3-GPIO3（左循迹）
#define TRACK_RIGHT 4   // 接口3-GPIO4（右循迹）

void setup() {
  pinMode(TRACK_LEFT, INPUT);
  pinMode(TRACK_RIGHT, INPUT);
}

// 黑线上 = LOW, 白色地面 = HIGH（根据实际传感器可能需要反转）
```

---

## PS2 手柄控制要点

引脚与 2.0 相同：
| 功能 | GPIO |
|------|------|
| CLK | GPIO41 |
| CMD | GPIO9 |
| CS | GPIO42 |
| DAT | GPIO10 |

**依赖库**：`Arduino-PS2X-ESP32-master`（在 `references/libraries/` 中提供）

> PS2 手柄控制架构与 2.0 版本相同，需要硬件定时器中断读取。

---

## Grove 传感器

3.0 版本有 6 个 Grove 接口，可接入多种传感器。

### 超声波传感器（接口2，GPIO7）

**⚠️ 这是单线超声波模块（SIG 模式）**，只使用 GPIO7 一个引脚完成距离测量。GPIO8 连接的是超声波模块的 NC（空引脚），忽略即可。

**超声波传感器代码模板**：
```cpp
#define ULTRASONIC_PIN 7  // 接口2-GPIO7，单线 SIG 模式

float readUltrasonic() {
  // Step 1: 发送触发脉冲
  pinMode(ULTRASONIC_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_PIN, LOW);
  
  // Step 2: 切换为输入，读取回波
  pinMode(ULTRASONIC_PIN, INPUT);
  long duration = pulseIn(ULTRASONIC_PIN, HIGH, 30000);
  
  // Step 3: 计算距离
  if (duration == 0) return -1;
  return duration / 58.0;  // 距离(cm) = 脉冲时间(μs) / 58
}
```

**⚠️ 超声波传感器注意事项**：
1. GPIO8 不需要初始化
2. 测量间隔至少 60ms，推荐 100ms
3. 有效量程约 2cm ~ 400cm
4. 建议使用中值滤波消除跳变（5次采样取中值）

### 温湿度传感器 (DHT20)
**依赖库**：`Grove_Temperature_And_Humidity_Sensor`
**I2C 地址**：0x38
```cpp
#include "DHT.h"
// DHT20 使用 I2C 接口
Wire.begin(39, 40);  // I2C1: SDA=39, SCL=40
```

### 三轴加速度计 (LIS3DHTR)
**依赖库**：`LIS3DHTR`
**I2C 地址**：0x18
```cpp
#include "LIS3DHTR.h"
LIS3DHTR<TwoWire> lis;
Wire.begin(39, 40);  // I2C1: SDA=39, SCL=40
lis.begin(Wire, 0x18);
```

### 颜色传感器 (VEML6040)
**依赖库**：`VEML6040`
**I2C 地址**：0x10

### LED Bar
**依赖库**：`Grove_LED_Bar`

### 🚨 I²C 传感器编码最佳实践

1. **I2C 初始化必须指定引脚**：
```cpp
Wire.begin(39, 40);   // I2C1: 接口1/接口8
// Wire.begin(37, 36); // I2C2: 接口7
```

2. **必须在初始化前进行 I2C 扫描**：
```cpp
void scanI2C() {
  Serial.println("正在扫描 I2C 设备...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  找到设备: 0x%02X\n", addr);
    }
  }
}
```

3. **传感器初始化失败必须提供降级模式**：
```cpp
// ❌ 错误：死循环
if (!sensor) { while (1) { 闪灯; } }

// ✅ 正确：降级到演示模式
bool sensorFound = false;
if (!sensor) {
  Serial.println("传感器未检测到，进入演示模式");
  sensorFound = false;
}
```

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

### 3.0 专用前置条件（必须满足）
- **CH343 驱动**：Windows 需装 WCH `CH343SER` 驱动，设备管理器出现黄叹号即未装
- **platformio.ini**：`board = esp32-s3-devkitc-1` + `-DARDUINO_USB_CDC_ON_BOOT=0`（否则 Serial 输出走内置 USB，网页收不到）
- 浏览器：Chrome / Edge；打开方式：localhost 或 HTTPS
- 波特率一致：固件 `Serial.begin(115200)` = 网页 `port.open({baudRate: 115200})`
- USB 数据线（充电线无法通讯）；`setup()` 中 `delay() ≥ 2000ms`
- **⚠️ 连接触发规范（强制）：串口连接必须由用户主动点击「🔌 连接串口」按钮触发，禁止自动连接**。原因：USB 烧录时串口会被烧录工具占用，自动连接会与烧录冲突、打扰未插主板的用户。所有生成的网页必须遵守此规范（加载时不自动 `requestPort()`，不自动重连）

### 处理流程
1. 提示用户前置条件（尤其 CH343 驱动 + platformio.ini 配置）
2. 生成主板固件代码（`Serial.println` 按行发指令）
3. 生成网页 HTML 文件（Web Serial API，含连接/断开/收发/日志）
4. 提示用户：本地起服务 `python -m http.server 8000`，浏览器打开 `http://localhost:8000`
5. 编译烧录主板固件（按常规 Phase 2/3 流程）

### 代码模板
完整模板见 `references/web_serial_guide.md`（含 3.0 的 platformio.ini + 主板固件 3 个场景 + 网页 HTML 单文件 + 排查表）。

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

## 🧳 离线安装包方案（国内/机房/无网络首选）

国内网络访问 PlatformIO Registry、dl.espressif.com、github 可能很慢甚至失败。
**推荐在一台能联网的电脑上提前打包好离线安装包，分发到教学电脑直接解压使用。**

### 生成离线包（在【能联网】的电脑上执行一次）

前提：该电脑已成功完成过一次 espressif32 编译（core 数据齐全）。

```bash
python scripts/build_offline_package.py --output ./offline_package
# 生成: offline_package/future-tech-box-pio-<系统>-<架构>.zip （约 400-700MB）
```

**注意**：
- 包内只保留 espressif32 相关组件（平台包+工具链+Arduino 框架+esptool）
- **跨平台不通用**：Windows 的包只能在 Windows 用；如目标机是 macOS/Linux 需在对应系统打包
- 打包过程会同时下载 PlatformIO CLI 的 wheel（供离线 pip 安装）

### 安装离线包（在【目标】电脑上执行）

```bash
# 方式 A：使用安装脚本（推荐，自动解压+装CLI+设环境变量）
python scripts/install_offline_package.py <离线包.zip>

# 方式 B：手动安装
# 1. 解压 zip 到任意目录（如 D:\DevTools\platformio）
# 2. 离线装 CLI: cd 解压目录/platformio_wheel && pip install --no-index --find-links=. platformio
# 3. 设置环境变量 PLATFORMIO_CORE_DIR=解压目录/platformio（需重启终端）
# 4. 验证: pio --core-dir 应输出该目录
```

### 检测脚本对离线包的适配

`check_environment.py` 通过 `get_platformio_core_dir()` 定位 core 目录：
- 离线包解压并设置 `PLATFORMIO_CORE_DIR` 后，检测脚本会**自然判定组件就绪**（实时检测文件系统）
- 无需修改检测逻辑；`platformio_core_source` 会显示为 `PLATFORMIO_CORE_DIR`

### 离线包放置建议

| 放置位置 | 是否合适 |
|----------|---------|
| 项目目录本地（如 `offline_package/`） | ✅ 推荐，但注意 zip 约 500MB，勿提交到 git |
| 网盘 / 学校共享盘 | ✅ 分发最方便 |
| git 仓库 | ❌ 太大，建议用 .gitignore 排除 |

---

## 引用资源

- **引脚映射**：`references/pinout_mapping_v3.csv`
- **硬件规格**：`references/future_tech_box_v3_hardware.md`
- **新课标方案设计参考**：`references/new_curriculum_solution_design.md`（方案生成模式必读：核心素养四维、学段定位、硬件映射、方案输出结构、方案方向库）
- **USB 串口→网页控制参考**：`references/web_serial_guide.md`（Web Serial API：3.0 platformio.ini + 主板固件模板 + 网页 HTML 模板 + 前置条件 + 排查表）
- **主板图片**：`references/3.0主板正面.jpg`、`references/3.0主板背面.jpg`
- **库文件**：`references/libraries/`
  - Arduino-PS2X-ESP32-master.zip
  - Grove_LED_Bar.zip
  - Grove_Temperature_And_Humidity_Sensor.zip
  - Grove_Ultrasonic_Ranger.zip
  - Grove-3-Axis-Digital-Accelerometer-2g-to-16g-LIS3DHTR.zip
  - VEML6040.zip
- **示例程序**：`references/examples/`
  - followline.7z（循迹小车）
  - ps_mode.7z（PS2 手柄控制）

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
└── lib/                # 第三方库（按需复制）
    └── PS2X_lib/       # PS2 手柄库（如需要）
```

---

## 约束与限制

1. 仅支持未来科技盒 3.0（ESP32-S3 QFN56 + CH343）
2. 需要稳定的网络连接（首次下载依赖）
3. USB 必须是数据线（非充电线）
4. **烧录必须使用 esptool 直调 + `--after no_reset`**（禁止 `pio run -t upload`，因其会 hard_reset 自动运行程序，烧录完成瞬间驱动电机可能触发笔记本 USB 电流过载提醒）。烧录完成后程序不自动运行，需提示用户**重新开关主板电源**后运行
5. **烧录时必须显式指定串口端口**，禁止依赖 PlatformIO 自动检测
6. **`setup()` 中 `delay()` 必须 ≥ 2000ms**，确保 USB 串口稳定
7. **PlatformIO 命令优先使用 `python -m platformio`**，如果 `pio.exe` 被系统策略阻止则必须使用此方式
8. **3.0 必须使用 `board = esp32-s3-devkitc-1`**，禁止使用 `seeed_xiao_esp32s3`（那是 2.0 板型）
9. **必须设置 `ARDUINO_USB_CDC_ON_BOOT=0`**，否则串口输出走错通道
10. **RGB LED 亮度建议 20-50**，避免 LED 过亮刺眼和电流过大
11. **NeoPixel 亮度调节禁止闪白反馈**：`setBrightness()` 后必须直接用当前颜色 `show()`，不得插入闪白效果
12. **I2C 传感器必须提供降级模式**，初始化失败不能进入死循环
13. **3.0 循迹传感器引脚与 2.0 不同**：3.0 用 GPIO3/GPIO4，2.0 用 GPIO2/GPIO1
14. 语音模块和视觉模块功能待完善（需要更多硬件信息）

---

## 常见问题排查指南

### 问题 1：`pio.exe` 被 Windows 应用控制策略阻止

**症状**：
```
程序"pio.exe"无法运行: An Application Control policy has blocked this file
```

**解决方案（已验证有效）**：使用 `python -m platformio` 代替 `pio` 命令：

```bash
# ❌ 被阻止的写法
pio run

# ✅ 正确写法（编译/设备列表用 python -m platformio）
python -m platformio run
python -m platformio device list
# ⚠️ 烧录不再用 pio upload（会 hard_reset 自动运行），改用 esptool 直调 + --after no_reset（见 Phase 3）
python %USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py \
  --chip esp32s3 --port COM10 --baud 460800 \
  --before default_reset --after no_reset write_flash ...
```

**⚠️ SKILL 执行时的强制规则**：所有 PlatformIO 命令一律使用 `python -m platformio` 前缀。

### 问题 2：board 配置错误导致编译失败或串口无输出

**症状**：编译失败，或烧录后串口无输出

**根因**：使用了 2.0 的 `seeed_xiao_esp32s3` 板型

```ini
; ❌ 错误
board = seeed_xiao_esp32s3

; ✅ 正确（3.0 版本）
board = esp32-s3-devkitc-1
```

### 问题 3：串口无输出（USB CDC 配置缺失）

**症状**：程序烧录成功，LED 正常工作，但串口监视器看不到任何输出

**根因**：3.0 使用 CH343 做 USB 转串口，但如果不显式关闭 USB CDC on Boot，串口输出走错通道

**解决方案**：确保 `platformio.ini` 中有：
```ini
build_flags = 
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=0
```

### 问题 4：CH343 驱动未安装

**症状**：USB 连接后设备管理器显示未知设备或黄色感叹号

**解决方案**：
1. 下载 CH343 驱动：https://www.wch.cn/downloads/CH343SER_EXE.html
2. 安装后重新插入 USB
3. 设备管理器应显示 `USB-Enhanced-SERIAL CH343 (COMx)`

### 问题 5：NeoPixel RGB LED 亮度切换时闪烁

**症状**：按键调节亮度时，LED 先闪烁白光，然后才恢复正确亮度

**根因**：代码中在 `setBrightness()` 后插入了闪白反馈效果

**❌ 错误写法**（会闪烁）：
```cpp
void onKeyPressed() {
  strip.setBrightness(newBrightness);
  // ❌ 闪白反馈
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
  }
  strip.show();
  delay(50);
}
```

**✅ 正确写法**（平滑过渡）：
```cpp
void onKeyPressed() {
  strip.setBrightness(newBrightness);
  // ✅ 直接用当前颜色刷新
  strip.show();  // 立即以新亮度显示当前颜色
}
```

### 问题 6：编译错误 - ledcAttach 未定义

**症状**：`error: 'ledcAttach' was not declared in this scope`

**解决方案**：使用 Arduino Core 2.x 兼容 API：
```cpp
ledcSetup(channel, PWM_FREQ, PWM_RESOLUTION);
ledcAttachPin(pin, channel);
ledcWrite(channel, brightness);  // 注意：使用通道号
```

### 问题 7：烧录失败 - 端口不可用

**解决方案**：
1. 先运行 `python -m platformio device list` 确认端口（或运行 3.0 专用检测脚本 `scripts/detect_port_*.py`）
2. 指定端口用 esptool 直调烧录（见 Phase 3，`--after no_reset`）
3. 如仍失败，按 RST 按钮后重试
4. 备用方案：使用 `scripts/upload_with_retry.py` 自动重试烧录

### 问题 8：烧录显示成功但程序未生效

**解决方案**：
1. **先确认已重新上电**：no_reset 模式下烧录完成后程序不会自动运行，需重新开关主板电源（或拔插 USB）
2. 始终显式指定端口（最关键）
3. 代码中 `setup()` 使用 `delay(2000)`
4. 使用 I2C 扫描辅助调试
5. 传感器初始化失败时切换到降级/演示模式，不要死循环

---

## ⚠️ 开发中 / 待补充内容

### 语音模块
- [x] ✅ 通信引脚已确认：UART2 GPIO34(TX)/GPIO35(RX)
- [ ] 确认语音模块具体型号
- [ ] 确认通信协议和波特率（暂用 9600）
- [ ] 编写完整驱动代码模板
- [ ] 测试语音播报/识别功能

### 视觉模块 / AI 摄像头
- [x] ✅ 接口已确认：I2C2 GPIO37(SDA)/GPIO36(SCL)
- [ ] 确认 AI 板型号
- [ ] 确认 I2C 地址
- [ ] 编写通信代码模板
- [ ] 测试视觉识别功能

### 蜂鸣器
- [x] ✅ 已确认：**3.0 无板载蜂鸣器模块**（与 2.0 不同；GPIO26 变为 Grove 接口4）
- [ ] 如需声音反馈，确认外部喇叭/语音模块的接入方式
- [ ] 适配声音反馈代码模板

### RGB LED 矩阵
- [x] ✅ 控制引脚已确认：GPIO33
- [x] ✅ 类型已确认：WS2812 / NeoPixel（NEO_GRB + NEO_KHZ800）
- [x] ✅ 亮度安全范围：建议 20-50
- [ ] 确认物理排列顺序（LED 索引对应主板位置）

### 电机方向
- [x] ✅ 引脚映射已确认（与 2.0 相同）
- [x] ✅ 右侧电机 M2/M4 的 GPIO 正反转顺序确认需要反转
- [ ] 实测确认各电机实际正反转方向

### 已确认的硬件信息（与 2.0 对比）
- [x] ✅ 主板型号：ESP32-S3 QFN56（非 XIAO ESP32S3）
- [x] ✅ USB 转串口：CH343（VID:PID = 1A86:55D3）
- [x] ✅ PlatformIO board：`esp32-s3-devkitc-1`
- [x] ✅ 必须设置 `ARDUINO_USB_CDC_ON_BOOT=0`
- [x] ✅ 舵机引脚：S1=GPIO47, S2=GPIO48
- [x] ✅ 循迹传感器引脚：左=GPIO3, 右=GPIO4（与 2.0 不同！）
- [x] ✅ Grove 接口引脚映射（接口2-6 全部确认）
