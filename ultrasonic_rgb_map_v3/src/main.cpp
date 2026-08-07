#include <Adafruit_NeoPixel.h>
#include "pins.h"

Adafruit_NeoPixel strip(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// 距离映射范围（cm）
#define MIN_DIST 2.0
#define MAX_DIST 100.0

// 5 次中值滤波
#define SAMPLE_COUNT 5

float samples[SAMPLE_COUNT];
int sampleIdx = 0;

// 读取超声波距离（SIG 单线模式）
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
  samples[sampleIdx] = readUltrasonic();
  sampleIdx = (sampleIdx + 1) % SAMPLE_COUNT;

  float sorted[SAMPLE_COUNT];
  int validCount = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    if (samples[i] > 0) {
      sorted[validCount++] = samples[i];
    }
  }

  if (validCount == 0) return -1;

  // 简单冒泡排序
  for (int i = 0; i < validCount - 1; i++) {
    for (int j = i + 1; j < validCount; j++) {
      if (sorted[j] < sorted[i]) {
        float tmp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = tmp;
      }
    }
  }

  return sorted[validCount / 2];
}

// 距离映射到颜色：近=红 → 中=黄 → 远=绿
uint32_t distanceToColor(float dist) {
  float ratio = constrain((dist - MIN_DIST) / (MAX_DIST - MIN_DIST), 0.0, 1.0);

  uint8_t r, g, b;
  if (ratio < 0.5) {
    // 近(红) → 中(黄)
    float t = ratio * 2.0;
    r = 255;
    g = (uint8_t)(255 * t);
    b = 0;
  } else {
    // 中(黄) → 远(绿)
    float t = (ratio - 0.5) * 2.0;
    r = (uint8_t)(255 * (1.0 - t));
    g = 255;
    b = 0;
  }

  return strip.Color(r, g, b);
}

// 点亮 LED 数量随距离变化：越近灯越少，越远灯越多
void displayDistance(float dist) {
  strip.clear();

  if (dist < 0) {
    // 无回波，全部灯灭
    strip.show();
    return;
  }

  float ratio = constrain((dist - MIN_DIST) / (MAX_DIST - MIN_DIST), 0.0, 1.0);
  int litCount = map((int)(ratio * 100), 0, 100, 1, NUM_LEDS);
  uint32_t color = distanceToColor(dist);

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

  // 初始化采样数组
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    samples[i] = -1;
  }

  Serial.println("超声波距离 → RGB 灯映射 (接口3 GPIO3)");
  Serial.println("近=红色(1灯) → 中=黄色(5灯) → 远=绿色(9灯)");
}

void loop() {
  float dist = getFilteredDistance();

  if (dist > 0) {
    Serial.printf("距离: %.1f cm\n", dist);
  } else {
    Serial.println("距离: --- (无回波)");
  }

  displayDistance(dist);
  delay(100);
}
