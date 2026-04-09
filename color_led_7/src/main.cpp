#include <Arduino.h>
#include <Wire.h>
#include "veml6040.h"
#include "pins.h"

// ==================== VEML6040 颜色传感器 ====================
VEML6040 colorSensor;
bool sensorFound = false;

// ==================== LED 矩阵 ====================
const int ROW_PINS[3] = {LED_ROW1, LED_ROW2, LED_ROW3};
const int COL_PINS[3] = {LED_COL1, LED_COL2, LED_COL3};

int ledState[3][3] = {
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}
};

// ==================== 颜色枚举 ====================
enum ColorType {
  COLOR_NONE = 0,
  COLOR_RED,       // 红
  COLOR_ORANGE,    // 橙
  COLOR_YELLOW,    // 黄
  COLOR_GREEN,     // 绿
  COLOR_CYAN,      // 青
  COLOR_BLUE,      // 蓝
  COLOR_PURPLE     // 紫
};

const char* colorNames[] = {
  "未识别", "红色", "橙色", "黄色", "绿色", "青色", "蓝色", "紫色"
};

ColorType currentColor = COLOR_NONE;

// 非阻塞定时
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 400;

// ==================== 自适应基线 ====================
// 开机时自动校准环境光基线（RGB 占比）
float baseR = 0.40;  // 默认值，会被校准覆盖
float baseG = 0.39;
float baseB = 0.21;
bool calibrated = false;

// ==================== LED 控制函数 ====================
void setLED(int num, int state) {
  if (num < 1 || num > 9) return;
  int idx = num - 1;
  int row = 2 - (idx / 3);
  int col = idx % 3;
  ledState[row][col] = state;
}

void turnOffAllLEDs() {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      ledState[i][j] = 0;
}

void turnOnAllLEDs() {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      ledState[i][j] = 1;
}

void scanDisplay() {
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      digitalWrite(COL_PINS[col], HIGH);
    }
    digitalWrite(ROW_PINS[row], HIGH);
    for (int col = 0; col < 3; col++) {
      digitalWrite(COL_PINS[col], ledState[row][col] ? LOW : HIGH);
    }
    delayMicroseconds(1000);
    digitalWrite(ROW_PINS[row], LOW);
  }
}

// ==================== 开机校准基线 ====================
// 启动后采集 8 次环境光数据，取平均值作为基线
void calibrateBaseline() {
  Serial.println("🔄 正在校准环境光基线...");
  Serial.println("   （请确保传感器前没有彩色物体）");
  
  // 校准过程中 LED 全亮表示正在校准
  turnOnAllLEDs();
  
  float sumR = 0, sumG = 0, sumB = 0;
  int validCount = 0;
  
  // 先等 1 秒让传感器稳定
  unsigned long waitStart = millis();
  while (millis() - waitStart < 1000) {
    scanDisplay();
  }
  
  // 采集 8 次数据
  for (int i = 0; i < 8; i++) {
    // 等待积分周期
    unsigned long sampleStart = millis();
    while (millis() - sampleStart < 400) {
      scanDisplay();
    }
    
    uint16_t r = colorSensor.getRed();
    uint16_t g = colorSensor.getGreen();
    uint16_t b = colorSensor.getBlue();
    uint16_t w = colorSensor.getWhite();
    
    Serial.print("  校准采样 ");
    Serial.print(i + 1);
    Serial.print("/8: R=");
    Serial.print(r);
    Serial.print(" G=");
    Serial.print(g);
    Serial.print(" B=");
    Serial.print(b);
    Serial.print(" W=");
    Serial.println(w);
    
    if (w > 100 && r > 30 && g > 30 && b > 10) {
      float total = (float)r + g + b;
      sumR += (float)r / total;
      sumG += (float)g / total;
      sumB += (float)b / total;
      validCount++;
    }
  }
  
  if (validCount >= 3) {
    baseR = sumR / validCount;
    baseG = sumG / validCount;
    baseB = sumB / validCount;
    calibrated = true;
    
    Serial.print("✅ 校准完成！基线: R=");
    Serial.print(baseR, 4);
    Serial.print(" G=");
    Serial.print(baseG, 4);
    Serial.print(" B=");
    Serial.println(baseB, 4);
  } else {
    Serial.println("⚠️ 光线太暗，使用默认基线");
  }
  
  // 校准完成提示音
  tone(BUZZER_PIN, 2000, 80);
  delay(100);
  tone(BUZZER_PIN, 2500, 80);
  
  turnOffAllLEDs();
}

