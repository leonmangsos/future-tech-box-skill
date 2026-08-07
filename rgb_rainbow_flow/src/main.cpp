/**
 * 未来科技盒 3.0 - RGB 流水彩虹灯 & 随机颜色流水灯
 * 
 * 硬件：ESP32-S3 + 9个 WS2812 RGB LED（GPIO33 单总线控制）
 * 
 * 效果说明：
 *   模式1：流水随机颜色 - 从第1个到第9个LED依次亮起随机颜色
 *   模式2：流水彩虹灯   - 从第1个到第9个LED依次亮起彩虹色，整体流动
 * 
 * 操作：
 *   按键A（GPIO21）：切换模式
 *   按键B（GPIO0） ：调整速度（快/中/慢三档）
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "pins.h"

// ==================== RGB LED 配置 ====================
#define BRIGHTNESS  30    // 亮度 (0-255)，降低亮度更护眼，建议不超过100避免电流过大

Adafruit_NeoPixel strip(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// ==================== 模式定义 ====================
enum EffectMode {
  MODE_RANDOM_WIPE = 0,   // 流水随机颜色
  MODE_RAINBOW_FLOW,      // 流水彩虹灯
  MODE_COUNT              // 模式总数
};

EffectMode currentMode = MODE_RANDOM_WIPE;

// ==================== 速度档位 ====================
const int SPEED_LEVELS[] = {50, 100, 200};  // 快/中/慢（毫秒）
const char* SPEED_NAMES[] = {"快速", "中速", "慢速"};
int currentSpeedLevel = 1;  // 默认中速
int animDelay = SPEED_LEVELS[1];

// ==================== 按键状态 ====================
bool lastKeyA = HIGH;
bool lastKeyB = HIGH;

// ==================== 工具函数 ====================

// 生成随机鲜艳颜色（使用HSV确保颜色鲜艳）
uint32_t randomVividColor() {
  uint16_t hue = random(0, 65536);  // 0-65535 全色相范围
  return strip.gamma32(strip.ColorHSV(hue, 255, 255));  // 饱和度和亮度拉满
}

// HSV 色轮颜色（0-255 映射到彩虹）
uint32_t wheelColor(uint8_t pos) {
  return strip.gamma32(strip.ColorHSV((uint16_t)pos * 256, 255, 255));
}

// ==================== 效果 1：流水随机颜色 ====================
// 从第1个到第9个LED依次亮起随机颜色，全亮后清除重来
void effectRandomWipe() {
  // 阶段1：依次点亮，每个LED随机颜色
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, randomVividColor());
    strip.show();
    delay(animDelay);
    
    // 检查按键
    if (digitalRead(KEY_A) == LOW || digitalRead(KEY_B) == LOW) return;
  }
  
  // 保持全亮一会儿
  delay(animDelay * 3);
  
  // 阶段2：依次熄灭
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0);
    strip.show();
    delay(animDelay / 2);
    
    if (digitalRead(KEY_A) == LOW || digitalRead(KEY_B) == LOW) return;
  }
  
  delay(animDelay);
}

// ==================== 效果 2：流水彩虹灯 ====================
// 彩虹色从第1个LED流向第9个LED，颜色持续变化
void effectRainbowFlow() {
  static uint16_t rainbowOffset = 0;
  
  // 每次刷新整个灯带，颜色偏移产生流动效果
  for (int cycle = 0; cycle < 256; cycle++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      // 每个LED的色相 = 基础偏移 + 在灯带中的位置偏移
      // i * 65536L / NUM_LEDS 让9个灯均匀覆盖一整圈彩虹
      uint16_t pixelHue = rainbowOffset + (i * 65536L / NUM_LEDS);
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    rainbowOffset += 256;  // 颜色整体往前推进
    
    delay(animDelay / 2);
    
    // 检查按键
    if (digitalRead(KEY_A) == LOW || digitalRead(KEY_B) == LOW) return;
  }
}

// ==================== 按键处理 ====================
void checkButtons() {
  // 按键A：切换模式
  bool currentKeyA = digitalRead(KEY_A);
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    delay(50);  // 消抖
    currentMode = (EffectMode)((currentMode + 1) % MODE_COUNT);
    
    // 切换时闪白色提示
    strip.clear();
    strip.show();
    delay(100);
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(255, 255, 255));
    }
    strip.show();
    delay(200);
    strip.clear();
    strip.show();
    delay(100);
    
    Serial.print("🔄 切换到模式: ");
    Serial.println(currentMode == MODE_RANDOM_WIPE ? "流水随机颜色" : "流水彩虹灯");
  }
  lastKeyA = currentKeyA;
  
  // 按键B：切换速度
  bool currentKeyB = digitalRead(KEY_B);
  if (lastKeyB == HIGH && currentKeyB == LOW) {
    delay(50);  // 消抖
    currentSpeedLevel = (currentSpeedLevel + 1) % 3;
    animDelay = SPEED_LEVELS[currentSpeedLevel];
    
    // 切换速度时所有灯闪一下当前颜色
    Serial.print("⚡ 速度切换: ");
    Serial.print(SPEED_NAMES[currentSpeedLevel]);
    Serial.print(" (");
    Serial.print(animDelay);
    Serial.println("ms)");
  }
  lastKeyB = currentKeyB;
}

// ==================== setup ====================
void setup() {
  Serial.begin(115200);
  delay(2000);  // 等待USB串口稳定
  
  Serial.println("╔══════════════════════════════════════╗");
  Serial.println("║  未来科技盒 3.0 - RGB 流水彩虹灯    ║");
  Serial.println("╠══════════════════════════════════════╣");
  Serial.println("║  LED: 9个 WS2812 RGB (GPIO33)       ║");
  Serial.println("║  按键A: 切换模式                     ║");
  Serial.println("║  按键B: 切换速度(快/中/慢)           ║");
  Serial.println("╚══════════════════════════════════════╝");
  Serial.println();
  
  // 初始化按键
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  
  // 初始化 RGB LED
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();
  
  // 开机动画：快速彩虹扫一遍
  Serial.println("🌈 开机动画...");
  for (int i = 0; i < NUM_LEDS; i++) {
    uint16_t hue = (i * 65536L) / NUM_LEDS;
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue)));
    strip.show();
    delay(80);
  }
  delay(500);
  
  // 渐灭
  for (int b = BRIGHTNESS; b >= 0; b -= 2) {
    strip.setBrightness(b);
    strip.show();
    delay(10);
  }
  strip.clear();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  delay(300);
  
  Serial.println("✅ 初始化完成！开始运行...");
  Serial.println("当前模式: 流水随机颜色 | 速度: 中速");
  Serial.println();
  
  randomSeed(analogRead(0));  // 用模拟引脚噪声作随机种子
}

// ==================== loop ====================
void loop() {
  // 检查按键
  checkButtons();
  
  // 根据当前模式运行效果
  switch (currentMode) {
    case MODE_RANDOM_WIPE:
      effectRandomWipe();
      break;
    case MODE_RAINBOW_FLOW:
      effectRainbowFlow();
      break;
    default:
      break;
  }
}
