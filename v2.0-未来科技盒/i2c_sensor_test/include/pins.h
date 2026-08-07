#ifndef PINS_H
#define PINS_H

// ========== I2C 引脚 ==========
#define I2C_SDA 39
#define I2C_SCL 40

// ========== I2C 设备地址 ==========
#define LIS3DHTR_ADDR 0x18   // 加速度计
#define VEML6040_ADDR 0x10   // 颜色传感器
#define DHT20_ADDR    0x38   // 温湿度传感器

// ========== LED 矩阵 ==========
#define LED_ROW1 33
#define LED_ROW2 34
#define LED_ROW3 35
#define LED_COL1 36
#define LED_COL2 37
#define LED_COL3 38

// ========== 按键 ==========
#define KEY_A 21
#define KEY_B 0

#endif