// ==================== 颜色识别算法（自适应基线版）====================
ColorType detectColor(uint16_t r, uint16_t g, uint16_t b, uint16_t w) {
  // 光线太暗无法识别
  if (w < 100) {
    return COLOR_NONE;
  }
  
  uint16_t maxVal = max(r, max(g, b));
  if (maxVal < 30) {
    return COLOR_NONE;
  }
  
  // 计算各通道占比
  float total = (float)r + g + b;
  float rr = (float)r / total;
  float gr = (float)g / total;
  float br = (float)b / total;
  
  // 相对于校准基线的偏离
  float rDev = rr - baseR;
  float gDev = gr - baseG;
  float bDev = br - baseB;
  
  // 饱和度 = 偏离幅度之和
  float saturation = fabs(rDev) + fabs(gDev) + fabs(bDev);
  
  // 调试输出
  Serial.print(" [rr=");
  Serial.print(rr, 3);
  Serial.print(" gr=");
  Serial.print(gr, 3);
  Serial.print(" br=");
  Serial.print(br, 3);
  Serial.print(" rD=");
  Serial.print(rDev, 4);
  Serial.print(" gD=");
  Serial.print(gDev, 4);
  Serial.print(" bD=");
  Serial.print(bDev, 4);
  Serial.print(" sat=");
  Serial.print(saturation, 4);
  Serial.print("]");
  
  // 饱和度太低 = 接近环境光，没有明显颜色
  if (saturation < 0.025) {
    return COLOR_NONE;
  }
  
  // ============================================================
  // 颜色判断策略：找出偏离最大的方向，再结合其他通道判断
  // ============================================================
  
  // 找出最大正偏离的通道
  float maxDev = max(rDev, max(gDev, bDev));
  
  // ---------- R 偏高为主 ----------
  if (rDev == maxDev && rDev > 0.005) {
    // R 通道占比升高，说明物体反射更多红光
    
    if (bDev > 0.005) {
      // R↑ B↑ → 紫色 (红+蓝混合)
      return COLOR_PURPLE;
    }
    
    // 看 G 的偏离程度来区分红/橙/黄
    // 红色: G 下降明显（物体只反射红光）
    // 橙色: G 下降一点（物体反射红光+少量绿光）
    // 黄色: G 也升高（物体反射红光+绿光）
    
    if (gDev > 0.003) {
      // G 也升高 → 黄色 (R+G 同时偏高，B 偏低)
      return COLOR_YELLOW;
    }
    if (gDev > -0.01) {
      // G 接近基线或微降 → 橙色
      return COLOR_ORANGE;
    }
    // G 明显下降 → 红色
    return COLOR_RED;
  }
  
  // ---------- G 偏高为主 ----------
  if (gDev == maxDev && gDev > 0.005) {
    // G 通道占比升高
    
    if (bDev > 0.003) {
      // G↑ B↑ → 青色 (绿+蓝混合)
      return COLOR_CYAN;
    }
    if (rDev > 0.003) {
      // G↑ R↑ → 黄色 (绿+红混合)
      return COLOR_YELLOW;
    }
    // 纯 G 升高 → 绿色
    return COLOR_GREEN;
  }
  
  // ---------- B 偏高为主 ----------
  if (bDev == maxDev && bDev > 0.005) {
    // B 通道占比升高
    
    if (rDev > 0.003) {
      // B↑ R↑ → 紫色
      return COLOR_PURPLE;
    }
    if (gDev > 0.003) {
      // B↑ G↑ → 青色
      return COLOR_CYAN;
    }
    // 纯 B 升高 → 蓝色
    return COLOR_BLUE;
  }
  
  // ---------- 负偏离为主（某通道降低）----------
  // 这种情况也能判断颜色：
  // B 大幅降低 + R/G 升高 → 黄色
  // G 大幅降低 + R 升高 → 红色
  // R 大幅降低 + G 升高 → 绿色
  
  float minDev = min(rDev, min(gDev, bDev));
  
  if (bDev == minDev && bDev < -0.008) {
    // B 通道降低最多
    if (rDev > gDev && rDev > 0.003) return COLOR_ORANGE;
    if (gDev > rDev && gDev > 0.003) return COLOR_GREEN;
    if (rDev > 0.0 && gDev > 0.0) return COLOR_YELLOW;
    return COLOR_ORANGE;
  }
  
  if (gDev == minDev && gDev < -0.008) {
    // G 通道降低最多
    if (rDev > bDev) return COLOR_RED;
    if (bDev > rDev) return COLOR_BLUE;
    return COLOR_PURPLE;
  }
  
  if (rDev == minDev && rDev < -0.008) {
    // R 通道降低最多
    if (gDev > bDev) return COLOR_GREEN;
    if (bDev > gDev) return COLOR_BLUE;
    return COLOR_CYAN;
  }
  
  return COLOR_NONE;
}

// ==================== 根据颜色点亮对应 LED ====================
void updateLEDsForColor(ColorType color) {
  turnOffAllLEDs();
  if (color >= COLOR_RED && color <= COLOR_PURPLE) {
    setLED((int)color, 1);
  }
}

