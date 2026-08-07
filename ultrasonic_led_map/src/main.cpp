/*
 * 未来科技盒 3.0 - 超声波距离映射 RGB LED
 * 
 * 功能说明：
 *   超声波传感器检测 0~100cm 范围内的距离，映射到 9 个 RGB LED 上：
 *   - 约 100cm 时 → 9 颗灯全亮（最远，绿色）
 *   - 约 50cm 时  → 5 颗灯亮（中间，黄色过渡）
 *   - 约 11cm 时  → 1 颗灯亮（最近，红色）
 *   - 靠得越近，亮的灯越少
 *   - 超出 100cm → 全灭
 * 
 *   额外效果：灯光颜色从绿(远)→黄(中)→红(近)渐变，直观显示距离
 * 
 * 硬件连接：
 *   - 超声波传感器 → 接口2 (GPIO7 - Grove 单总线 SIG 模式)
 *   - RGB LED 矩阵 → GPIO33 (WS2812 × 9)
 * 
 * 硬件版本：未来科技盒 3.0 (ESP32-S3 QFN56 + CH343 USB转串口)
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "pins.h"

// ============================================
// RGB LED 配置
// ============================================
#define BRIGHTNESS  40  // 亮度 (0-255)，建议20-50避免刺眼

Adafruit_NeoPixel strip(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// ============================================
// 超声波配置
// ============================================
const float MAX_DISTANCE = 100.0;  // 最大映射距离 (cm)
const float MIN_DISTANCE = 2.0;    // 最小有效距离 (cm)

// 平滑滤波
const int FILTER_SIZE = 5;         // 中值滤波采样数
float filterBuffer[FILTER_SIZE];
int filterIndex = 0;
int filterCount = 0;               // 当前缓冲区中有效数据个数

// 无效测量连续计数（用于判断是否真的超出范围）
int invalidCount = 0;
const int INVALID_THRESHOLD = 5;   // 连续5次无效才判定为超出范围

// ============================================
// 超声波测距函数 (Grove 单总线 SIG 模式)
// ============================================
// 单线超声波：只使用 GPIO7 一个引脚，既做 Trig 又做 Echo
// 时序：先输出 10μs 高电平触发，然后切换为输入读取回波脉宽
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
  long duration = pulseIn(ULTRASONIC_PIN, HIGH, 30000);  // 超时 30ms（约 5m）
  
  // Step 3: 计算距离
  if (duration == 0) {
    return -1;  // 超时，无有效回波
  }
  
  float distance = duration / 58.0;  // 标准公式：距离(cm) = 脉冲时间(μs) / 58
  
  // 有效范围检查
  if (distance < MIN_DISTANCE || distance > 400) {
    return -1;
  }
  
  return distance;
}

// ============================================
// 中值滤波 - 消除超声波偶发跳变
// ============================================
float medianFilter(float newValue) {
  filterBuffer[filterIndex] = newValue;
  filterIndex = (filterIndex + 1) % FILTER_SIZE;
  if (filterCount < FILTER_SIZE) filterCount++;
  
  // 复制到临时数组排序
  float sorted[FILTER_SIZE];
  for (int i = 0; i < filterCount; i++) {
    sorted[i] = filterBuffer[i];
  }
  
  // 简单冒泡排序（数组很小，开销可忽略）
  for (int i = 0; i < filterCount - 1; i++) {
    for (int j = 0; j < filterCount - 1 - i; j++) {
      if (sorted[j] > sorted[j + 1]) {
        float tmp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = tmp;
      }
    }
  }
  
  return sorted[filterCount / 2];  // 取中值
}

// ============================================
// 测距 + 滤波
// ============================================
float measureDistance() {
  float raw = readUltrasonic();
  
  if (raw < 0) {
    invalidCount++;
    // 连续多次无效才返回 -1，偶发无效则沿用上次有效值
    if (invalidCount >= INVALID_THRESHOLD) {
      return -1;
    }
    return -2;  // 表示"偶发无效，不更新显示"
  }
  
  // 有效测量，重置无效计数
  invalidCount = 0;
  return medianFilter(raw);
}

// ============================================
// 距离映射到 LED 数量 (0~9)
// ============================================
// 100cm → 9灯, 靠近 → 灯变少, 超出100cm → 0灯
int distanceToLEDCount(float distance) {
  if (distance < 0) return 0;            // 无效测量
  if (distance > MAX_DISTANCE) return 0;  // 超出范围
  
  // 将 MIN_DISTANCE~100cm 线性映射到 1~9 颗灯
  // distance ≈ 2cm   → 1颗 (最近，至少亮1颗)
  // distance = 100cm  → 9颗 (最远)
  int count = (int)((distance / MAX_DISTANCE) * 8.0) + 1;
  
  // 限制范围
  if (count < 1) count = 1;
  if (count > 9) count = 9;
  
  return count;
}

// ============================================
// 根据距离计算灯光颜色 (绿→黄→红 渐变)
// ============================================
// 远(100cm) = 绿色, 中(50cm) = 黄色, 近(0cm) = 红色
uint32_t getDistanceColor(float distance) {
  if (distance < 0) distance = 0;
  if (distance > MAX_DISTANCE) distance = MAX_DISTANCE;
  
  // 归一化到 0.0~1.0 (0=最近, 1=最远)
  float ratio = distance / MAX_DISTANCE;
  
  uint8_t r, g;
  
  if (ratio < 0.5) {
    // 近距离: 红 → 黄 (红满，绿从0增到255)
    r = 255;
    g = (uint8_t)(ratio * 2.0 * 255);
  } else {
    // 远距离: 黄 → 绿 (红从255减到0，绿满)
    r = (uint8_t)((1.0 - ratio) * 2.0 * 255);
    g = 255;
  }
  
  return strip.Color(r, g, 0);
}

// ============================================
// 更新 LED 显示
// ============================================
void updateLEDs(int count, float distance) {
  uint32_t color = getDistanceColor(distance);
  
  strip.clear();
  
  // 从 LED0 开始，点亮 count 颗
  for (int i = 0; i < count; i++) {
    strip.setPixelColor(i, color);
  }
  
  strip.show();
}

// ============================================
// 非阻塞定时变量
// ============================================
unsigned long lastMeasureTime = 0;
const unsigned long MEASURE_INTERVAL = 100;  // 每100ms测量一次（超声波推荐间隔≥100ms）

float currentDistance = -1;
float lastValidDistance = 50.0;  // 上次有效距离，用于颜色计算
int currentLEDCount = 0;

// ============================================
// 开机自检动画
// ============================================
void startupAnimation() {
  // 逐个点亮（彩虹扫描效果）
  for (int i = 0; i < NUM_LEDS; i++) {
    uint16_t hue = (i * 65536L / NUM_LEDS);
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue, 255, 255)));
    strip.show();
    delay(80);
  }
  delay(400);
  
  // 全部闪烁一下绿色表示就绪
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0));
  }
  strip.show();
  delay(300);
  
  // 逐个熄灭
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    strip.setPixelColor(i, 0);
    strip.show();
    delay(60);
  }
  delay(200);
}

// ============================================
// setup
// ============================================
void setup() {
  Serial.begin(115200);
  delay(2000);  // ESP32-S3 USB 枚举等待（CH343 也需要时间稳定）
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  未来科技盒 3.0 - 超声波距离映射RGB LED");
  Serial.println("========================================");
  Serial.println("硬件版本: 3.0 (ESP32-S3 + CH343)");
  Serial.println("超声波接口: 接口2 (GPIO7, Grove单总线SIG)");
  Serial.println("RGB LED:    GPIO33 (WS2812 x9)");
  Serial.println();
  Serial.println("映射规则 (越远灯越多, 颜色渐变):");
  Serial.println("  ~11cm  → 1颗灯 (红色-危险)");
  Serial.println("  ~22cm  → 2颗灯");
  Serial.println("  ~33cm  → 3颗灯");
  Serial.println("  ~44cm  → 4颗灯");
  Serial.println("  ~55cm  → 5颗灯 (黄色-注意)");
  Serial.println("  ~66cm  → 6颗灯");
  Serial.println("  ~77cm  → 7颗灯");
  Serial.println("  ~88cm  → 8颗灯");
  Serial.println("  ~100cm → 9颗灯 (绿色-安全)");
  Serial.println("  >100cm → 全灭 (超出范围)");
  Serial.println("========================================");
  
  // 初始化 RGB LED
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();  // 初始全灭
  
  // 开机自检动画
  startupAnimation();
  
  // 初始化滤波缓冲区
  for (int i = 0; i < FILTER_SIZE; i++) {
    filterBuffer[i] = 0;
  }
  
  // 预热：丢弃前几次不稳定的测量
  Serial.println("超声波预热中...");
  for (int i = 0; i < 5; i++) {
    readUltrasonic();
    delay(100);
  }
  
  Serial.println("初始化完成，开始测距...");
  Serial.println();
}

// ============================================
// loop
// ============================================
void loop() {
  unsigned long now = millis();
  
  // 非阻塞定时测量（100ms 间隔，超声波推荐值）
  if (now - lastMeasureTime >= MEASURE_INTERVAL) {
    lastMeasureTime = now;
    
    // 测量距离（含中值滤波）
    currentDistance = measureDistance();
    
    // -2 表示偶发无效，跳过本次不更新显示
    if (currentDistance == -2) {
      Serial.println("距离: ... (偶发无效，保持当前显示)");
      return;
    }
    
    // 映射到 LED 数量
    currentLEDCount = distanceToLEDCount(currentDistance);
    
    // 记录有效距离（用于颜色计算）
    if (currentDistance > 0) {
      lastValidDistance = currentDistance;
    }
    
    // ★ 每次都更新 LED 显示（即使数量相同，颜色可能变化）
    updateLEDs(currentLEDCount, currentDistance > 0 ? currentDistance : 0);
    
    // 串口输出
    if (currentDistance > 0) {
      Serial.printf("距离: %6.1f cm → LED: %d 颗", currentDistance, currentLEDCount);
      
      // 可视化进度条
      Serial.print("  [");
      for (int i = 0; i < 9; i++) {
        if (i < currentLEDCount) {
          Serial.print("●");
        } else {
          Serial.print("○");
        }
      }
      Serial.println("]");
    } else {
      Serial.println("距离: --- (超出范围/无回波) → LED: 0 颗  [○○○○○○○○○]");
    }
  }
}
