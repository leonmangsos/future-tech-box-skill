#ifndef PINS_H
#define PINS_H

// ==================== I2C 引脚 ====================
#define I2C_SDA 39
#define I2C_SCL 40

// ==================== LED 矩阵引脚 ====================
// 行引脚（阳极/高电平有效）
// 注意：GPIO33 是物理下方，GPIO35 是物理上方
#define LED_ROW1 33  // 物理下方
#define LED_ROW2 34  // 物理中间
#define LED_ROW3 35  // 物理上方

// 列引脚（阴极/低电平有效）
#define LED_COL1 36  // 左列
#define LED_COL2 37  // 中列
#define LED_COL3 38  // 右列

#endif
