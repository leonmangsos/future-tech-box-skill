#ifndef PINS_H
#define PINS_H

// ========== 电机引脚定义 ==========
// 电机布局（顺时针）：
//     前方
// ┌─────────┐
// │  M1  M2 │  M1=左上  M2=右上
// │         │
// │  M3  M4 │  M3=左下  M4=右下
// └─────────┘
//     后方

// M1 - 左上
#define M1_FWD 11  // 正转
#define M1_REV 12  // 反转

// M2 - 右上（注意：方向相反）
#define M2_FWD 14  // 正转
#define M2_REV 13  // 反转

// M3 - 左下
#define M3_FWD 15  // 正转
#define M3_REV 16  // 反转

// M4 - 右下（注意：方向相反）
#define M4_FWD 18  // 正转
#define M4_REV 17  // 反转

// ========== 按键引脚 ==========
#define KEY_A 21
#define KEY_B 0

// ========== 蜂鸣器 ==========
#define BUZZER_PIN 26

#endif
