/*
 * 未来科技盒 3.0 - 按键控制 RGB 灯光
 * 
 * 功能说明：
 *   按键 A：开/关灯切换，每次开灯时 9 个 RGB 灯随机分配不同颜色
 *   按键 B：切换亮度档位（共 5 档：20% → 40% → 60% → 80% → 100% → 循环）
 * 
 * 硬件：
 *   - 3×3 WS2812 RGB LED 矩阵 (GPIO33)
 *   - 按键A (GPIO21), 按键B (GPIO0)
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "pins.h"

// ============================================
// 亮度档位配置
// ============================================
const uint8_t BRIGHTNESS_LEVELS[] = {50, 100, 150, 200, 255};  // 5档亮度
const int NUM_BRIGHTNESS_LEVELS = sizeof(BRIGHTNESS_LEVELS) / sizeof(BRIGHTNESS_LEVELS[0]);
int currentBrightnessIndex = 0;  // 当前亮度档位索引

// ============================================
// 灯光状态
// ============================================
bool ledsOn = false;  // 灯光开关状态

// ============================================
// 预定义鲜艳颜色（用于随机选择）
// ============================================
struct Color {
  uint8_t r, g, b;
};

const Color VIVID_COLORS[] = {
  {255,   0,   0},  // 红
  {0,   255,   0},  // 绿
  {0,     0, 255},  // 蓝
  {255, 255,   0},  // 黄
  {0,   255, 255},  // 青
  {255,   0, 255},  // 品红
  {255, 128,   0},  // 橙
  {128,   0, 255},  // 紫
  {255, 255, 255},  // 白
  {0,   255, 128},  // 春绿
  {255,   0, 128},  // 玫红
  {128, 255,   0},  // 黄绿
};
const int NUM_COLORS = sizeof(VIVID_COLORS) / sizeof(VIVID_COLORS[0]);

// ============================================
// NeoPixel 对象
// ============================================
Adafruit_NeoPixel strip(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// ============================================
// 按键消抖相关
// ============================================
bool lastKeyA = HIGH;
bool lastKeyB = HIGH;
unsigned long lastKeyATime = 0;
unsigned long lastKeyBTime = 0;
const unsigned long DEBOUNCE_MS = 200;  // 消抖时间

// ============================================
// 为每个LED分配随机颜色并点亮
// ============================================
void randomizeAndShowLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    int colorIdx = random(NUM_COLORS);
    strip.setPixelColor(i, strip.Color(
      VIVID_COLORS[colorIdx].r,
      VIVID_COLORS[colorIdx].g,
      VIVID_COLORS[colorIdx].b
    ));
  }
  strip.show();
}

// ============================================
// 关闭所有灯
// ============================================
void turnOffLEDs() {
  strip.clear();
  strip.show();
}

// ============================================
// 按键A处理：切换灯光开关，开灯时随机颜色
// ============================================
void onKeyAPressed() {
  ledsOn = !ledsOn;
  
  if (ledsOn) {
    randomizeAndShowLEDs();
    Serial.println("[KEY A] 灯光开启 - 随机颜色");
  } else {
    turnOffLEDs();
    Serial.println("[KEY A] 灯光关闭");
  }
}

// ============================================
// 按键B处理：切换亮度档位
// ============================================
void onKeyBPressed() {
  currentBrightnessIndex = (currentBrightnessIndex + 1) % NUM_BRIGHTNESS_LEVELS;
  uint8_t newBrightness = BRIGHTNESS_LEVELS[currentBrightnessIndex];
  strip.setBrightness(newBrightness);
  
  // 如果灯是亮的，立即刷新显示以体现新亮度
  if (ledsOn) {
    strip.show();
  }
  
  int percent = (newBrightness * 100) / 255;
  Serial.printf("[KEY B] 亮度档位 %d/%d (亮度值: %d, 约 %d%%)\n",
    currentBrightnessIndex + 1, NUM_BRIGHTNESS_LEVELS, newBrightness, percent);
}

// ============================================
// setup
// ============================================
void setup() {
  Serial.begin(115200);
  delay(2000);  // ESP32-S3 启动等待
  
  Serial.println("========================================");
  Serial.println("  未来科技盒 3.0 - 按键控制RGB灯光");
  Serial.println("  KEY_A: 开/关灯 (随机颜色)");
  Serial.println("  KEY_B: 调节亮度 (5档循环)");
  Serial.println("========================================");
  
  // 初始化按键（内部上拉）
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  
  // 初始化 RGB LED
  strip.begin();
  strip.setBrightness(BRIGHTNESS_LEVELS[currentBrightnessIndex]);
  strip.show();  // 初始全灭
  
  // 初始化随机种子
  randomSeed(analogRead(0) ^ (millis() << 16));
  
  Serial.println("初始化完成，等待按键操作...");
}

// ============================================
// loop
// ============================================
void loop() {
  bool currentKeyA = digitalRead(KEY_A);
  bool currentKeyB = digitalRead(KEY_B);
  unsigned long now = millis();
  
  // 按键A 边沿检测 + 消抖
  if (lastKeyA == HIGH && currentKeyA == LOW && (now - lastKeyATime > DEBOUNCE_MS)) {
    delay(50);  // 3.0 可安全使用 delay 消抖
    if (digitalRead(KEY_A) == LOW) {
      onKeyAPressed();
      lastKeyATime = now;
    }
  }
  lastKeyA = currentKeyA;
  
  // 按键B 边沿检测 + 消抖
  if (lastKeyB == HIGH && currentKeyB == LOW && (now - lastKeyBTime > DEBOUNCE_MS)) {
    delay(50);
    if (digitalRead(KEY_B) == LOW) {
      onKeyBPressed();
      lastKeyBTime = now;
    }
  }
  lastKeyB = currentKeyB;
}
