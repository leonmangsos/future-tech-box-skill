/**
 * 未来科技盒 3.0 - RGB 骰子
 * 
 * 硬件：ESP32-S3 + 9个 WS2812 RGB LED（GPIO33 单总线控制）
 * 
 * 功能说明：
 *   按键A（GPIO21）：掷骰子 - 随机数字快速闪烁，由快到慢逐渐停下
 *   按键B（GPIO0） ：切换骰子点数的颜色（多种颜色循环）
 * 
 * 骰子布局（3×3 LED 矩阵）：
 *   LED0  LED1  LED2
 *   LED3  LED4  LED5
 *   LED6  LED7  LED8
 * 
 *   1 = 中心          2 = 对角线          3 = 对角线+中心
 *   . . .              ● . .              ● . .
 *   . ● .              . . .              . ● .
 *   . . .              . . ●              . . ●
 * 
 *   4 = 四角           5 = 四角+中心       6 = 两列各三
 *   ● . ●              ● . ●              ● . ●
 *   . . .              . ● .              ● . ●
 *   ● . ●              ● . ●              ● . ●
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "pins.h"

// ==================== 配置 ====================
#define BRIGHTNESS  20    // 亮度调低，避免晃眼（0-255）

Adafruit_NeoPixel strip(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// ==================== 骰子点数映射 ====================
// 每个数字需要点亮的LED索引
// 布局: LED0=左上, LED1=上中, LED2=右上
//       LED3=中左, LED4=正中, LED5=中右
//       LED6=左下, LED7=下中, LED8=右下

const int DICE_1[] = {4, -1};                         // 中心
const int DICE_2[] = {0, 8, -1};                      // 左上 + 右下
const int DICE_3[] = {0, 4, 8, -1};                   // 左上 + 中心 + 右下
const int DICE_4[] = {0, 2, 6, 8, -1};                // 四角
const int DICE_5[] = {0, 2, 4, 6, 8, -1};             // 四角 + 中心
const int DICE_6[] = {0, 2, 3, 5, 6, 8, -1};          // 左三 + 右三

const int* DICE_PATTERNS[] = {DICE_1, DICE_2, DICE_3, DICE_4, DICE_5, DICE_6};

// ==================== 颜色方案 ====================
// 每种颜色方案包含 {R, G, B}
const uint8_t COLORS[][3] = {
  {255,  60,  10},   // 暖橙色（默认）
  {  0, 180, 255},   // 天蓝色
  {  0, 255,  80},   // 翠绿色
  {255,   0, 100},   // 玫红色
  {180,   0, 255},   // 紫色
  {255, 255,   0},   // 黄色
  {255, 255, 255},   // 白色
};
const int NUM_COLORS = sizeof(COLORS) / sizeof(COLORS[0]);
int currentColorIndex = 0;

// ==================== 状态变量 ====================
bool lastKeyA = HIGH;
bool lastKeyB = HIGH;
bool isRolling = false;    // 是否正在掷骰子
int currentDice = 1;       // 当前显示的骰子数字 (1-6)

// ==================== 工具函数 ====================

// 获取当前颜色
uint32_t getDiceColor() {
  return strip.Color(
    COLORS[currentColorIndex][0],
    COLORS[currentColorIndex][1],
    COLORS[currentColorIndex][2]
  );
}

// 获取暗一点的背景色（当前颜色的微弱版本，用于未选中的LED）
uint32_t getDimColor() {
  return strip.Color(
    COLORS[currentColorIndex][0] / 30,
    COLORS[currentColorIndex][1] / 30,
    COLORS[currentColorIndex][2] / 30
  );
}

// 清除所有LED
void clearAll() {
  strip.clear();
  strip.show();
}

// 显示骰子数字 (1-6)
void showDice(int number) {
  if (number < 1 || number > 6) return;
  
  // 先全部设为极暗的背景色
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, getDimColor());
  }
  
  // 点亮对应的点
  const int* pattern = DICE_PATTERNS[number - 1];
  uint32_t color = getDiceColor();
  
  for (int i = 0; pattern[i] != -1; i++) {
    strip.setPixelColor(pattern[i], color);
  }
  
  strip.show();
}

// ==================== 掷骰子动画 ====================
// 由快到慢，逐渐停在最终数字
void rollDice() {
  isRolling = true;
  
  // 先来一个全闪提示开始
  uint32_t color = getDiceColor();
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
  delay(150);
  clearAll();
  delay(100);
  
  // 决定最终结果
  int finalNumber = random(1, 7);  // 1-6
  
  // 滚动动画：总共滚约 18-25 步，速度从快到慢
  int totalSteps = random(18, 26);
  
  for (int step = 0; step < totalSteps; step++) {
    // 随机选一个骰子数字显示
    int displayNum;
    if (step < totalSteps - 1) {
      // 还没到最后一步，随机选
      displayNum = random(1, 7);
      // 避免连续显示相同数字（看起来更有动感）
      while (displayNum == currentDice && step > 0) {
        displayNum = random(1, 7);
      }
    } else {
      // 最后一步，显示最终结果
      displayNum = finalNumber;
    }
    
    currentDice = displayNum;
    showDice(displayNum);
    
    // 计算当前延时：从快到慢
    // 使用指数曲线让减速更自然
    // 起始约 50ms，结束约 400ms
    float progress = (float)step / (float)(totalSteps - 1);
    // 用三次方曲线让前面快后面慢
    float curve = progress * progress * progress;
    int delayTime = 50 + (int)(curve * 400);
    
    delay(delayTime);
  }
  
  // 停下后闪烁 2 次确认最终结果
  delay(200);
  for (int blink = 0; blink < 2; blink++) {
    clearAll();
    delay(150);
    showDice(finalNumber);
    delay(250);
  }
  
  currentDice = finalNumber;
  isRolling = false;
  
  Serial.print("🎲 骰子结果: ");
  Serial.println(finalNumber);
}

// ==================== 切换颜色 ====================
void switchColor() {
  currentColorIndex = (currentColorIndex + 1) % NUM_COLORS;
  
  // 切换时来个小动画：从中心向外扩散新颜色
  clearAll();
  delay(50);
  
  // 先亮中心
  strip.setPixelColor(4, getDiceColor());
  strip.show();
  delay(80);
  
  // 再亮十字
  int cross[] = {1, 3, 5, 7};
  for (int i = 0; i < 4; i++) {
    strip.setPixelColor(cross[i], getDiceColor());
  }
  strip.show();
  delay(80);
  
  // 最后亮四角
  int corners[] = {0, 2, 6, 8};
  for (int i = 0; i < 4; i++) {
    strip.setPixelColor(corners[i], getDiceColor());
  }
  strip.show();
  delay(200);
  
  // 然后恢复显示当前骰子
  showDice(currentDice);
  
  Serial.print("🎨 颜色切换: #");
  Serial.println(currentColorIndex + 1);
}

// ==================== setup ====================
void setup() {
  Serial.begin(115200);
  delay(2000);  // 等待USB串口稳定
  
  Serial.println("╔══════════════════════════════════════╗");
  Serial.println("║   未来科技盒 3.0 - RGB 骰子 🎲      ║");
  Serial.println("╠══════════════════════════════════════╣");
  Serial.println("║  按键A: 掷骰子（由快到慢随机停下）    ║");
  Serial.println("║  按键B: 切换骰子颜色                  ║");
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
  
  // 随机种子
  randomSeed(analogRead(0) ^ (micros() * 13));
  
  // 开机动画：快速展示 1-6 的骰子
  Serial.println("🎲 开机动画...");
  for (int d = 1; d <= 6; d++) {
    showDice(d);
    delay(250);
  }
  clearAll();
  delay(200);
  
  // 默认显示 1
  currentDice = 1;
  showDice(currentDice);
  
  Serial.println("✅ 就绪！按 A 掷骰子，按 B 换颜色");
  Serial.println();
}

// ==================== loop ====================
void loop() {
  // 正在掷骰子时不响应按键
  if (isRolling) return;
  
  // === 按键A：掷骰子 ===
  bool currentKeyA = digitalRead(KEY_A);
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    delay(50);  // 消抖
    if (digitalRead(KEY_A) == LOW) {  // 确认仍然按下
      Serial.println("🎲 掷骰子！");
      rollDice();
    }
  }
  lastKeyA = currentKeyA;
  
  // === 按键B：切换颜色 ===
  bool currentKeyB = digitalRead(KEY_B);
  if (lastKeyB == HIGH && currentKeyB == LOW) {
    delay(50);  // 消抖
    if (digitalRead(KEY_B) == LOW) {  // 确认仍然按下
      switchColor();
    }
  }
  lastKeyB = currentKeyB;
  
  delay(10);  // 小延时降低CPU占用
}
