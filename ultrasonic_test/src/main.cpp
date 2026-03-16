/**
 * 未来科技盒 2.0 - 超声波测距传感器测试
 * 
 * 硬件连接：超声波传感器接到接口1 (Grove1)
 * - 信号引脚: GPIO5 (GROVE1_PIN1)
 * 
 * 测量范围: 3cm ~ 350cm
 * 
 * 注意：超声波传感器不要接 D5 接口，避免数据冲突
 */

#include <Arduino.h>

// 超声波传感器引脚 - 接口1 (Grove1)
#define ULTRASONIC_PIN 5  // GPIO5 = Grove接口1第1脚

// 测量参数
#define SOUND_SPEED 0.034  // 声速 cm/us (340m/s = 0.034cm/us)
#define MAX_DISTANCE 400   // 最大测量距离 cm
#define TIMEOUT_US (MAX_DISTANCE * 2 / SOUND_SPEED)  // 超时时间 us

/**
 * 读取超声波传感器距离
 * @return 距离(cm)，-1 表示超出范围或无回波
 */
float readUltrasonic() {
  // 1. 发送触发脉冲 (10us HIGH)
  pinMode(ULTRASONIC_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_PIN, LOW);
  
  // 2. 切换为输入模式，等待回波
  pinMode(ULTRASONIC_PIN, INPUT);
  
  // 3. 测量回波脉冲宽度
  unsigned long duration = pulseIn(ULTRASONIC_PIN, HIGH, TIMEOUT_US);
  
  // 4. 计算距离
  if (duration == 0) {
    return -1;  // 超时，无回波
  }
  
  // 距离 = 时间 × 声速 / 2 (来回)
  float distance = duration * SOUND_SPEED / 2.0;
  
  // 过滤无效数据
  if (distance < 3 || distance > 350) {
    return -1;
  }
  
  return distance;
}

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);  // 等待串口稳定
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  未来科技盒 2.0 - 超声波测距传感器");
  Serial.println("========================================");
  Serial.println("接口: Grove1 (GPIO5)");
  Serial.println("测量范围: 3cm ~ 350cm");
  Serial.println();
}

void loop() {
  // 读取距离
  float distance = readUltrasonic();
  
  // 输出到串口
  if (distance > 0) {
    Serial.print("距离: ");
    Serial.print(distance, 1);  // 保留1位小数
    Serial.println(" cm");
  } else {
    Serial.println("距离: 超出范围 或 无障碍物");
  }
  
  // 每 500ms 测量一次
  delay(500);
}