// ==================== I2C 扫描 ====================
void scanI2C() {
  Serial.println("正在扫描 I2C 设备...");
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  找到设备: 0x");
      Serial.println(addr, HEX);
      found++;
    }
  }
  if (found == 0) {
    Serial.println("  未找到任何 I2C 设备！");
  } else {
    Serial.print("  共找到 ");
    Serial.print(found);
    Serial.println(" 个设备");
  }
}

// ==================== setup ====================
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("===================================");
  Serial.println("  颜色传感器 → 7 色彩虹 LED");
  Serial.println("  VEML6040 + 自适应基线校准");
  Serial.println("===================================");
  
  // 初始化 LED 矩阵引脚
  for (int i = 0; i < 3; i++) {
    pinMode(ROW_PINS[i], OUTPUT);
    pinMode(COL_PINS[i], OUTPUT);
    digitalWrite(ROW_PINS[i], LOW);
    digitalWrite(COL_PINS[i], HIGH);
  }
  
  // 蜂鸣器
  pinMode(BUZZER_PIN, OUTPUT);
  
  // 初始化 I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  scanI2C();
  
  // 初始化 VEML6040
  if (colorSensor.begin()) {
    sensorFound = true;
    colorSensor.setConfiguration(VEML6040_IT_320MS + VEML6040_AF_AUTO + VEML6040_SD_ENABLE);
    Serial.println("✅ VEML6040 颜色传感器初始化成功！");
    
    tone(BUZZER_PIN, 1000, 100);
    delay(150);
    tone(BUZZER_PIN, 1500, 100);
    delay(200);
    
    // 开机自动校准环境光基线
    calibrateBaseline();
    
  } else {
    sensorFound = false;
    Serial.println("❌ VEML6040 未检测到，进入演示模式");
    tone(BUZZER_PIN, 300, 500);
  }
  
  Serial.println();
  Serial.println("颜色 → LED 对应关系：");
  Serial.println("  LED 1 (左上) = 红色");
  Serial.println("  LED 2 (中上) = 橙色");
  Serial.println("  LED 3 (右上) = 黄色");
  Serial.println("  LED 4 (左中) = 绿色");
  Serial.println("  LED 5 (正中) = 青色");
  Serial.println("  LED 6 (右中) = 蓝色");
  Serial.println("  LED 7 (左下) = 紫色");
  Serial.println();
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("⚠️  颜色传感器使用注意事项：");
  Serial.println("  1. 请在相对封闭、光线稳定的环境下使用");
  Serial.println("  2. 避免强光直射（阳光、日光灯直射）");
  Serial.println("  3. 将彩色物体靠近传感器（1-3cm 最佳）");
  Serial.println("  4. 建议使用纯色、高饱和度的物体");
  Serial.println("  5. 如果更换环境（开/关灯），请按 RST 重新校准");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println();
}

// ==================== 演示模式 ====================
int demoColorIndex = 0;
unsigned long lastDemoTime = 0;
const unsigned long DEMO_INTERVAL = 1000;

void runDemoMode() {
  unsigned long now = millis();
  if (now - lastDemoTime >= DEMO_INTERVAL) {
    lastDemoTime = now;
    demoColorIndex++;
    if (demoColorIndex > 7) demoColorIndex = 1;
    
    turnOffAllLEDs();
    setLED(demoColorIndex, 1);
    
    Serial.print("演示模式 → ");
    Serial.println(colorNames[demoColorIndex]);
  }
}

// ==================== loop ====================
void loop() {
  if (!sensorFound) {
    runDemoMode();
    scanDisplay();
    return;
  }
  
  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL) {
    lastReadTime = now;
    
    uint16_t r = colorSensor.getRed();
    uint16_t g = colorSensor.getGreen();
    uint16_t b = colorSensor.getBlue();
    uint16_t w = colorSensor.getWhite();
    
    // 串口输出原始值
    Serial.print("R:");
    Serial.print(r);
    Serial.print(" G:");
    Serial.print(g);
    Serial.print(" B:");
    Serial.print(b);
    Serial.print(" W:");
    Serial.print(w);
    
    // 识别颜色
    ColorType detected = detectColor(r, g, b, w);
    
    Serial.print(" → ");
    Serial.print(colorNames[(int)detected]);
    if (detected != COLOR_NONE) {
      Serial.print(" (LED ");
      Serial.print((int)detected);
      Serial.print(")");
    }
    Serial.println();
    
    // 颜色变化时更新 LED
    if (detected != currentColor) {
      currentColor = detected;
      updateLEDsForColor(currentColor);
      
      if (currentColor != COLOR_NONE) {
        tone(BUZZER_PIN, 800 + (int)currentColor * 100, 50);
      }
    }
  }
  
  scanDisplay();
}
