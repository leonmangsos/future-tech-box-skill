/**
 * 未来科技盒 2.0 - I2C 传感器综合测试
 * 
 * 测试的传感器：
 * 1. LIS3DHTR - 三轴加速度计（地址 0x18）
 * 2. VEML6040 - RGBW 颜色传感器（地址 0x10）
 * 3. DHT20 - 温湿度传感器（地址 0x38）
 * 
 * 按键A: 切换显示模式（加速度/颜色/温湿度）
 * 按键B: 执行 I2C 扫描
 */

#include <Arduino.h>
#include <Wire.h>
#include "pins.h"

// 尝试引入传感器库（如果可用）
#ifdef USE_LIS3DHTR
#include "LIS3DHTR.h"
LIS3DHTR<TwoWire> accel;
bool hasAccel = false;
#endif

#ifdef USE_VEML6040
#include "veml6040.h"
VEML6040 colorSensor;
bool hasColor = false;
#endif

#ifdef USE_DHT20
#include "Grove_Temperature_And_Humidity_Sensor.h"
DHT dht(DHT20);
bool hasTemp = false;
#endif

// 显示模式
int displayMode = 0;  // 0=扫描, 1=加速度, 2=颜色, 3=温湿度
bool lastKeyA = HIGH;
bool lastKeyB = HIGH;

// I2C 扫描函数
void scanI2C() {
  Serial.println("\n========== I2C 设备扫描 ==========");
  int deviceCount = 0;
  
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("发现设备地址: 0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);
      
      // 识别已知设备
      if (addr == 0x18 || addr == 0x19) {
        Serial.print(" - LIS3DHTR 加速度计");
      } else if (addr == 0x10) {
        Serial.print(" - VEML6040 颜色传感器");
      } else if (addr == 0x38) {
        Serial.print(" - DHT20 温湿度传感器");
      }
      Serial.println();
      deviceCount++;
    }
  }
  
  Serial.print("共发现 ");
  Serial.print(deviceCount);
  Serial.println(" 个 I2C 设备");
  Serial.println("====================================\n");
}

// 读取加速度计（简单实现，不依赖库）
void readAccelerometer() {
  Serial.println("--- 加速度计 (LIS3DHTR @ 0x18) ---");
  
  Wire.beginTransmission(0x18);
  byte error = Wire.endTransmission();
  
  if (error != 0) {
    Serial.println("未检测到加速度计！");
    return;
  }
  
  // 读取 WHO_AM_I 寄存器
  Wire.beginTransmission(0x18);
  Wire.write(0x0F);  // WHO_AM_I
  Wire.endTransmission(false);
  Wire.requestFrom(0x18, 1);
  
  if (Wire.available()) {
    byte whoami = Wire.read();
    Serial.print("WHO_AM_I: 0x");
    Serial.println(whoami, HEX);
    
    if (whoami == 0x33) {
      Serial.println("确认为 LIS3DHTR 设备");
      
      // 使能传感器：设置 CTRL_REG1 = 0x57 (100Hz, 所有轴使能)
      Wire.beginTransmission(0x18);
      Wire.write(0x20);  // CTRL_REG1
      Wire.write(0x57);  // 100Hz, 所有轴
      Wire.endTransmission();
      
      delay(10);
      
      // 读取加速度数据 (0x28-0x2D，连续读取)
      Wire.beginTransmission(0x18);
      Wire.write(0x28 | 0x80);  // 0x80 启用自动递增
      Wire.endTransmission(false);
      Wire.requestFrom(0x18, 6);
      
      if (Wire.available() >= 6) {
        int16_t x = Wire.read() | (Wire.read() << 8);
        int16_t y = Wire.read() | (Wire.read() << 8);
        int16_t z = Wire.read() | (Wire.read() << 8);
        
        // 转换为 g 值（±2g 范围）
        float ax = x / 16384.0;
        float ay = y / 16384.0;
        float az = z / 16384.0;
        
        Serial.print("X: "); Serial.print(ax, 3);
        Serial.print(" g  Y: "); Serial.print(ay, 3);
        Serial.print(" g  Z: "); Serial.print(az, 3);
        Serial.println(" g");
      }
    }
  }
}

