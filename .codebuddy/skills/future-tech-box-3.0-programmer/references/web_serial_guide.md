# 未来科技盒 · USB 串口 ↔ 网页交互指南（Web Serial API）

> 本文件为 SKILL 的「USB 串口→网页控制」能力提供参考模板。
> 适用：主板通过 USB 连电脑，网页直接读写主板串口，把主板变成遥控器/输入设备，实时控制网页效果。
> 核心：**Web Serial API**（浏览器原生，无需安装任何东西，无需服务器）。

---

## 一、适用场景（用户问这些就用本能力）

| 场景 | 描述 | 主板固件侧 | 网页侧 |
|------|------|-----------|--------|
| 1 | 主板当遥控器，USB 连电脑，控制网页 | 读按键/传感器，`Serial.println` 发指令 | 网页监听串口数据 → 触发 JS 效果 |
| 2 | 主板按钮直接控制网页效果 | 读 KEY_A/KEY_B，串口发指令 | 收到指令 → 改 CSS/播放音效/翻页 |
| 3 | 手柄控制主板 → 主板转发给电脑 → 控制网页 | PS2 手柄解析 + 串口转发 | 收到手柄键值 → 控制网页游戏/动画 |

## 二、前置条件（必须满足，缺一不可）

| 条件 | 说明 | 处理 |
|------|------|------|
| 浏览器 | 仅 **Chrome / Edge**（桌面版）支持 Web Serial API；Firefox/Safari 不支持 | 提示用户用 Chrome 打开 |
| 打开方式 | 需 **localhost 或 HTTPS**（secure context）；双击 file:// 打开可能受限 | 开发时用 `python -m http.server 8000` 起本地服务 |
| USB 数据线 | 必须是**数据线**（能传数据），充电线无法通讯 | 提示换数据线 |
| 波特率一致 | 固件 `Serial.begin()` 与网页 `port.open({baudRate})` 必须相同 | 统一用 115200 |
| 串口独占 | 串口被占用（串口监视器/其他程序）时网页连不上 | 关闭其他占用程序 |

### ⚠️ 连接触发规范（强制，必须遵守）

**串口连接必须由用户主动触发，禁止任何形式的自动连接**。原因：

> 每次用 USB 线连接主板进行**烧录时，串口（COM 口）会被烧录工具占用**。如果网页/应用在打开时自动连接串口，会导致：① 烧录时串口被抢占，网页与烧录互相冲突；② 用户可能根本没插主板/没打算用串口，却被弹窗骚扰；③ 拔插 USB 后端口变化，自动连接难以感知。

因此：
- 网页/应用**加载时不得自动调用 `navigator.serial.requestPort()` 或自动打开端口**
- 必须提供一个明确的「🔌 连接串口」按钮，由用户点击后触发连接
- 连接过程走 `requestPort()`（浏览器会弹出设备选择框，这一步天然是用户主动行为，不能绕过）
- 断开也应由用户主动点击「断开」，或在页面卸载时优雅释放，但绝不自动重连
- 生成代码/文档/演示时，都必须向用户说明这一规范

（本指南的所有网页模板均已按此规范实现：连接由「🔌 连接串口」按钮触发。）

### 3.0 专用说明（ESP32-S3 + CH343 外置串口芯片）
- 串口走 **CH343 外置 USB 转串口芯片**（VID:PID `1A86:55D3`）
- **Windows 需手动安装 CH343 驱动**（WCH 官网 `CH343SER`）；设备管理器显示"未识别设备/黄色感叹号"即驱动缺失
- `platformio.ini` 必须设置 `board = esp32-s3-devkitc-1` + `ARDUINO_USB_CDC_ON_BOOT=0`，否则 `Serial` 输出会走内置 USB 而非 CH343，网页收不到数据
- `setup()` 中 `delay()` 必须 ≥ 2000ms，等 USB 枚举稳定

---

## 三、主板固件侧模板（Arduino / PlatformIO）

### 3.1 platformio.ini（3.0 必须）

```ini
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200

build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=0
```

> ⚠️ 缺少 `ARDUINO_USB_CDC_ON_BOOT=0` 会导致 Serial 输出走内置 USB 而非 CH343，网页将收不到任何数据。

### 3.2 场景 2：按钮控制网页（最简版）

```cpp
#include <Arduino.h>

#define KEY_A 21
#define KEY_B 0

void setup() {
  Serial.begin(115200);
  delay(2000);  // 等 USB 串口稳定（必须）
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  Serial.println("READY");
}

void loop() {
  static bool lastA = HIGH, lastB = HIGH;
  bool a = digitalRead(KEY_A);
  bool b = digitalRead(KEY_B);

  // 下降沿检测（按下瞬间发送一次）
  if (lastA == HIGH && a == LOW) {
    Serial.println("KEY_A_PRESSED");
  }
  if (lastB == HIGH && b == LOW) {
    Serial.println("KEY_B_PRESSED");
  }

  lastA = a;
  lastB = b;
  delay(20);  // 简单消抖
}
```

