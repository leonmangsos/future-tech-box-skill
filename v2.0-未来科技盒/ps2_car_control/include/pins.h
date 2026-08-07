#ifndef PINS_H
#define PINS_H

// ==================== PS2 手柄引脚 ====================
#define PS2_CLK 41
#define PS2_CMD 9
#define PS2_CS  42
#define PS2_DAT 10

// ==================== 电机引脚定义 ====================
// 小车布局（俯视）：
//     前方
//   M1    M2    M1=左上  M2=右上
//   M3    M4    M3=左下  M4=右下
//     后方

#define M1_FWD 11   // 左上正转
#define M1_REV 12   // 左上反转
#define M2_FWD 14   // 右上正转（注意：GPIO14正转，GPIO13反转）
#define M2_REV 13   // 右上反转
#define M3_FWD 15   // 左下正转
#define M3_REV 16   // 左下反转
#define M4_FWD 18   // 右下正转（注意：GPIO18正转，GPIO17反转）
#define M4_REV 17   // 右下反转

// ==================== 按键引脚 ====================
#define KEY_A 21
#define KEY_B 0

// ==================== 蜂鸣器引脚 ====================
#define BUZZER_PIN 26

#endif // PINS_H
