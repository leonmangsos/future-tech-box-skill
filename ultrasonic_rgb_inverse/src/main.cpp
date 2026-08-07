#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "pins.h"

// ===== 配置参数 =====
#define BRIGHTNESS     30       // LED 亮度（建议 20-50）
#define MIN_DISTANCE   2.0      // 最小有效距离 cm
#define MAX_DISTANCE   100.0    // 最大映射距离 cm
#define MEDIAN_SAMPLES 5        // 中值滤波采样次数
#define UPDATE_INTERVAL 100     // 测量间隔 ms

Adafruit_NeoPixel strip(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// ===== 超声波测距（单线 SIG 模式）=====
float readUltrasonic() {
  pinMode(ULTRASONIC_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_PIN, LOW);

  pinMode(ULTRASONIC_PIN, INPUT);
  long duration = pulseIn(ULTRASONIC_PIN, HIGH, 30000);

  if (duration == 0) return -1;
  return duration / 58.0;
}

// ===== 中值滤波 =====
float getMedianDistance() {
  float samples[MEDIAN_SAMPLES];
  int validCount = 0;

  for (int i = 0; i < MEDIAN_SAMPLES; i++) {
    float d = readUltrasonic();
    if (d > 0) {
      samples[validCount++] = d;
    }
    delay(10);
  }

  if (validCount == 0) return -1;

  // 冒泡排序
  for (int i = 0; i < validCount - 1; i++) {
    for (int j = i + 1; j < validCount; j++) {
      if (samples[i] > samples[j]) {
        float tmp = samples[i];
        samples[i] = samples[j];
        samples[j] = tmp;
      }
    }
  }

  return samples[validCount / 2];
}

// ===== 距离映射到颜色 =====
// 近(红) -> 中(黄/绿) -> 远(蓝) 的渐变
uint32_t distanceToColor(float distance) {
  float ratio = constrain((distance - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);

  uint8_t r, g, b;

  if (ratio < 0.5) {
    // 近 → 中：红 → 黄 → 绿
    float t = ratio * 2.0;
    r = 255 * (1.0 - t);
    g = 255 * t;
    b = 0;
  } else {
    // 中 → 远：绿 → 青 → 蓝
    float t = (ratio - 0.5) * 2.0;
    r = 0;
    g = 255 * (1.0 - t);
    b = 255 * t;
  }

  return strip.Color(r, g, b);
}

// ===== 距离映射到点亮的 LED 数量（反向）=====
// 近距离点亮全部 LED，远距离只点亮 1 个 LED
int distanceToLEDCount(float distance) {
  float ratio = constrain((distance - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);
  // ratio=0（最近）→ NUM_LEDS 个灯；ratio=1（最远）→ 1 个灯
  return map(ratio * 100, 0, 100, NUM_LEDS, 1);
}

// ===== 显示函数 =====
void displayDistance(float distance) {
  if (distance < 0) {
    // 读取失败，LED 闪烁紫色
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(128, 0, 128));
    }
    strip.show();
    return;
  }

  uint32_t color = distanceToColor(distance);
  int litCount = distanceToLEDCount(distance);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < litCount) {
      strip.setPixelColor(i, color);
    } else {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
}

// ===== 初始化 =====
void setup() {
  Serial.begin(115200);
  delay(2000);

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();

  Serial.println("超声波距离 → RGB LED 反向映射");
  Serial.println("传感器接口: 接口3 (GPIO3)");
  Serial.println("距离范围: 2cm ~ 100cm");
  Serial.println("颜色映射: 红(近) → 绿(中) → 蓝(远)");
  Serial.println("LED数量: 近距离满灯, 远距离1个灯");
  Serial.println("-------------------------------");
}

// ===== 主循环 =====
void loop() {
  float distance = getMedianDistance();

  displayDistance(distance);

  if (distance > 0) {
    Serial.printf("距离: %.1f cm | LED: %d/%d\n", distance, distanceToLEDCount(distance), NUM_LEDS);
  } else {
    Serial.println("距离: 读取失败!");
  }

  delay(UPDATE_INTERVAL);
}