### 3.3 场景 1：主板当遥控器（按键 + 传感器混合）

```cpp
#include <Arduino.h>

#define KEY_A 21
#define KEY_B 0

// 自定义协议：每条指令一行，冒号分隔，如 "KEY:A" "DIST:35"
void sendCmd(const String &name, const String &value) {
  Serial.print(name);
  Serial.print(":");
  Serial.println(value);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  Serial.println("READY");
}

void loop() {
  static bool lastA = HIGH, lastB = HIGH;
  bool a = digitalRead(KEY_A);
  bool b = digitalRead(KEY_B);

  if (lastA == HIGH && a == LOW) sendCmd("KEY", "A");
  if (lastB == HIGH && b == LOW) sendCmd("KEY", "B");

  lastA = a;
  lastB = b;
  delay(20);
}
```

### 3.4 场景 3：PS2 手柄 → 串口转发（控制网页游戏/动画）

```cpp
#include <Arduino.h>
#include <PS2X_lib.h>

#define PS2_CLK 41
#define PS2_CMD 9
#define PS2_CS  42
#define PS2_DAT 10

PS2X ps2x;

void sendCmd(const String &name, const String &value) {
  Serial.print(name);
  Serial.print(":");
  Serial.println(value);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  int err = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_CS, PS2_DAT, true, true);
  if (err != 0) {
    Serial.println("PS2_ERROR");  // 网页可据此提示"手柄未连接"
  } else {
    Serial.println("PS2_READY");
  }
}

void loop() {
  if (ps2x.Button(GamepadType)) {
    // 手柄未连接时跳过
    delay(100);
    return;
  }
  ps2x.read_gamepad();

  // 方向键 / 按钮
  if (ps2x.Button(PSB_PAD_UP))    sendCmd("BTN", "UP");
  if (ps2x.Button(PSB_PAD_DOWN))  sendCmd("BTN", "DOWN");
  if (ps2x.Button(PSB_PAD_LEFT))  sendCmd("BTN", "LEFT");
  if (ps2x.Button(PSB_PAD_RIGHT)) sendCmd("BTN", "RIGHT");
  if (ps2x.Button(PSB_CROSS))     sendCmd("BTN", "CROSS");
  if (ps2x.Button(PSB_CIRCLE))    sendCmd("BTN", "CIRCLE");
  if (ps2x.Button(PSB_SQUARE))    sendCmd("BTN", "SQUARE");
  if (ps2x.Button(PSB_TRIANGLE))  sendCmd("BTN", "TRIANGLE");

  // 摇杆（0-255，中心 128）
  sendCmd("LX", String(ps2x.Analog(PSS_LX)));
  sendCmd("LY", String(ps2x.Analog(PSS_LY)));
  sendCmd("RX", String(ps2x.Analog(PSS_RX)));
  sendCmd("RY", String(ps2x.Analog(PSS_RY)));

  delay(30);  // 控制发送频率，避免串口拥堵
}
```

> PS2 手柄库需要 `lib/PS2X_lib/`（ESP32 版），引脚 CLK=41, CMD=9, CS=42, DAT=10。

---

## 四、网页侧模板（HTML 单文件，Web Serial API）

> 这个 HTML 用 Chrome/Edge 打开即可。连上串口后，主板的每一行输出（`READY`、`KEY:A` 等）都会实时进入 `handleData()` 函数，你在里面写自己的网页效果逻辑。

