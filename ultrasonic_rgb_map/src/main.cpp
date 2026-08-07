#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "pins.h"

Adafruit_NeoPixel strip(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// 距离范围 (cm)
#define MIN_DIST 2.0
#define MAX_DIST 50.0

// 中值滤波缓冲区
#define SAMPLE_SIZE 5
float samples[SAMPLE_SIZE];
int sampleIndex = 0;

// 读取超声波距离 (SIG单线模式)
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

// 中值滤波
float getFilteredDistance() {
  samples[sampleIndex] = readUltrasonic();
  sampleIndex = (sampleIndex + 1) % SAMPLE_SIZE;

  float sorted[SAMPLE_SIZE];
  memcpy(sorted, samples, sizeof(samples));

  // 冒泡排序
  for (int i = 0; i < SAMPLE_SIZE - 1; i++) {
    for (int j = 0; j < SAMPLE_SIZE - i - 1; j++) {
      if (sorted[j] > sorted[j + 1]) {
        float temp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = temp;
      }
    }
  }

  return sorted[SAMPLE_SIZE / 2];
}

// 将距离映射到颜色：近=红 → 中=黄 → 远=绿
uint32_t distanceToColor(float distance) {
  if (distance < 0) return strip.Color(0, 0, 0);  // 无读数，熄灭

  float ratio = constrain((distance - MIN_DIST) / (MAX_DIST - MIN_DIST), 0.0, 1.0);

  uint8_t r, g, b;

  if (ratio < 0.5) {
    // 红 → 黄
    float t = ratio * 2.0;
    r = 255;
    g = (uint8_t)(255 * t);
    b = 0;
  } else {
    // 黄 → 绿
    float t = (ratio - 0.5) * 2.0;
    r = (uint8_t)(255 * (1.0 - t));
    g = 255;
    b = 0;
  }

  return strip.Color(r, g, b);
}

// 根据距离点亮LED数量 (近=少, 远=多)
void displayDistance(float distance) {
  uint32_t color = distanceToColor(distance);

  if (distance < 0) {
    // 传感器无读数，全灭
    strip.clear();
    strip.show();
    return;
  }

  // 距离越近亮的灯越少，距离越远亮的灯越多
  float ratio = constrain((distance - MIN_DIST) / (MAX_DIST - MIN_DIST), 0.0, 1.0);
  int litCount = map(ratio * 100, 0, 100, 1, NUM_LEDS);

  strip.clear();
  for (int i = 0; i < litCount; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();

  // 初始化采样缓冲区
  for (int i = 0; i < SAMPLE_SIZE; i++) {
    samples[i] = readUltrasonic();
    delay(60);
  }

  Serial.println("超声波→RGB映射 已启动");
  Serial.printf("超声波引脚: GPIO%d (接口4)\n", ULTRASONIC_PIN);
  Serial.printf("距离范围: %.0f ~ %.0f cm\n", MIN_DIST, MAX_DIST);
  Serial.println("近=红色(1灯) → 中=黄色(5灯) → 远=绿色(9灯)");
}

void loop() {
  float distance = getFilteredDistance();

  if (distance > 0) {
    Serial.printf("距离: %.1f cm\n", distance);
  } else {
    Serial.println("距离: 超出范围");
  }

  displayDistance(distance);
  delay(100);
}
