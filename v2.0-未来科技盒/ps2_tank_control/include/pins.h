#pragma once

// ==================== PS2 手柄引脚 ====================
#define PS2_CLK 41
#define PS2_CMD 9
#define PS2_CS  42
#define PS2_DAT 10

// ==================== 电机引脚 ====================
// M1(左上): GPIO11=正转, GPIO12=反转
// M2(右上): GPIO14=正转, GPIO13=反转 ← 注意顺序
// M3(左下): GPIO15=正转, GPIO16=反转
// M4(右下): GPIO18=正转, GPIO17=反转 ← 注意顺序
#define M1_FWD 11
#define M1_REV 12
#define M2_FWD 14
#define M2_REV 13
#define M3_FWD 15
#define M3_REV 16
#define M4_FWD 18
#define M4_REV 17

// ==================== 蜂鸣器 ====================
#define BUZZER_PIN 26