// 读取颜色传感器（简单实现）
void readColorSensor() {
  Serial.println("--- 颜色传感器 (VEML6040 @ 0x10) ---");
  
  Wire.beginTransmission(0x10);
  byte error = Wire.endTransmission();
  
  if (error != 0) {
    Serial.println("未检测到颜色传感器！");
    return;
  }
  
  // 配置传感器：320ms 积分时间，自动模式
  Wire.beginTransmission(0x10);
  Wire.write(0x00);  // CONF 寄存器
  Wire.write(0x30);  // IT=320ms, AF=AUTO, SD=ENABLE
  Wire.write(0x00);
  Wire.endTransmission();
  
  delay(350);  // 等待积分完成
  
  // 读取 RGBW 值
  uint16_t colors[4];
  const char* names[] = {"R", "G", "B", "W"};
  
  for (int i = 0; i < 4; i++) {
    Wire.beginTransmission(0x10);
    Wire.write(0x08 + i);  // RED=0x08, GREEN=0x09, BLUE=0x0A, WHITE=0x0B
    Wire.endTransmission(false);
    Wire.requestFrom(0x10, 2);
    
    if (Wire.available() >= 2) {
      colors[i] = Wire.read() | (Wire.read() << 8);
      Serial.print(names[i]); Serial.print(": ");
      Serial.print(colors[i]);
      Serial.print("  ");
    }
  }
  Serial.println();
  
  // 判断主色
  if (colors[3] > 500) {  // 有足够光线
    int maxIdx = 0;
    for (int i = 1; i < 3; i++) {
      if (colors[i] > colors[maxIdx]) maxIdx = i;
    }
    Serial.print("主要颜色: ");
    Serial.println(names[maxIdx]);
  }
}

// 读取温湿度传感器（简单实现）
void readTempHumidity() {
  Serial.println("--- 温湿度传感器 (DHT20 @ 0x38) ---");
  
  Wire.beginTransmission(0x38);
  byte error = Wire.endTransmission();
  
  if (error != 0) {
    Serial.println("未检测到温湿度传感器！");
    return;
  }
  
  // 发送测量命令
  Wire.beginTransmission(0x38);
  Wire.write(0xAC);  // 触发测量
  Wire.write(0x33);
  Wire.write(0x00);
  Wire.endTransmission();
  
  delay(80);  // 等待测量完成
  
  // 读取数据
  Wire.requestFrom(0x38, 7);
  if (Wire.available() >= 7) {
    byte status = Wire.read();
    
    if ((status & 0x80) == 0) {  // 测量完成
      uint32_t humidity = Wire.read();
      humidity = (humidity << 8) | Wire.read();
      humidity = (humidity << 4) | (Wire.read() >> 4);
      
      uint32_t temp = Wire.read() & 0x0F;
      temp = (temp << 8) | Wire.read();
      temp = (temp << 8) | Wire.read();
      
      float h = humidity / 1048576.0 * 100.0;
      float t = temp / 1048576.0 * 200.0 - 50.0;
      
      Serial.print("温度: ");
      Serial.print(t, 1);
      Serial.print(" °C  湿度: ");
      Serial.print(h, 1);
      Serial.println(" %");
    } else {
      Serial.println("测量未完成，请重试");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("未来科技盒 2.0 - I2C 传感器测试");
  Serial.println("========================================");
  Serial.println("按键A: 切换显示模式");
  Serial.println("按键B: I2C 扫描");
  Serial.println("========================================\n");
  
  // 初始化 I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // 初始化按键
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  
  // 执行初始扫描
  scanI2C();
}

void loop() {
  bool currentKeyA = digitalRead(KEY_A);
  bool currentKeyB = digitalRead(KEY_B);
  
  // 按键A：切换模式
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    displayMode = (displayMode + 1) % 4;
    Serial.print("\n>>> 切换到模式 ");
    Serial.print(displayMode);
    Serial.print(": ");
    switch (displayMode) {
      case 0: Serial.println("I2C 扫描"); break;
      case 1: Serial.println("加速度计"); break;
      case 2: Serial.println("颜色传感器"); break;
      case 3: Serial.println("温湿度"); break;
    }
    delay(200);
  }
  
  // 按键B：立即执行 I2C 扫描
  if (lastKeyB == HIGH && currentKeyB == LOW) {
    scanI2C();
    delay(200);
  }
  
  lastKeyA = currentKeyA;
  lastKeyB = currentKeyB;
  
  // 定时读取传感器
  static unsigned long lastRead = 0;
  if (millis() - lastRead > 1000) {
    lastRead = millis();
    
    switch (displayMode) {
      case 0:
        // 扫描模式不自动执行
        break;
      case 1:
        readAccelerometer();
        break;
      case 2:
        readColorSensor();
        break;
      case 3:
        readTempHumidity();
        break;
    }
  }
}