```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>未来科技盒 · 串口控制台</title>
<style>
  body { font-family: 'Microsoft YaHei', sans-serif; max-width: 600px; margin: 40px auto; padding: 0 16px; }
  button { font-size: 16px; padding: 10px 20px; margin: 4px; cursor: pointer; }
  #status { margin: 12px 0; padding: 8px; border-radius: 4px; background: #f0f0f0; }
  #log { height: 200px; overflow-y: auto; background: #111; color: #0f0; padding: 8px; font-family: monospace; }
  #stage { height: 200px; border: 2px solid #333; border-radius: 8px; margin: 12px 0;
           display: flex; align-items: center; justify-content: center; font-size: 28px;
           transition: background-color 0.2s; }
</style>
</head>
<body>
<h2>🎮 未来科技盒 · 串口控制台</h2>

<button onclick="connectSerial()">🔌 连接串口</button>
<button onclick="disconnectSerial()">断开</button>
<span id="status">未连接</span>

<div id="stage">等待主板信号…</div>
<div id="log"></div>

<script>
let port = null;
let reader = null;
let writer = null;

// ---------- 连接 ----------
async function connectSerial() {
  try {
    port = await navigator.serial.requestPort();
    await port.open({ baudRate: 115200 });  // 必须与固件 Serial.begin() 一致
    document.getElementById('status').textContent = '已连接：' + port.getInfo().usbVendorId;
    document.getElementById('stage').textContent = '已连接！';
    readLoop();
  } catch (e) {
    alert('连接失败：' + e.message);
  }
}

// ---------- 断开 ----------
async function disconnectSerial() {
  if (reader) { await reader.cancel(); reader = null; }
  if (writer) { writer.releaseLock(); writer = null; }
  if (port) { await port.close(); port = null; }
  document.getElementById('status').textContent = '已断开';
}

// ---------- 读取循环 ----------
async function readLoop() {
  let buffer = '';
  const decoder = new TextDecoder();
  while (port && port.readable) {
    reader = port.readable.getReader();
    try {
      while (true) {
        const { value, done } = await reader.read();
        if (done) break;
        buffer += decoder.decode(value, { stream: true });
        // 按行处理（主板每条指令以 \n 结尾）
        let idx;
        while ((idx = buffer.indexOf('\n')) >= 0) {
          const line = buffer.slice(0, idx).trim();
          buffer = buffer.slice(idx + 1);
          if (line) handleData(line);
        }
      }
    } catch (e) {
      console.log('读取错误', e);
    } finally {
      reader.releaseLock();
    }
  }
}

// ---------- 发送（如需网页→主板） ----------
async function sendSerial(text) {
  if (!port || !port.writable) return;
  writer = port.writable.getWriter();
  await writer.write(new TextEncoder().encode(text + '\n'));
  writer.releaseLock();
}

// ---------- ⭐ 核心：处理主板发来的每一条指令 ----------
// 协议：普通文本行（如 "READY" "KEY:A" "KEY:B"）
//      或 "名称:值" 格式（如 "DIST:35" "LX:128"）
function handleData(line) {
  log(line);

  const [name, value] = line.split(':');

  switch (name) {
    case 'READY':
      document.getElementById('stage').textContent = '✅ 主板已就绪';
      break;

    case 'KEY':  // 按钮
      if (value === 'A') {
        document.getElementById('stage').textContent = '🅰️ KEY_A 按下！';
        document.getElementById('stage').style.background = '#ffe0e0';
      } else if (value === 'B') {
        document.getElementById('stage').textContent = '🅱️ KEY_B 按下！';
        document.getElementById('stage').style.background = '#e0e0ff';
      }
      setTimeout(() => document.getElementById('stage').style.background = '', 300);
      break;

    case 'PS2_ERROR':
      document.getElementById('stage').textContent = '⚠️ 手柄未连接';
      break;
    case 'PS2_READY':
      document.getElementById('stage').textContent = '🎮 手柄已就绪';
      break;
    case 'BTN':  // 手柄按钮 → 控制网页游戏/动画
      document.getElementById('stage').textContent = '🎮 ' + value;
      // 在这里写你的网页效果：按键上下左右控制元素移动等
      break;
    case 'LX': case 'LY': case 'RX': case 'RY':
      // 摇杆数据：实时更新网页元素位置
      break;
    default:
      document.getElementById('stage').textContent = line;
  }
}

// ---------- 日志 ----------
function log(msg) {
  const el = document.getElementById('log');
  el.textContent += msg + '\n';
  el.scrollTop = el.scrollHeight;
}
</script>
</body>
</html>
```

---

## 五、开发时启动本地服务（避免 file:// 限制）

Chrome 中 Web Serial API 需要 secure context。本地开发建议起一个 http 服务：

```bash
# 在 HTML 所在目录执行（Python 3）
python -m http.server 8000
# 浏览器访问 http://localhost:8000/你的文件.html
```

**不要**双击 file:// 直接打开（部分 Chrome 版本会禁用 Web Serial）。

---

## 六、注意事项与排查

| 问题 | 原因 | 解决 |
|------|------|------|
| `requestPort()` 没反应 | 非 Chrome/Edge，或不是 secure context | 换 Chrome，用 localhost/HTTPS 打开 |
| 连接成功但无数据 | 波特率不一致，或 3.0 缺 `ARDUINO_USB_CDC_ON_BOOT=0` | 检查 platformio.ini；统一 115200 |
| 数据乱码/丢字 | 串口拥堵 | 固件发送频率降低（如 delay 30ms+） |
| 连不上串口 | 串口被监视器/其他程序占用 | 关闭串口监视器等占用程序 |
| 设备管理器黄叹号 | CH343 驱动缺失（3.0） | 安装 WCH CH343SER 驱动 |
| 拔插后端口变化 | USB 重枚举 | 重新点"连接"，重新选端口 |

## 七、与 WiFi Web 遥控的区别

| 对比 | USB 串口→网页（本能力） | WiFi Web 遥控 |
|------|------------------------|---------------|
| 连接方式 | 主板 USB 线连电脑 | 主板开 WiFi 热点/连路由 |
| 对电脑网络影响 | **无** | 占用网络，可能断网 |
| 依赖 | Web Serial API（Chrome） | ESP32 内置 WebServer |
| 适用 | 近距离、单机、教学演示 | 远程、无线、移动控制 |
| 浏览器 | 仅 Chrome/Edge | 任何浏览器 |
